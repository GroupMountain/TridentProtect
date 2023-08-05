#include "version.h"
#include <llapi/LoggerAPI.h>
#include <memory/hook.h>
#include <llapi/EventAPI.h>
#include <llapi/mc/Level.hpp>
#include <llapi/mc/Actor.hpp>
#include <llapi/KVDBAPI.h>
#include <llapi/mc/CompoundTag.hpp>
#include <llapi/mc/ChunkSource.hpp>
#include <llapi/mc/LevelChunk.hpp>
#include <llapi/mc/ChunkPos.hpp>
#include <llapi/mc/Spawner.hpp>
#include <llapi/mc/Dimension.hpp>
#include <llapi/mc/ActorDefinitionIdentifier.hpp>
#include <llapi/ScheduleAPI.h>
#include <filesystem>

extern Logger logger;

std::unique_ptr<KVDB> posdb;

struct FloatPos {
  float x, y, z;
  int dimid;
};

std::string_view toByteData(const FloatPos &pos) {
  const char *data = (const char *)new FloatPos(pos);
  std::string_view d(data, sizeof(FloatPos));
  return d;
}
void saveData() {
  for (auto &e : Level::getAllEntities()) {
    size_t index = 0;
    if (e->getTypeName() == "minecraft:thrown_trident") {
      auto tpos = e->getPos();
      std::string_view pos = toByteData({tpos.x, tpos.y, tpos.z, e->getDimensionId()});
      posdb->set(pos, e->getNbt()->toSNBT(0));
      index++;
      e->kill();
    }
  }
}

void loadData(const ChunkPos &pos, int dimid) {
  logger.info(__func__);
  posdb->iter([=](std::string_view key, std::string_view val) {
    auto dpos = (FloatPos *)key.data();
    if (((int)dpos->x) / 16 == pos.x && ((int)dpos->z) / 16 == pos.z && dimid == dpos->dimid) {
      auto r = Global<Level>->getSpawner().spawnProjectile(*(Global<Level>->getBlockSource(dimid)), ActorDefinitionIdentifier("minecraft:thrown_trident"), (Actor *)Global<Level>->getAllPlayers()[0], {dpos->x, dpos->y, dpos->z}, {0, 0, 0});
      logger.info(val);
      // r->setNbt(CompoundTag::fromBinaryNBT(val, 0ull, true).get());
      // logger.info(CompoundTag::fromBinaryNBT(val, 0ull, true)->toPrettySNBT());
      size_t o = 0;
      CompoundTag::fromBinaryNBT(val, o, true)->setActor(r);
      logger.info(CompoundTag::fromSNBT(val.data())->toPrettySNBT());
      posdb->del(key);
    }
    return true;
  });
}
void PluginInit() {
  logger.info(PLUGIN_NAME " by " PLUGIN_AUTHOR " loaded");

  if (std::filesystem::status(Level::getCurrentLevelPath() + "/thrown_trident_pos.db").type() == std::filesystem::file_type::not_found) {
    posdb = KVDB::create(Level::getCurrentLevelPath() + "/thrown_trident_pos.db");
  } else {
    posdb = KVDB::open(Level::getCurrentLevelPath() + "/thrown_trident_pos.db");
  }
}

THook(bool, "?stop@DedicatedServer@@UEAA_NXZ", void *_this) {
  saveData();
  return original(_this);
}
THook(void, "?onChunkLoaded@Level@@UEAAXAEAVChunkSource@@AEAVLevelChunk@@@Z", Level *level, ChunkSource *source, LevelChunk *chunk) {
  original(level, source, chunk);
  Schedule::delay(std::bind(loadData, chunk->getPosition(), chunk->getDimension().getDimensionId()), 20);
}