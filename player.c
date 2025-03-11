#include "player.h"
#include "constants.h"
#include "raylib.h"
#include <stdlib.h>

//Collision/Debug Helpers
Rectangle GetPlayerCollisionRect(const Player* p) {
    float fullW = p->walkSprite.frameWidth * p->physics.scale;
    float fullH = p->walkSprite.frameHeight * p->physics.scale;
    float collW = fullW / p->physics.collisionShrinkFactor;
    float collH = fullH / p->physics.collisionShrinkFactor;
    float offsetX = (fullW - collW) / 2.0f;
    float offsetY = (fullH - collH) / 2.0f;
    return (Rectangle){ p->physics.position.x + offsetX, p->physics.position.y + offsetY, collW, collH };
}

static void DebugDrawPlayerCollision(Rectangle rect) {
#if DEBUG_DRAW_PLAYER_COLLISION
    DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, GREEN);
#endif
}

static int CheckCollisionPolyRectangle(Vector2* poly, int polyCount, Rectangle rec) {
    Vector2 corners[4] = {
        { rec.x,             rec.y },
        { rec.x + rec.width, rec.y },
        { rec.x + rec.width, rec.y + rec.height },
        { rec.x,             rec.y + rec.height }
    };
    // rectangle corner inside polygon
    for (int i = 0; i < 4; i++) {
        if (CheckCollisionPointPoly(corners[i], poly, polyCount))
            return 1;
    }
    // edges
    Vector2 r1 = corners[0], r2 = corners[1], r3 = corners[2], r4 = corners[3];
    for (int i = 0; i < polyCount; i++) {
        Vector2 p1 = poly[i];
        Vector2 p2 = poly[(i + 1) % polyCount];
        if (CheckCollisionLines(p1, p2, r1, r2, NULL)) return 1;
        if (CheckCollisionLines(p1, p2, r2, r3, NULL)) return 1;
        if (CheckCollisionLines(p1, p2, r3, r4, NULL)) return 1;
        if (CheckCollisionLines(p1, p2, r4, r1, NULL)) return 1;
    }
    return 0;
}

static int CheckCollisionObjects(GameMap* map, Rectangle playerRect, float scale) {
    for (int i = 0; i < map->collisionLayer.count; i++) {
        Polygon poly = map->collisionLayer.polygons[i];
        if (poly.pointCount < 2) continue;
        if (poly.pointCount > 128) continue;

        Vector2 worldPoly[128];
        for (int j = 0; j < poly.pointCount; j++) {
            worldPoly[j].x = poly.points[j].x * scale;
            worldPoly[j].y = poly.points[j].y * scale;
        }
        if (CheckCollisionPolyRectangle(worldPoly, poly.pointCount, playerRect))
            return 1;
    }
    return 0;
}


static void LoadSpriteSheet(PlayerSprite* ps, const char* path, int rows, int cols) {
    ps->texture = LoadTexture(path);
    if (ps->texture.id == 0) {
        TraceLog(LOG_ERROR, "Failed to load texture: %s", path);
    }
    ps->rows = rows;
    ps->columns = cols;
    ps->frameWidth  = ps->texture.width  / cols;
    ps->frameHeight = ps->texture.height / rows;
}

//Public

void InitPlayer(Player* p, const char* walkSpritePath, Vector2 startPos, float scaleVal) {
    LoadSpriteSheet(&p->walkSprite, walkSpritePath, 4, 4);

    p->actionSprite.texture.id = 0;
    p->actionSprite.rows = 0;
    p->actionSprite.columns = 0;
    p->actionSprite.frameWidth = 0;
    p->actionSprite.frameHeight = 0;

    p->sprite = p->walkSprite;

    p->physics.position = startPos;
    p->physics.scale = scaleVal;
    p->physics.speed = 2.0f;
    p->physics.collisionShrinkFactor = 3.0f;

    p->state = PLAYER_STATE_IDLE;
    p->facingDir = 0; // 0 = down
    p->frameTime = 0.0f;
    p->frameDelay = 0.1f;
    p->currentFrame = 0;
}

void LoadActionSprite(Player* p, const char* actionSpritePath, int rows, int columns) {
    LoadSpriteSheet(&p->actionSprite, actionSpritePath, rows, columns);
}

static void SelectActiveSprite(Player* p) {
    if ((p->state == PLAYER_STATE_ACTION1 ||
         p->state == PLAYER_STATE_ACTION2 ||
         p->state == PLAYER_STATE_ACTION3) &&
         p->actionSprite.texture.id != 0)
    {
        p->sprite = p->actionSprite;
    }
    else {
        p->sprite = p->walkSprite;
    }
}

