#ifndef TILED_LOADER_H
#define TILED_LOADER_H

#include "raylib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Vector2* points;
    int pointCount;
} Polygon;

// a tile's collision data (from tilesets not used if using object layer collisions)
typedef struct {
    Polygon* polygons;
    int polygonCount;
} TileCollision;

// loaded from Tiled JSON
typedef struct {
    int firstgid;// Global ID for where this tileset starts
    char* source;//filename "GrassHills.tsj"
    Texture2D texture;// Loaded texture for the tileset image
    int tileWidth;//(from tileset)
    int tileHeight;//(from tileset)
    int imageWidth;//entire image
    int imageHeight;
    int tileCount;// Num of tiles in tileset
    TileCollision* collisions; //uses Collision object level
} Tileset;

typedef struct {
    int width;
    int height;
    int* tiles;//array of tile IDs converted 0-based -1 indicates no tile
} TileLayer;

//from object layer collisions
typedef struct {
    Polygon* polygons;
    int count;
} CollisionLayer;


typedef struct {
    char* targetMap;// target map name without .tmj
    float startX;// Starting X coordinate (in pixels) in the target map
    float startY;// Starting Y coordinate (in pixels) in the target map
    Polygon triggerArea;// The polygon area that triggers the transition
} MapTransition;

// complete game map.
typedef struct {
    int mapWidth;       // in tiles
    int mapHeight;      // in tiles
    int tileWidth;      // in pixels
    int tileHeight;     // in pixels
    Tileset* tilesets;
    int tilesetCount;
    TileLayer* tileLayers;   // rendered Tile Layer 1 first ...
    int tileLayerCount;
    CollisionLayer collisionLayer;  //from object layer"Collision"
    MapTransition* transitions;     //from object layer "MapTransition"
    int transitionCount;
} GameMap;

// Loads a game map from "Tiled/Tiledmaps/somemap.tmj"
GameMap LoadGameMap(const char* mapFilePath);

//free stuff
void UnloadGameMap(GameMap* map);

#ifdef __cplusplus
}
#endif

#endif
