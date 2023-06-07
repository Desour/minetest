/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __ANDROID__
#error This file may only be compiled for android!
#endif

#include "util/numeric.h"
#include "porting.h"
#include "porting_android.h"
#include "threading/thread.h"
#include "config.h"
#include "filesys.h"
#include "log.h"

#include <sstream>
#include <exception>
#include <cstdlib>
#include <signal.h>
#include <set>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <vector>

#ifdef GPROF
#include "prof.h"
#endif

extern int main(int argc, char *argv[]);

void android_main(android_app *app)
{
	int retval = 0;
	porting::app_global = app;

	porting::selfExecInit();

	Thread::setName("Main");

	char *argv[] = {strdup(PROJECT_NAME), strdup("--verbose"), nullptr};
	try {
		main(ARRLEN(argv) - 1, argv);
	} catch (std::exception &e) {
		errorstream << "Uncaught exception in main thread: " << e.what() << std::endl;
		retval = -1;
	} catch (...) {
		errorstream << "Uncaught exception in main thread!" << std::endl;
		retval = -1;
	}
	free(argv[0]);
	free(argv[1]);

	porting::cleanupAndroid();
	infostream << "Shutting down." << std::endl;
	exit(retval);
}

namespace porting {
android_app *app_global;
JNIEnv      *jnienv;
jclass       nativeActivity;

jclass findClass(const std::string &classname)
{
	if (jnienv == nullptr)
		return nullptr;

	jclass nativeactivity = jnienv->FindClass("android/app/NativeActivity");
	jmethodID getClassLoader = jnienv->GetMethodID(
			nativeactivity, "getClassLoader", "()Ljava/lang/ClassLoader;");
	jobject cls = jnienv->CallObjectMethod(
						app_global->activity->clazz, getClassLoader);
	jclass classLoader = jnienv->FindClass("java/lang/ClassLoader");
	jmethodID findClass = jnienv->GetMethodID(classLoader, "loadClass",
					"(Ljava/lang/String;)Ljava/lang/Class;");
	jstring strClassName = jnienv->NewStringUTF(classname.c_str());
	return (jclass) jnienv->CallObjectMethod(cls, findClass, strClassName);
}

void initAndroid()
{
	porting::jnienv = nullptr;
	JavaVM *jvm = app_global->activity->vm;
	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = PROJECT_NAME_C "NativeThread";
	lJavaVMAttachArgs.group = nullptr;

	if (jvm->AttachCurrentThread(&porting::jnienv, &lJavaVMAttachArgs) == JNI_ERR) {
		errorstream << "Failed to attach native thread to jvm" << std::endl;
		exit(-1);
	}

	nativeActivity = findClass("net/minetest/minetest/GameActivity");
	if (nativeActivity == nullptr)
		errorstream <<
			"porting::initAndroid unable to find Java native activity class" <<
			std::endl;

#ifdef GPROF
	// in the start-up code
	__android_log_print(ANDROID_LOG_ERROR, PROJECT_NAME_C,
			"Initializing GPROF profiler");
	monstartup("libMinetest.so");
#endif
}

void cleanupAndroid()
{
#ifdef GPROF
	errorstream << "Shutting down GPROF profiler" << std::endl;
	setenv("CPUPROFILE", (path_user + DIR_DELIM + "gmon.out").c_str(), 1);
	moncleanup();
#endif

	porting::selfExecDestroy();

	JavaVM *jvm = app_global->activity->vm;
	jvm->DetachCurrentThread();
}

static std::string readJavaString(jstring j_str)
{
	// Get string as a UTF-8 C string
	const char *c_str = jnienv->GetStringUTFChars(j_str, nullptr);
	// Save it
	std::string str(c_str);
	// And free the C string
	jnienv->ReleaseStringUTFChars(j_str, c_str);
	return str;
}

void initializePathsAndroid()
{
	// Set user and share paths
	{
		jmethodID getUserDataPath = jnienv->GetMethodID(nativeActivity,
				"getUserDataPath", "()Ljava/lang/String;");
		FATAL_ERROR_IF(getUserDataPath==nullptr,
				"porting::initializePathsAndroid unable to find Java getUserDataPath method");
		jobject result = jnienv->CallObjectMethod(app_global->activity->clazz, getUserDataPath);
		std::string str = readJavaString((jstring) result);
		path_user = str;
		path_share = str;
		path_locale = str + DIR_DELIM + "locale";
	}

	// Set cache path
	{
		jmethodID getCachePath = jnienv->GetMethodID(nativeActivity,
				"getCachePath", "()Ljava/lang/String;");
		FATAL_ERROR_IF(getCachePath==nullptr,
				"porting::initializePathsAndroid unable to find Java getCachePath method");
		jobject result = jnienv->CallObjectMethod(app_global->activity->clazz, getCachePath);
		path_cache = readJavaString((jstring) result);

		migrateCachePath();
	}
}

static int self_exec_socket = -1;
static pid_t self_exec_pid = 0;

int self_exec_spawned_proc(char *args, size_t args_size, int fd) noexcept
{
	int argc = 0;
	std::vector<char *> argv;
	for (size_t i = 0; i < args_size; i++) {
		void *next = memchr(args + i, 0, args_size - i);
		if (next) {
			argc++;
			argv.push_back(args + i);
			i = (char *)next - args;
		} else {
			break;
		}
	}
	argv.push_back(nullptr);
	if (fd >= 0 && fd != SELF_EXEC_SPAWN_FD && dup2(fd, SELF_EXEC_SPAWN_FD) < 0)
		return EXIT_FAILURE;
	return main(argc, argv.data());
}

void self_exec_spawner_proc(int socket) noexcept
{
	porting::app_global = nullptr;
	porting::jnienv = nullptr;

	std::set<pid_t> children;
	try {
		for (;;) {
			char buf[2048] = {};
			struct iovec iov = {buf, sizeof(buf)};
			char cmsg_buf alignas(cmsghdr) [CMSG_SPACE(sizeof(int))] = {};
			msghdr msg = {};
			msg.msg_iov = &iov;
			msg.msg_iovlen = 1;
			msg.msg_control = cmsg_buf;
			msg.msg_controllen = CMSG_SPACE(sizeof(int));
			ssize_t recv_size = recvmsg(socket, &msg, 0);
			if (recv_size <= 0)
				break;
			if (recv_size == sizeof(pid_t) + 1 && buf[sizeof(pid_t)] == 'k') {
				pid_t pid;
				memcpy(&pid, buf, sizeof(pid));
				if (children.find(pid) != children.end()) {
					children.erase(pid);
					kill(pid, SIGKILL);
					waitpid(pid, nullptr, 0);
				}
			} else {
				int fd = -1;
				if (CMSG_FIRSTHDR(&msg))
					memcpy(&fd, CMSG_DATA(CMSG_FIRSTHDR(&msg)), sizeof(fd));
				pid_t pid = fork();
				if (pid == 0) {
					exit(self_exec_spawned_proc(buf, recv_size, fd));
				} else if (pid > 0) {
					children.insert(pid);
					send(socket, &pid, sizeof(pid), MSG_EOR);
				}
				close(fd);
			}
		}
	} catch (...) {
	}
	for (pid_t pid : children)
		kill(pid, SIGKILL);
	for (pid_t pid : children)
		waitpid(pid, nullptr, 0);
}

bool selfExecInit() noexcept
{
	int sockets[2];
	if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockets) == 0) {
		pid_t pid = fork();
		if (pid == 0) {
			close(sockets[0]);
			self_exec_spawner_proc(sockets[1]);
			exit(EXIT_SUCCESS);
		} else if (pid > 0) {
			close(sockets[1]);
			self_exec_socket = sockets[0];
			self_exec_pid = pid;
			return true;
		}
		close(sockets[0]);
		close(sockets[1]);
	}
	return false;
}

