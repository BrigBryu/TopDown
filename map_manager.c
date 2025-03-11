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
        //check player corners for collision
        Vector2 corners[4] = {
            { playerRect.x, playerRect.y },
            { playerRect.x + playerRect.width, playerRect.y },
            { playerRect.x + playerRect.width, playerRect.y + playerRect.height },
            { playerRect.x, playerRect.y + playerRect.height }
        };
        for (int c = 0; c < 4; c++) {
            if (CheckCollisionPointPoly(corners[c], worldPoly, poly.pointCount)) {
                *transitionIndex = i;
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
        manager->currentMap = LoadGameMap(mapFilePath);
    }
    return manager;
}

void DestroyMapManager(MapManager* manager) {
    if (manager) {
        UnloadGameMap(&manager->currentMap);
        free(manager);
    }
}

void UpdateMapManager(MapManager* manager, Player* player, float dt) {
    // Update the player
    UpdatePlayer(player, &manager->currentMap, dt);
    
    Rectangle playerRect = GetPlayerCollisionRect(player);
    
    // Check if player triggers a map transition
    int transitionIndex = -1;
    if (CheckMapTransitionCollision(&manager->currentMap, playerRect, player->physics.scale, &transitionIndex)) {
        //new map path
        char newMapPath[512];
        snprintf(newMapPath, sizeof(newMapPath), "Tiled/Tiledmaps/%s.tmj", 
                 manager->currentMap.transitions[transitionIndex].targetMap);
        TraceLog(LOG_INFO, "Switching map: %s", newMapPath);
        
        // Get the new start pos
        Vector2 newStart = { manager->currentMap.transitions[transitionIndex].startX, 
                             manager->currentMap.transitions[transitionIndex].startY };
                             
        // Unload the current map and load the new map
        UnloadGameMap(&manager->currentMap);
        manager->currentMap = LoadGameMap(newMapPath); 
        player->physics.position = newStart;
    }
}

void RenderMapManager(MapManager* manager, float scale) {
    // Render tile layers
    RenderGameMap(&manager->currentMap, scale);
    // Render debug if on
    RenderCollisionPolygons(&manager->currentMap, scale);
    RenderMapTransitionPolygons(&manager->currentMap, scale); 
    #if DEBUG_GRID
    for (int y = 0; y < manager->currentMap.mapHeight; y++) {
        for (int x = 0; x < manager->currentMap.mapWidth; x++) {
            Rectangle tileRect = {
                x * manager->currentMap.tileWidth * scale,
                y * manager->currentMap.tileHeight * scale,
                manager->currentMap.tileWidth * scale,
                manager->currentMap.tileHeight * scale
            };
            DrawRectangleLines((int)tileRect.x, (int)tileRect.y, (int)tileRect.width, (int)tileRect.height, DARKGRAY);
        }
    }
    #endif
}
