/*
Minetest
Copyright (C) 2010-2024 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once

#include <vector>
#include <memory>

#include "map.h"
#include "util/container.h"
#include "util/metricsbackend.h"
#include "map_settings_manager.h"

class Settings;
class MapDatabase;
class IRollbackManager;
class EmergeManager;
class ServerEnvironment;
struct BlockMakeData;
class MetricsBackend;

// TODO: this could wrap all calls to MapDatabase, including locking
struct MapDatabaseAccessor {
	/// Lock, to be taken for any operation
	std::mutex mutex;
	/// Main database
	MapDatabase *dbase = nullptr;
	/// Fallback database for read operations
	MapDatabase *dbase_ro = nullptr;

	/// Load a block, taking dbase_ro into account.
	/// @note call locked
	void loadBlock(v3s16 blockpos, std::string &ret);
};

/*
	ServerMap

	This is the only map class that is able to generate map.
*/

class ServerMap : public Map
{
public:
	/*
		savedir: directory to which map data should be saved
	*/
	ServerMap(const std::string &savedir, IGameDef *gamedef, EmergeManager *emerge, MetricsBackend *mb);
	~ServerMap();

	/*
		Get a sector from somewhere.
		- Check memory
		- Check disk (doesn't load blocks)
		- Create blank one
	*/
	MapSector *createSector(v2s16 p);

	/*
		Blocks are generated by using these and makeBlock().
	*/
	bool blockpos_over_mapgen_limit(v3s16 p);
	bool initBlockMake(v3s16 blockpos, BlockMakeData *data);
	void finishBlockMake(BlockMakeData *data,
		std::map<v3s16, MapBlock*> *changed_blocks);

	/*
		Get a block from somewhere.
		- Memory
		- Create blank
	*/
	MapBlock *createBlock(v3s16 p);

	/*
		Forcefully get a block from somewhere (blocking!).
		- Memory
		- Load from disk
		- Create blank filled with CONTENT_IGNORE

	*/
	MapBlock *emergeBlock(v3s16 p, bool create_blank=true) override;

	/*
		Try to get a block.
		If it does not exist in memory, add it to the emerge queue.
		- Memory
		- Emerge Queue (deferred disk or generate)
	*/
	MapBlock *getBlockOrEmerge(v3s16 p3d, bool generate);

	bool isBlockInQueue(v3s16 pos);

	void addNodeAndUpdate(v3s16 p, MapNode n,
			std::map<v3s16, MapBlock*> &modified_blocks,
			bool remove_metadata) override;

	/*
		Database functions
	*/
	static MapDatabase *createDatabase(const std::string &name, const std::string &savedir, Settings &conf);

	// Call these before and after saving of blocks
	void beginSave() override;
	void endSave() override;

	void save(ModifiedState save_level) override;
	void listAllLoadableBlocks(std::vector<v3s16> &dst);
	void listAllLoadedBlocks(std::vector<v3s16> &dst);

	MapgenParams *getMapgenParams();

	bool saveBlock(MapBlock *block) override;
	static bool saveBlock(MapBlock *block, MapDatabase *db, int compression_level = -1);

	// Load block in a synchronous fashion
	MapBlock *loadBlock(v3s16 p);
	// Load a block that was already read from disk. Used by EmergeManager.
	void loadBlock(const std::string &blob, v3s16 p, bool save_after_load=false);

	// Helper for deserializing blocks from disk
	// @throws SerializationError
	static void deSerializeBlock(MapBlock *block, std::istream &is);

	// Blocks are removed from the map but not deleted from memory until
	// deleteDetachedBlocks() is called, since pointers to them may still exist
	// when deleteBlock() is called.
	bool deleteBlock(v3s16 blockpos) override;

	void deleteDetachedBlocks();

	void step();

	void updateVManip(v3s16 pos);

	// For debug printing
	void PrintInfo(std::ostream &out) override;

	bool isSavingEnabled(){ return m_map_saving_enabled; }

	u64 getSeed();

	/*!
	 * Fixes lighting in one map block.
	 * May modify other blocks as well, as light can spread
	 * out of the specified block.
	 * Returns false if the block is not generated (so nothing
	 * changed), true otherwise.
	 */
	bool repairBlockLight(v3s16 blockpos,
		std::map<v3s16, MapBlock *> *modified_blocks);

	void transformLiquids(std::map<v3s16, MapBlock*> & modified_blocks,
			ServerEnvironment *env);

	void transforming_liquid_add(v3s16 p);

	MapSettingsManager settings_mgr;

protected:

	void reportMetrics(u64 save_time_us, u32 saved_blocks, u32 all_blocks) override;

private:
	friend class ModApiMapgen; // for m_transforming_liquid

	// Emerge manager
	EmergeManager *m_emerge;

	std::string m_savedir;
	bool m_map_saving_enabled;

	int m_map_compression_level;

	std::set<v3s16> m_chunks_in_progress;

	// used by deleteBlock() and deleteDetachedBlocks()
	std::vector<std::unique_ptr<MapBlock>> m_detached_blocks;

	// Queued transforming water nodes
	UniqueQueue<v3s16> m_transforming_liquid;
	f32 m_transforming_liquid_loop_count_multiplier = 1.0f;
	u32 m_unprocessed_count = 0;
	u64 m_inc_trending_up_start_time = 0; // milliseconds
	bool m_queue_size_timer_started = false;

	/*
		Metadata is re-written on disk only if this is true.
		This is reset to false when written on disk.
	*/
	bool m_map_metadata_changed = true;

	MapDatabaseAccessor m_db;

	// Map metrics
	MetricGaugePtr m_loaded_blocks_gauge;
	MetricCounterPtr m_save_time_counter;
	MetricCounterPtr m_save_count_counter;
};
