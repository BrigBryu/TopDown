#include "tiled_loader.h"
#include "constants.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// read whole file into a string useing cjson
static char* ReadFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(length + 1);
    if (buffer) {
        fread(buffer, 1, length, file);
        buffer[length] = '\0';
    }
    fclose(file);
    return buffer;
}

// make Polygon from cJSON array of points at offsetX offsetY
static Polygon ParsePolygon(cJSON* polygonArray, float offsetX, float offsetY) {
    Polygon poly = {0};
    if (!polygonArray || !cJSON_IsArray(polygonArray))
        return poly;
    int count = cJSON_GetArraySize(polygonArray);
    poly.pointCount = count;
    poly.points = (Vector2*)malloc(count * sizeof(Vector2));
    int i = 0;
    cJSON* pointItem = NULL;
    cJSON_ArrayForEach(pointItem, polygonArray) {
        double x = cJSON_GetObjectItem(pointItem, "x")->valuedouble;
        double y = cJSON_GetObjectItem(pointItem, "y")->valuedouble;
        poly.points[i++] = (Vector2){ (float)(x + offsetX), (float)(y + offsetY) };
    }
    return poly;
}

// make TileCollision from json
static TileCollision ParseTileCollision(cJSON* tileJSON) {
    TileCollision collision = {0};
    cJSON* objectgroup = cJSON_GetObjectItem(tileJSON, "objectgroup");
    if (!objectgroup)
        return collision;
    cJSON* objects = cJSON_GetObjectItem(objectgroup, "objects");
    if (!objects || !cJSON_IsArray(objects))
        return collision;
    int objectCount = cJSON_GetArraySize(objects);
    collision.polygonCount = objectCount;
    collision.polygons = (Polygon*)malloc(objectCount * sizeof(Polygon));
    int i = 0;
    cJSON* objectItem = NULL;
    cJSON_ArrayForEach(objectItem, objects) {
        float offsetX = 0, offsetY = 0;
        cJSON* xItem = cJSON_GetObjectItem(objectItem, "x");
        cJSON* yItem = cJSON_GetObjectItem(objectItem, "y");
        if (xItem)
            offsetX = (float)xItem->valuedouble;
        if (yItem)
            offsetY = (float)yItem->valuedouble;
        cJSON* polygonArray = cJSON_GetObjectItem(objectItem, "polygon");
        if (!polygonArray)
            polygonArray = cJSON_GetObjectItem(objectItem, "polyline");
        if (polygonArray && cJSON_IsArray(polygonArray))
            collision.polygons[i] = ParsePolygon(polygonArray, offsetX, offsetY);
        else {
            collision.polygons[i].points = NULL;
            collision.polygons[i].pointCount = 0;
        }
        i++;
    }
    return collision;
}

static Tileset LoadTileset(const char* tilesetFilename, int firstgid) {
    Tileset ts = {0};
    char* jsonText = ReadFile(tilesetFilename);
    if (!jsonText) {
        printf("Failed to load tileset file: %s\n", tilesetFilename);
        return ts;
    }
    cJSON* root = cJSON_Parse(jsonText);
    free(jsonText);
    if (!root) {
        printf("Failed to parse tileset JSON: %s\n", tilesetFilename);
        return ts;
    }
    ts.tileWidth  = cJSON_GetObjectItem(root, "tilewidth")->valueint;
    ts.tileHeight = cJSON_GetObjectItem(root, "tileheight")->valueint;
    ts.tileCount  = cJSON_GetObjectItem(root, "tilecount")->valueint;
    ts.imageWidth = cJSON_GetObjectItem(root, "imagewidth")->valueint;
    ts.imageHeight= cJSON_GetObjectItem(root, "imageheight")->valueint;
    ts.firstgid   = firstgid;
    cJSON* imageItem = cJSON_GetObjectItem(root, "image");
    if (imageItem && imageItem->valuestring) {
        char imagePath[512];
        const char* rawPath = imageItem->valuestring;
        if (strncmp(rawPath, "../../", 6) == 0)
            rawPath += 6;
        snprintf(imagePath, sizeof(imagePath), "%s", rawPath);
        ts.texture = LoadTexture(imagePath);
    } else {
        printf("Tileset image not found in %s\n", tilesetFilename);
    }
    ts.source = strdup(tilesetFilename);
    ts.collisions = (TileCollision*)calloc(ts.tileCount, sizeof(TileCollision));
    cJSON* tilesArray = cJSON_GetObjectItem(root, "tiles");
    if (tilesArray && cJSON_IsArray(tilesArray)) {
        cJSON* tileEntry = NULL;
        cJSON_ArrayForEach(tileEntry, tilesArray) {
            int localID = cJSON_GetObjectItem(tileEntry, "id")->valueint;
            if (localID >= 0 && localID < ts.tileCount)
                ts.collisions[localID] = ParseTileCollision(tileEntry);
        }
    }
    cJSON_Delete(root);
    return ts;
}

