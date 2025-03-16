#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include "tiled_loader.h"
#include "entity_manager.h"
#include "raylib.h"
#include "player.h"

typedef struct MapManager {
    GameMap currentMap;
    EntityManager* entityManager;  // Each map has its own entity manager
    char* currentMapName;         // Store current map name for reference
} MapManager;

MapManager* CreateMapManager(const char* mapFilePath);

void DestroyMapManager(MapManager* manager);

// Updates the map manager:
// - Updates the player (moveme, collision...) on the current map
// - Checks for map transition collisions if found unloads current map and loads target map
void UpdateMapManager(MapManager* manager, Player* player, float dt);

//renders current map all tile layers plus debug
void RenderMapManager(MapManager* manager, float scale);

// Add these new functions
void SpawnMapEntities(MapManager* manager);
void ClearMapEntities(MapManager* manager);

#endif