bool selfExecDestroy() noexcept
{
	shutdown(self_exec_socket, SHUT_RDWR);
	close(self_exec_socket);
	bool ok = waitpid(self_exec_pid, nullptr, 0) < 0;
	self_exec_socket = -1;
	self_exec_pid = 0;
	return ok;
}

pid_t selfExecSpawn(const char *const argv[], int fd) noexcept
{
	msghdr msg = {};
	size_t argc;
	for (argc = 0; argv[argc]; argc++)
		;
	if (argc < 1)
		return -1;
	std::unique_ptr<iovec[]> iov(new iovec[argc]);
	for (size_t i = 0; i < argc; i++) {
		iov[i].iov_base = (void *)argv[i];
		iov[i].iov_len = strlen(argv[i]) + 1;
	}
	msg.msg_iov = iov.get();
	msg.msg_iovlen = argc;
	char cmsg_buf alignas(cmsghdr) [CMSG_SPACE(sizeof(fd))] = {};
	if (fd >= 0) {
		msg.msg_control = cmsg_buf;
		msg.msg_controllen = CMSG_SPACE(sizeof(fd));
		CMSG_FIRSTHDR(&msg)->cmsg_level = SOL_SOCKET;
		CMSG_FIRSTHDR(&msg)->cmsg_type = SCM_RIGHTS;
		CMSG_FIRSTHDR(&msg)->cmsg_len = CMSG_LEN(sizeof(fd));
		memcpy(CMSG_DATA(CMSG_FIRSTHDR(&msg)), &fd, sizeof(fd));
	}
	if (sendmsg(self_exec_socket, &msg, MSG_EOR) < 0)
		return -1;
	pid_t pid = 0;
	if (recv(self_exec_socket, &pid, sizeof(pid), 0) <= 0)
		return -1;
	return pid;
}