GameMap LoadGameMap(const char* mapFilePath) {
    GameMap map = {0};
    char* jsonText = ReadFile(mapFilePath);
    if (!jsonText) {
        printf("Failed to load map file: %s\n", mapFilePath);
        return map;
    }
    cJSON* root = cJSON_Parse(jsonText);
    free(jsonText);
    if (!root) {
        printf("Error parsing map JSON: %s\n", mapFilePath);
        return map;
    }

    // Basic map properties
    map.mapWidth   = cJSON_GetObjectItem(root, "width")->valueint;
    map.mapHeight  = cJSON_GetObjectItem(root, "height")->valueint;
    // force BASE_TILE_SIZE
    map.tileWidth  = BASE_TILE_SIZE;
    map.tileHeight = BASE_TILE_SIZE;

    //Load Tilesets
    cJSON* tsArray = cJSON_GetObjectItem(root, "tilesets");
    if (tsArray && cJSON_IsArray(tsArray)) {
        int tsCount = cJSON_GetArraySize(tsArray);
        map.tilesetCount = tsCount;
        map.tilesets = (Tileset*)malloc(tsCount * sizeof(Tileset));
        int i = 0;
        cJSON* tsEntry = NULL;
        cJSON_ArrayForEach(tsEntry, tsArray) {
            int firstgid = cJSON_GetObjectItem(tsEntry, "firstgid")->valueint;
            char* source = cJSON_GetObjectItem(tsEntry, "source")->valuestring;
            if (source) {
                char* relative = source;
                if (strncmp(source, "../", 3) == 0)
                    relative = source + 3;
                char tsPath[512];
                if (strncmp(relative, "Tilesets/", 9) == 0)
                    snprintf(tsPath, sizeof(tsPath), "Tiled/%s", relative);
                else
                    snprintf(tsPath, sizeof(tsPath), "Tiled/Tilesets/%s", relative);
                char* ext = strrchr(tsPath, '.');
                if (ext && strcmp(ext, ".tsx") == 0)
                    strcpy(ext, ".tsj");
                map.tilesets[i++] = LoadTileset(tsPath, firstgid);
            }
        }
    }

    //count num tile layers and map transitions
    int tileLayerCount = 0, transitionCount = 0;
    cJSON* layers = cJSON_GetObjectItem(root, "layers");
    cJSON* layerCounter = layers ? layers->child : NULL;
    while (layerCounter) {
        char* layerType = cJSON_GetObjectItem(layerCounter, "type")->valuestring;
        char* layerName = cJSON_GetObjectItem(layerCounter, "name")->valuestring;
        if (strcmp(layerType, "tilelayer") == 0) {
            tileLayerCount++;
        } else if (strcmp(layerType, "objectgroup") == 0 && strcmp(layerName, "MapTransition") == 0) {
            transitionCount++;
        }
        layerCounter = layerCounter->next;
    }
    //malloc
    map.tileLayerCount = tileLayerCount;
    map.tileLayers = (TileLayer*)malloc(tileLayerCount * sizeof(TileLayer));
    map.transitionCount = transitionCount;
    map.transitions = (MapTransition*)malloc(transitionCount * sizeof(MapTransition));
    map.collisionLayer.count = 0;
    map.collisionLayer.polygons = NULL;

    // Process layers after count with fresh iterator
    int tIdx = 0, trIdx = 0;
    cJSON* layerIter = layers ? layers->child : NULL;
    while (layerIter) {
        char* layerType = cJSON_GetObjectItem(layerIter, "type")->valuestring;
        char* layerName = cJSON_GetObjectItem(layerIter, "name")->valuestring;
        if (strcmp(layerType, "tilelayer") == 0) {
            cJSON* data = cJSON_GetObjectItem(layerIter, "data");
            if (!data || !cJSON_IsArray(data)) {
                layerIter = layerIter->next;
                continue;
            }
            map.tileLayers[tIdx].width = map.mapWidth;
            map.tileLayers[tIdx].height = map.mapHeight;
            map.tileLayers[tIdx].tiles = (int*)malloc(map.mapWidth * map.mapHeight * sizeof(int));
            int idx = 0;
            cJSON* tileVal = NULL;
            cJSON_ArrayForEach(tileVal, data) {
                int id = tileVal->valueint;
                map.tileLayers[tIdx].tiles[idx++] = (id > 0) ? id - 1 : -1;
            }
            tIdx++;
        }
        else if (strcmp(layerType, "objectgroup") == 0) {
            // Map Transitions
            if (strcmp(layerName, "MapTransition") == 0) {
                cJSON* objects = cJSON_GetObjectItem(layerIter, "objects");
                if (!objects || !cJSON_IsArray(objects)) {
                    layerIter = layerIter->next;
                    continue;
                }
                cJSON* obj;
                cJSON_ArrayForEach(obj, objects) {
                    float offsetX = 0, offsetY = 0;
                    cJSON* xItem = cJSON_GetObjectItem(obj, "x");
                    cJSON* yItem = cJSON_GetObjectItem(obj, "y");
                    if (xItem) offsetX = (float)xItem->valuedouble;
                    if (yItem) offsetY = (float)yItem->valuedouble;
                    cJSON* nameItem = cJSON_GetObjectItem(obj, "name");
                    if (!nameItem) continue;
                    char* name = nameItem->valuestring;
                    // Expected format: "targetMap:tileX,tileY" (e.g., "smallFlowerMap:0,10")
                    char targetMap[256] = {0};
                    float tileX = 0, tileY = 0;
                    if (sscanf(name, "%[^:]:%f,%f", targetMap, &tileX, &tileY) == 3) {
                        map.transitions[trIdx].targetMap = strdup(targetMap);
                        map.transitions[trIdx].startX = tileX * BASE_TILE_SIZE * PIXEL_SCALE;
                        map.transitions[trIdx].startY = tileY * BASE_TILE_SIZE * PIXEL_SCALE;
                        TraceLog(LOG_INFO, "Parsed transition: targetMap = %s, tile coords = (%.2f, %.2f) -> pixel coords = (%.2f, %.2f)",
                            targetMap, tileX, tileY, map.transitions[trIdx].startX, map.transitions[trIdx].startY);
                    } else {
                        continue;
                    }
                    cJSON* polygonArray = cJSON_GetObjectItem(obj, "polygon");
                    if (!polygonArray)
                        polygonArray = cJSON_GetObjectItem(obj, "polyline");
                    if (polygonArray && cJSON_IsArray(polygonArray))
                        map.transitions[trIdx].triggerArea = ParsePolygon(polygonArray, offsetX, offsetY);
                    else {
                        map.transitions[trIdx].triggerArea.points = NULL;
                        map.transitions[trIdx].triggerArea.pointCount = 0;
                    }
                    trIdx++;
                }
            }
            // Collision Objects
            else if (strcmp(layerName, "Collision") == 0) {
                cJSON* objects = cJSON_GetObjectItem(layerIter, "objects");
                if (!objects || !cJSON_IsArray(objects)) {
                    layerIter = layerIter->next;
                    continue;
                }
                int count = cJSON_GetArraySize(objects);
                map.collisionLayer.count = count;
                map.collisionLayer.polygons = (Polygon*)malloc(count * sizeof(Polygon));
                int i = 0;
                cJSON* obj;
                cJSON_ArrayForEach(obj, objects) {
                    float offsetX = 0, offsetY = 0;
                    cJSON* xItem = cJSON_GetObjectItem(obj, "x");
                    cJSON* yItem = cJSON_GetObjectItem(obj, "y");
                    if (xItem) offsetX = (float)xItem->valuedouble;
                    if (yItem) offsetY = (float)yItem->valuedouble;
                    cJSON* polygonArray = cJSON_GetObjectItem(obj, "polygon");
                    if (!polygonArray)
                        polygonArray = cJSON_GetObjectItem(obj, "polyline");
                    if (polygonArray && cJSON_IsArray(polygonArray))
                        map.collisionLayer.polygons[i] = ParsePolygon(polygonArray, offsetX, offsetY);
                    else {
                        map.collisionLayer.polygons[i].points = NULL;
                        map.collisionLayer.polygons[i].pointCount = 0;
                    }
                    i++;
                }
            }
        }
        layerIter = layerIter->next;
    }

    cJSON_Delete(root);
    return map;
}

void UnloadGameMap(GameMap* map) {
    int i, t, p;
    for (i = 0; i < map->tilesetCount; i++) {
        UnloadTexture(map->tilesets[i].texture);
        free(map->tilesets[i].source);
        for (t = 0; t < map->tilesets[i].tileCount; t++) {
            TileCollision* tc = &map->tilesets[i].collisions[t];
            for (p = 0; p < tc->polygonCount; p++) {
                free(tc->polygons[p].points);
            }
            free(tc->polygons);
        }
        free(map->tilesets[i].collisions);
    }
    free(map->tilesets);

    for (i = 0; i < map->tileLayerCount; i++) {
        free(map->tileLayers[i].tiles);
    }
    free(map->tileLayers);

    //collision layer
    for (i = 0; i < map->collisionLayer.count; i++) {
        free(map->collisionLayer.polygons[i].points);
    }
    free(map->collisionLayer.polygons);

    //map transitions.
    for (i = 0; i < map->transitionCount; i++) {
        free(map->transitions[i].targetMap);
        free(map->transitions[i].triggerArea.points);
    }
    free(map->transitions);
}
