#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "tiled_loader.h"

typedef enum {
    PLAYER_STATE_IDLE = 0,
    PLAYER_STATE_WALK,
    PLAYER_STATE_ACTION1,
    PLAYER_STATE_ACTION2,
    PLAYER_STATE_ACTION3
} PlayerState;

typedef struct PlayerSprite {
    Texture2D texture;
    int rows;//rows in sheet
    int columns;//cols in sheet
    int frameWidth;//texture.width / columns
    int frameHeight;//texture.height / rows
} PlayerSprite;

typedef struct PlayerPhysics {
    Vector2 position;
    float scale;// How big to draw
    float speed; //moce speed
    float collisionShrinkFactor;//Shrink factor for collision box
} PlayerPhysics;

typedef struct Player {
    // The "active" sprite used for drawing currentlly
    PlayerSprite sprite;

    // The default walking/idle sprite sheet
    PlayerSprite walkSprite;

    // second sprite sheet for actions
    PlayerSprite actionSprite;

    //animation info
    PlayerState state;
    int facingDir;// 0=down, 1=up, 2=left, 3=right
    float frameTime;//Timmer for animation timing
    float frameDelay;// Time between frames
    int currentFrame;// Current column/position in animation in9= sprite row

    PlayerPhysics physics;
} Player;

void InitPlayer(Player* p, const char* walkSpritePath, Vector2 startPos, float scaleVal);
void LoadActionSprite(Player* p, const char* actionSpritePath, int rows, int columns);
void UpdatePlayer(Player* p, GameMap* map, float dt);
void DrawPlayer(Player* p);
void UnloadPlayer(Player* p);
Rectangle GetPlayerCollisionRect(const Player* p);

#endif
