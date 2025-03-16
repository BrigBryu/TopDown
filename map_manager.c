#include "map_manager.h"
#include "constants.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int GetTilesetIndex(GameMap* map, int globalTileID) {
    int index = -1;
    for (int i = 0; i < map->tilesetCount; i++) {
        if (globalTileID >= map->tilesets[i].firstgid)
            index = i;
        else
            break;
    }
    return index;
}

static void RenderLayer(GameMap* map, TileLayer* layer, float scale) {
    for (int y = 0; y < layer->height; y++) {
        for (int x = 0; x < layer->width; x++) {
            int tileIndex = layer->tiles[y * layer->width + x];
            if (tileIndex < 0) continue;  //skip empty

            int globalTileID = tileIndex + 1;
            int tsIndex = GetTilesetIndex(map, globalTileID);
            if (tsIndex < 0) continue;

            Tileset ts = map->tilesets[tsIndex];
            int localTileID = globalTileID - ts.firstgid;
            int tsColumns = ts.texture.width / ts.tileWidth;

            Rectangle sourceRec = {
                (localTileID % tsColumns) * ts.tileWidth,
                (localTileID / tsColumns) * ts.tileHeight,
                ts.tileWidth,
                ts.tileHeight
            };

            Rectangle destRec = {
                x * map->tileWidth * scale,
                y * map->tileHeight * scale,
                map->tileWidth * scale,
                map->tileHeight * scale
            };

            DrawTexturePro(ts.texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }
}

static void RenderGameMap(GameMap* map, float scale) {
    for (int i = 0; i < map->tileLayerCount; i++) {
        RenderLayer(map, &map->tileLayers[i], scale);
    }
}

static void RenderCollisionPolygons(GameMap* map, float scale) {
#if DEBUG_DRAW_COLLISIONS
    for (int i = 0; i < map->collisionLayer.count; i++) {
        Polygon poly = map->collisionLayer.polygons[i];
        if (poly.pointCount < 2) continue;
        for (int j = 0; j < poly.pointCount; j++) {
            int next = (j + 1) % poly.pointCount;
            Vector2 p1 = { poly.points[j].x * scale, poly.points[j].y * scale };
            Vector2 p2 = { poly.points[next].x * scale, poly.points[next].y * scale };
            DrawLine((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, BLUE);
        }
    }
#endif
}

static void RenderMapTransitionPolygons(GameMap* map, float scale) {
#if DEBUG_DRAW_MAPTRANSITIONS
    for (int i = 0; i < map->transitionCount; i++) {
        Polygon poly = map->transitions[i].triggerArea;
        if (poly.pointCount < 2) continue;
        for (int j = 0; j < poly.pointCount; j++) {
            int next = (j + 1) % poly.pointCount;
            Vector2 p1 = { poly.points[j].x * scale, poly.points[j].y * scale };
            Vector2 p2 = { poly.points[next].x * scale, poly.points[next].y * scale };
            DrawLine((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, RED);
        }
    }
#endif
}

static int CheckMapTransitionCollision(GameMap* map, Rectangle playerRect, float scale, int* transitionIndex) {
    for (int i = 0; i < map->transitionCount; i++) {
        Polygon poly = map->transitions[i].triggerArea;
        if (poly.pointCount < 2) continue;
        if (poly.pointCount > 128) continue;

        Vector2 worldPoly[128];
        for (int j = 0; j < poly.pointCount; j++) {
            worldPoly[j].x = poly.points[j].x * scale;
            worldPoly[j].y = poly.points[j].y * scale;
        }
        
        // Debug output
        TraceLog(LOG_DEBUG, "Transition %d: Player at (%.2f, %.2f), checking against polygon:", 
                i, playerRect.x, playerRect.y);
        for (int j = 0; j < poly.pointCount; j++) {
            TraceLog(LOG_DEBUG, "Point %d: (%.2f, %.2f)", j, worldPoly[j].x, worldPoly[j].y);
        }

        Vector2 corners[4] = {
            { playerRect.x, playerRect.y },
            { playerRect.x + playerRect.width, playerRect.y },
            { playerRect.x + playerRect.width, playerRect.y + playerRect.height },
            { playerRect.x, playerRect.y + playerRect.height }
        };
        
        for (int c = 0; c < 4; c++) {
            if (CheckCollisionPointPoly(corners[c], worldPoly, poly.pointCount)) {
                *transitionIndex = i;
                TraceLog(LOG_INFO, "Transition triggered at corner %d", c);
                return 1;
            }
        }
    }
    return 0;
}

// MARK- Public MapManager functions
MapManager* CreateMapManager(const char* mapFilePath) {
    MapManager* manager = (MapManager*)malloc(sizeof(MapManager));
    if (manager) {
        // Initialize to NULL/0 first
        manager->entityManager = NULL;
        manager->currentMapName = NULL;
        
        // Load the map
        manager->currentMap = LoadGameMap(mapFilePath);
        
        // Create entity manager
        manager->entityManager = CreateEntityManager();
        if (!manager->entityManager) {
            TraceLog(LOG_ERROR, "Failed to create entity manager");
            free(manager);
            return NULL;
        }
        
        // Extract and store map name
        const char* fileName = strrchr(mapFilePath, '/');
        if (fileName) {
            fileName++; // Skip the '/'
        } else {
            fileName = mapFilePath;
        }
        manager->currentMapName = strdup(fileName);
        char* dot = strrchr(manager->currentMapName, '.');
        if (dot) *dot = '\0'; // Remove extension
        
        TraceLog(LOG_INFO, "Created map manager for map: %s", manager->currentMapName);
        
        // Spawn initial entities
        SpawnMapEntities(manager);
    }
    return manager;
}

void DestroyMapManager(MapManager* manager) {
    if (manager) {
        UnloadGameMap(&manager->currentMap);
        DestroyEntityManager(manager->entityManager);
        free(manager->currentMapName);
        free(manager);
    }
}

void SpawnMapEntities(MapManager* manager) {
    TraceLog(LOG_INFO, "Spawning entities for map: %s", manager->currentMapName);
    
    if (strcmp(manager->currentMapName, "field") == 0) {
        // Spawn field map entities
        Entity* slime = CreateSlime((Vector2){300, 300}, 2.0f);
        if (slime) {
            InitEntitySprite(&slime->sprite, 
                "SproutLandsPack/Characters/BasicCharakterSpritesheet.png", 4, 4, 0.1f);
            AddEntity(manager->entityManager, slime);
            TraceLog(LOG_INFO, "Spawned slime at (300, 300)");
        }
        
        Entity* bat = CreateBat((Vector2){400, 400}, 2.0f);
        if (bat) {
            InitEntitySprite(&bat->sprite, 
                "SproutLandsPack/Characters/BasicCharakterSpritesheet.png", 4, 4, 0.1f);
            AddEntity(manager->entityManager, bat);
            TraceLog(LOG_INFO, "Spawned bat at (400, 400)");
        }
    }
    else if (strcmp(manager->currentMapName, "cave") == 0) {
        // Spawn cave map entities
        Entity* skeleton = CreateSkeleton((Vector2){200, 200}, 2.0f);
        if (skeleton) {
            InitEntitySprite(&skeleton->sprite, 
                "SproutLandsPack/Characters/BasicCharakterSpritesheet.png", 4, 4, 0.1f);
            AddEntity(manager->entityManager, skeleton);
            TraceLog(LOG_INFO, "Spawned skeleton at (200, 200)");
        }
    }
}

void ClearMapEntities(MapManager* manager) {
    DestroyEntityManager(manager->entityManager);
    manager->entityManager = CreateEntityManager();
}

void UpdateMapManager(MapManager* manager, Player* player, float dt) {
    if (!manager || !manager->entityManager) return;
    
    // Check for map transitions
    Rectangle playerRect = GetPlayerCollisionRect(player);
    int transitionIndex = -1;
    
    // Use existing CheckMapTransitionCollision function instead of CheckCollisionPolyRec
    if (CheckMapTransitionCollision(&manager->currentMap, playerRect, PIXEL_SCALE, &transitionIndex)) {
        MapTransition* transition = &manager->currentMap.transitions[transitionIndex];
        
        // Validate transition data
        if (!transition->targetMap) {
            TraceLog(LOG_ERROR, "Invalid transition: NULL target map");
            return;
        }

        char newMapPath[512];
        snprintf(newMapPath, sizeof(newMapPath), "Tiled/Tiledmaps/%s.tmj", transition->targetMap);
        
        TraceLog(LOG_INFO, "Loading new map: %s", newMapPath);
        
        // Store transition data before clearing anything
        char* targetMap = strdup(transition->targetMap);
        float startX = transition->startX;
        float startY = transition->startY;
        
        if (!targetMap) {
            TraceLog(LOG_ERROR, "Failed to copy target map name");
            return;
        }
        
        // Clear current map's entities
        ClearMapEntities(manager);
        
        // Unload current map and load new map
        UnloadGameMap(&manager->currentMap);
        manager->currentMap = LoadGameMap(newMapPath);
        
        // Update map name
        free(manager->currentMapName);
        manager->currentMapName = strdup(targetMap);
        
        // Update player position
        player->physics.position.x = startX;
        player->physics.position.y = startY;
        
        // Spawn new map's entities
        SpawnMapEntities(manager);
        
        TraceLog(LOG_INFO, "Map transition complete. New player position: (%.2f, %.2f)", 
                 player->physics.position.x, player->physics.position.y);
        
        // Clean up
        free(targetMap);
    }
    
    // Update all entities if movement is enabled
    if (manager->entityManager && ENTITIES_CAN_MOVE) {
        UpdateEntities(manager->entityManager, &manager->currentMap, dt);
    }
}

void RenderMapManager(MapManager* manager, float scale) {
    // Render map layers
    RenderGameMap(&manager->currentMap, scale);
    
    // Render entities
    DrawEntities(manager->entityManager);
    
    // Debug rendering if enabled
    #if DEBUG_DRAW_COLLISIONS
    RenderCollisionPolygons(&manager->currentMap, scale);
    #endif
    
    #if DEBUG_DRAW_MAPTRANSITIONS
    RenderMapTransitionPolygons(&manager->currentMap, scale);
    #endif
}