void UpdatePlayer(Player* p, GameMap* map, float dt) {
    p->frameTime += dt;

    // Movement
    int moving = 0; 
    float oldX = p->physics.position.x;
    float oldY = p->physics.position.y;

    // Left
    if (IsKeyDown(KEY_LEFT)) {
        p->physics.position.x -= p->physics.speed;
        p->facingDir = 2; // left
        moving = 1;
        if (CheckCollisionObjects(map, GetPlayerCollisionRect(p), p->physics.scale)) {
            p->physics.position.x = oldX;
        }
    }
    // Right
    else if (IsKeyDown(KEY_RIGHT)) {
        p->physics.position.x += p->physics.speed;
        p->facingDir = 3; // right
        moving = 1;
        if (CheckCollisionObjects(map, GetPlayerCollisionRect(p), p->physics.scale)) {
            p->physics.position.x = oldX;
        }
    }
    // Up
    if (IsKeyDown(KEY_UP)) {
        p->physics.position.y -= p->physics.speed;
        p->facingDir = 1; // up
        moving = 1;
        if (CheckCollisionObjects(map, GetPlayerCollisionRect(p), p->physics.scale)) {
            p->physics.position.y = oldY;
        }
    }
    // Down
    else if (IsKeyDown(KEY_DOWN)) {
        p->physics.position.y += p->physics.speed;
        p->facingDir = 0; // down
        moving = 1;
        if (CheckCollisionObjects(map, GetPlayerCollisionRect(p), p->physics.scale)) {
            p->physics.position.y = oldY;
        }
    }
    
    if (moving && (p->state != PLAYER_STATE_ACTION1 &&
                   p->state != PLAYER_STATE_ACTION2 &&
                   p->state != PLAYER_STATE_ACTION3)) {
        p->state = PLAYER_STATE_WALK;
    }
    else if (!moving && p->state == PLAYER_STATE_WALK) {
        p->state = PLAYER_STATE_IDLE;
    }
    
    if (IsKeyPressed(KEY_H)) {
        p->state = PLAYER_STATE_ACTION1;
        p->currentFrame = 0;
        p->frameTime = 0.0f;
    }
    if (IsKeyPressed(KEY_J)) {
        p->state = PLAYER_STATE_ACTION2;
        p->currentFrame = 0;
        p->frameTime = 0.0f;
    }
    if (IsKeyPressed(KEY_K)) {
        p->state = PLAYER_STATE_ACTION3;
        p->currentFrame = 0;
        p->frameTime = 0.0f;
    }
    
    SelectActiveSprite(p);
    
    switch (p->state) {
        case PLAYER_STATE_IDLE:
            p->currentFrame = 0;
            break;
        case PLAYER_STATE_WALK:
            if (p->frameTime >= p->frameDelay) {
                p->currentFrame = (p->currentFrame + 1) % p->sprite.columns; 
                p->frameTime = 0.0f;
            }
            break;
        case PLAYER_STATE_ACTION1:
        case PLAYER_STATE_ACTION2:
        case PLAYER_STATE_ACTION3:
            if (p->sprite.texture.id != 0 && p->frameTime >= p->frameDelay) {
                p->currentFrame++;
                p->frameTime = 0.0f;
                if (p->currentFrame >= p->sprite.columns) {
                    p->currentFrame = 0;
                    p->state = PLAYER_STATE_IDLE;
                }
            }
            break;
    }
    
    #ifdef DEBUG_PLAYER
    TraceLog(LOG_INFO, "Player position: (%.2f, %.2f)", p->physics.position.x, p->physics.position.y);
    #endif
}

void DrawPlayer(Player* p) {
    int row = p->facingDir;
    int col = p->currentFrame;
    
    Rectangle srcRec = {
        col * (float)p->sprite.frameWidth,
        row * (float)p->sprite.frameHeight,
        (float)p->sprite.frameWidth,
        (float)p->sprite.frameHeight
    };
    
    Rectangle destRec = {
        p->physics.position.x,
        p->physics.position.y,
        p->sprite.frameWidth * p->physics.scale,
        p->sprite.frameHeight * p->physics.scale
    };
    
    DrawTexturePro(p->sprite.texture, srcRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
    
    #if DEBUG_DRAW_PLAYER_COLLISION
    Rectangle cRect = GetPlayerCollisionRect(p);
    DebugDrawPlayerCollision(cRect);
    #endif
}

void UnloadPlayer(Player* p) {
    if (p->walkSprite.texture.id != 0) {
        UnloadTexture(p->walkSprite.texture);
        p->walkSprite.texture.id = 0;
    }
    if (p->actionSprite.texture.id != 0) {
        UnloadTexture(p->actionSprite.texture);
        p->actionSprite.texture.id = 0;
    }
}
