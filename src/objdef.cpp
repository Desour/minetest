/*
Minetest
Copyright (C) 2010-2015 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include "objdef.h"
#include "util/numeric.h"
#include "log.h"
#include "gamedef.h"
#include <utility>

ObjDefManager::ObjDefManager(IGameDef *gamedef, ObjDefType type)
{
	m_objtype = type;
	m_ndef = gamedef ? gamedef->getNodeDefManager() : NULL;
}


ObjDefHandle ObjDefManager::add(std::unique_ptr<ObjDef> &&obj)
{
	assert(obj);

	if (obj->name.length() && getByName(obj->name))
		return OBJDEF_INVALID_HANDLE;

	auto obj_p = obj.get();
	u32 index = addRaw(std::move(obj));
	if (index == OBJDEF_INVALID_INDEX)
		return OBJDEF_INVALID_HANDLE;

	obj_p->handle = createHandle(index, m_objtype, obj_p->uid);
	return obj_p->handle;
}


ObjDef *ObjDefManager::get(ObjDefHandle handle) const
{
	u32 index = validateHandle(handle);
	return (index != OBJDEF_INVALID_INDEX) ? getRaw(index) : NULL;
}


std::unique_ptr<ObjDef> ObjDefManager::set(ObjDefHandle handle, std::unique_ptr<ObjDef> obj)
{
	u32 index = validateHandle(handle);
	if (index == OBJDEF_INVALID_INDEX)
		return nullptr;

	auto obj_p = obj.get();
	auto oldobj = setRaw(index, std::move(obj));

	obj_p->uid    = oldobj->uid;
	obj_p->index  = oldobj->index;
	obj_p->handle = oldobj->handle;

	return oldobj;
}


u32 ObjDefManager::addRaw(std::unique_ptr<ObjDef> &&obj)
{
	size_t nobjects = m_objects.size();
	if (nobjects >= OBJDEF_MAX_ITEMS)
		return OBJDEF_INVALID_INDEX;

	obj->index = nobjects;

	// Ensure UID is nonzero so that a valid handle == OBJDEF_INVALID_HANDLE
	// is not possible.  The slight randomness bias isn't very significant.
	obj->uid = myrand() & OBJDEF_UID_MASK;
	if (obj->uid == 0)
		obj->uid = 1;

	ObjDef *obj_p = obj.get();
	m_objects.push_back(std::move(obj));

	infostream << "ObjDefManager: added " << getObjectTitle()
		<< ": name=\"" << obj_p->name
		<< "\" index=" << obj_p->index
		<< " uid="     << obj_p->uid
		<< std::endl;

	return nobjects;
}


ObjDef *ObjDefManager::getRaw(u32 index) const
{
	return m_objects[index].get();
}


std::unique_ptr<ObjDef> ObjDefManager::setRaw(u32 index, std::unique_ptr<ObjDef> obj)
{
	return std::exchange(m_objects[index], std::move(obj));
}


ObjDef *ObjDefManager::getByName(const std::string &name) const
{
	for (size_t i = 0; i != m_objects.size(); i++) {
		ObjDef *obj = m_objects[i].get();
		if (obj && !strcasecmp(name.c_str(), obj->name.c_str()))
			return obj;
	}

	return nullptr;
}


u32 ObjDefManager::validateHandle(ObjDefHandle handle) const
{
	ObjDefType type;
	u32 index;
	u32 uid;

	bool is_valid =
		(handle != OBJDEF_INVALID_HANDLE)         &&
		decodeHandle(handle, &index, &type, &uid) &&
		(type == m_objtype)                       &&
		(index < m_objects.size())                &&
		(m_objects[index]->uid == uid);

	return is_valid ? index : -1;
}


ObjDefHandle ObjDefManager::createHandle(u32 index, ObjDefType type, u32 uid)
{
	ObjDefHandle handle = 0;
	set_bits(&handle, 0, 18, index);
	set_bits(&handle, 18, 6, type);
	set_bits(&handle, 24, 7, uid);

	u32 parity = calc_parity(handle);
	set_bits(&handle, 31, 1, parity);

	return handle ^ OBJDEF_HANDLE_SALT;
}


bool ObjDefManager::decodeHandle(ObjDefHandle handle, u32 *index,
	ObjDefType *type, u32 *uid)
{
	handle ^= OBJDEF_HANDLE_SALT;

	u32 parity = get_bits(handle, 31, 1);
	set_bits(&handle, 31, 1, 0);
	if (parity != calc_parity(handle))
		return false;

	*index = get_bits(handle, 0, 18);
	*type  = (ObjDefType)get_bits(handle, 18, 6);
	*uid   = get_bits(handle, 24, 7);
	return true;
}

// Cloning

void ObjDef::cloneTo(ObjDef *def) const
{
	def->index = index;
	def->uid = uid;
	def->handle = handle;
	def->name = name;
}

void ObjDefManager::cloneTo(ObjDefManager *mgr) const
{
	mgr->m_ndef = m_ndef;
	mgr->m_objects.reserve(m_objects.size());
	for (const auto &obj : m_objects)
		mgr->m_objects.push_back(obj->clone());
	mgr->m_objtype = m_objtype;
}