bool selfExecKill(pid_t pid) noexcept
{
	char buf[sizeof(pid) + 1];
	memcpy(buf, &pid, sizeof(pid));
	buf[sizeof(pid)] = 'k';
	return send(self_exec_socket, buf, sizeof(buf), MSG_EOR) >= 0;
}


void showInputDialog(const std::string &acceptButton, const std::string &hint,
		const std::string &current, int editType)
{
	jmethodID showdialog = jnienv->GetMethodID(nativeActivity, "showDialog",
		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");

	FATAL_ERROR_IF(showdialog == nullptr,
		"porting::showInputDialog unable to find Java showDialog method");

	jstring jacceptButton = jnienv->NewStringUTF(acceptButton.c_str());
	jstring jhint         = jnienv->NewStringUTF(hint.c_str());
	jstring jcurrent      = jnienv->NewStringUTF(current.c_str());
	jint    jeditType     = editType;

	jnienv->CallVoidMethod(app_global->activity->clazz, showdialog,
			jacceptButton, jhint, jcurrent, jeditType);
}

void openURIAndroid(const std::string &url)
{
	jmethodID url_open = jnienv->GetMethodID(nativeActivity, "openURI",
		"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
		"porting::openURIAndroid unable to find Java openURI method");

	jstring jurl = jnienv->NewStringUTF(url.c_str());
	jnienv->CallVoidMethod(app_global->activity->clazz, url_open, jurl);
}

void shareFileAndroid(const std::string &path)
{
	jmethodID url_open = jnienv->GetMethodID(nativeActivity, "shareFile",
			"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
			"porting::shareFileAndroid unable to find Java shareFile method");

	jstring jurl = jnienv->NewStringUTF(path.c_str());
	jnienv->CallVoidMethod(app_global->activity->clazz, url_open, jurl);
}

int getInputDialogState()
{
	jmethodID dialogstate = jnienv->GetMethodID(nativeActivity,
			"getDialogState", "()I");

	FATAL_ERROR_IF(dialogstate == nullptr,
		"porting::getInputDialogState unable to find Java getDialogState method");

	return jnienv->CallIntMethod(app_global->activity->clazz, dialogstate);
}

std::string getInputDialogValue()
{
	jmethodID dialogvalue = jnienv->GetMethodID(nativeActivity,
			"getDialogValue", "()Ljava/lang/String;");

	FATAL_ERROR_IF(dialogvalue == nullptr,
		"porting::getInputDialogValue unable to find Java getDialogValue method");

	jobject result = jnienv->CallObjectMethod(app_global->activity->clazz,
			dialogvalue);
	return readJavaString((jstring) result);
}

#ifndef SERVER
float getDisplayDensity()
{
	static bool firstrun = true;
	static float value = 0;

	if (firstrun) {
		jmethodID getDensity = jnienv->GetMethodID(nativeActivity,
				"getDensity", "()F");

		FATAL_ERROR_IF(getDensity == nullptr,
			"porting::getDisplayDensity unable to find Java getDensity method");

		value = jnienv->CallFloatMethod(app_global->activity->clazz, getDensity);
		firstrun = false;
	}

	return value;
}

v2u32 getDisplaySize()
{
	static bool firstrun = true;
	static v2u32 retval;

	if (firstrun) {
		jmethodID getDisplayWidth = jnienv->GetMethodID(nativeActivity,
				"getDisplayWidth", "()I");

		FATAL_ERROR_IF(getDisplayWidth == nullptr,
			"porting::getDisplayWidth unable to find Java getDisplayWidth method");

		retval.X = jnienv->CallIntMethod(app_global->activity->clazz,
				getDisplayWidth);

		jmethodID getDisplayHeight = jnienv->GetMethodID(nativeActivity,
				"getDisplayHeight", "()I");

		FATAL_ERROR_IF(getDisplayHeight == nullptr,
			"porting::getDisplayHeight unable to find Java getDisplayHeight method");

		retval.Y = jnienv->CallIntMethod(app_global->activity->clazz,
				getDisplayHeight);

		firstrun = false;
	}

	return retval;
}

std::string getLanguageAndroid()
{
	jmethodID getLanguage = jnienv->GetMethodID(nativeActivity,
			"getLanguage", "()Ljava/lang/String;");

	FATAL_ERROR_IF(getLanguage == nullptr,
		"porting::getLanguageAndroid unable to find Java getLanguage method");

	jobject result = jnienv->CallObjectMethod(app_global->activity->clazz,
			getLanguage);
	return readJavaString((jstring) result);
}

#endif // ndef SERVER
}
