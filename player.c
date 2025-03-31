#include "player.h"
#include "constants.h"
#include "raylib.h"
#include "raymath.h"  // For Vector2 operations
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

// Add these new functions
static Rectangle CreateBasicAttackHitbox(const Player* p, Rectangle collisionRect) {
    float attackWidth = collisionRect.width * 1.5f;
    float attackHeight = collisionRect.height * 1.5f;
    
    float centerX = collisionRect.x + collisionRect.width / 2.0f;
    float centerY = collisionRect.y + collisionRect.height / 2.0f;
    
    Rectangle hitbox = {0};
    
    switch (p->facingDir) {
        case 0: // Down
            hitbox = (Rectangle){
                centerX - attackWidth / 2.0f,
                centerY,
                attackWidth,
                attackHeight / 2.0f
            };
            break;
        case 1: // Up
            hitbox = (Rectangle){
                centerX - attackWidth / 2.0f,
                centerY - attackHeight / 2.0f,
                attackWidth,
                attackHeight / 2.0f
            };
            break;
        case 2: // Left
            hitbox = (Rectangle){
                centerX - attackWidth / 2.0f,
                centerY - attackHeight / 2.0f,
                attackWidth / 2.0f,
                attackHeight
            };
            break;
        case 3: // Right
            hitbox = (Rectangle){
                centerX,
                centerY - attackHeight / 2.0f,
                attackWidth / 2.0f,
                attackHeight
            };
            break;
    }
    return hitbox;
}

// Add these new attack hitbox functions
static Rectangle CreateCursorBasedAttackHitbox(const Player* p, Rectangle collisionRect) {
    // Get player center in world coordinates
    Vector2 playerCenter = {
        collisionRect.x + collisionRect.width / 2.0f,
        collisionRect.y + collisionRect.height / 2.0f
    };
    
    // Get screen dimensions
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Get cursor position relative to screen center
    Vector2 cursorPos = GetMousePosition();
    Vector2 screenCenter = { screenWidth / 2.0f, screenHeight / 2.0f };
    
    // Calculate cursor offset from screen center
    Vector2 cursorOffset = {
        (cursorPos.x - screenCenter.x) * p->physics.scale,
        (cursorPos.y - screenCenter.y) * p->physics.scale
    };
    
    // Calculate world space cursor position relative to player
    Vector2 worldCursorPos = {
        playerCenter.x + cursorOffset.x,
        playerCenter.y + cursorOffset.y
    };
    
    // Calculate attack box dimensions
    float attackWidth = collisionRect.width * 1.0f;
    float attackHeight = collisionRect.height * 1.0f;
    
    // Calculate direction from player to cursor
    Vector2 diff = Vector2Subtract(worldCursorPos, playerCenter);
    Vector2 direction = Vector2Normalize(diff);
    
    // Position the attack hitbox between player and cursor
    float attackDistance = collisionRect.width * 1.5f; // Adjust this value to control attack range
    Vector2 attackPos = Vector2Add(playerCenter, Vector2Scale(direction, attackDistance));
    
    return (Rectangle){
        attackPos.x - attackWidth / 2.0f,
        attackPos.y - attackHeight / 2.0f,
        attackWidth,
        attackHeight
    };
}

static Rectangle CreateSuperAttackHitbox(const Player* p, Rectangle collisionRect) {
    float centerX = collisionRect.x + collisionRect.width / 2.0f;
    float centerY = collisionRect.y + collisionRect.height / 2.0f;
    
    // Create a large square area around the player
    float size = p->physics.superAttackRadius * 2.0f;
    return (Rectangle){
        centerX - size / 2.0f,
        centerY - size / 2.0f,
        size,
        size
    };
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

    p->physics.isAttacking = false;
    p->physics.attackDuration = 0.3f;
    p->physics.attackTimer = 0;
    p->physics.hitFlashTimer = 0;
    p->physics.hitFlashColor = WHITE;

    p->physics.createAttackHitbox = CreateBasicAttackHitbox; // Set default attack

    p->physics.maxHealth = 10;
    p->physics.currentHealth = 10;
    p->physics.attackDamage = 1;

    p->physics.superAttackRadius = 100.0f;  // Adjust this value as needed
    p->physics.lastCursorPos = (Vector2){0, 0};

    // Initialize dash properties
    p->physics.isDashing = 0;
    p->physics.dashTimer = 0.0f;
    p->physics.dashDuration = 0.2f;    // 0.2 seconds dash duration
    p->physics.dashSpeed = 8.0f;       // 4x normal speed
    p->physics.dashCooldown = 1.0f;    // 1 second between dashes
    p->physics.dashCooldownTimer = 0.0f;
    p->physics.dashDirection = (Vector2){0, 0};
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
    // Store old position for collision resolution
    Vector2 oldPos = p->physics.position;
    int isMoving = 0;  // Track if player is actually moving
    
    if (ENTITIES_CAN_MOVE) {
        // Track movement direction
        Vector2 moveDir = {0.0f, 0.0f};
        
        // Capture input direction
        if (IsKeyDown(KEY_RIGHT )|| IsKeyDown(KEY_D)) {
            moveDir.x += 1.0f;
            p->facingDir = 3;
            p->state = PLAYER_STATE_WALK;
            isMoving = 1;
        }
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            moveDir.x -= 1.0f;
            p->facingDir = 2;
            p->state = PLAYER_STATE_WALK;
            isMoving = 1;
        }
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
            moveDir.y -= 1.0f;
            p->facingDir = 1;
            p->state = PLAYER_STATE_WALK;
            isMoving = 1;
        }
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
            moveDir.y += 1.0f;
            p->facingDir = 0;
            p->state = PLAYER_STATE_WALK;
            isMoving = 1;
        }

        // If not moving, set to idle state
        if (!isMoving) {
            p->state = PLAYER_STATE_IDLE;
            p->currentFrame = 0;  // Reset to first frame when idle TODO add idle animation but dont have art
        }
        
        // Apply movement speed (now using the same speed for diagonal)
        float speed = p->physics.speed;
        if (moveDir.x != 0.0f && moveDir.y != 0.0f) {
            // if diagonal move (1/√2)
            speed *= 0.7071f;
        }
        
        moveDir.x = (moveDir.x != 0.0f) ? (moveDir.x > 0.0f ? 1.0f : -1.0f) : 0.0f;
        moveDir.y = (moveDir.y != 0.0f) ? (moveDir.y > 0.0f ? 1.0f : -1.0f) : 0.0f;
        
        // Apply movement
        p->physics.position.x += moveDir.x * speed;
        Rectangle playerRect = GetPlayerCollisionRect(p);
        if (CheckCollisionObjects(map, playerRect, PIXEL_SCALE)) {
            p->physics.position.x = oldPos.x;
        }
        
        p->physics.position.y += moveDir.y * speed;
        playerRect = GetPlayerCollisionRect(p);
        if (CheckCollisionObjects(map, playerRect, PIXEL_SCALE)) {
            p->physics.position.y = oldPos.y;
        }
    }

    p->frameTime += dt;

    // Store cursor position (just use screen coordinates)
    p->physics.lastCursorPos = GetMousePosition();

    // Handle attacks
    Rectangle collisionRect = GetPlayerCollisionRect(p);
    
    if (IsKeyPressed(KEY_J) && !p->physics.isAttacking) {
        // Basic attack (cursor-based)
        p->state = PLAYER_STATE_ATTACK;
        p->physics.isAttacking = 1;
        p->physics.attackTimer = p->physics.attackDuration;
        p->physics.attackHitbox = CreateCursorBasedAttackHitbox(p, collisionRect);
    }
    
    if (IsKeyPressed(KEY_K) && !p->physics.isAttacking) {
        // Strong attack (directional)
        p->state = PLAYER_STATE_ATTACK;
        p->physics.isAttacking = 1;
        p->physics.attackTimer = p->physics.attackDuration;
        p->physics.attackHitbox = CreateBasicAttackHitbox(p, collisionRect);
    }
    
    if (IsKeyPressed(KEY_H) && !p->physics.isAttacking) {
        // Super attack (area)
        p->state = PLAYER_STATE_ATTACK;
        p->physics.isAttacking = 1;
        p->physics.attackTimer = p->physics.attackDuration;
        p->physics.attackHitbox = CreateSuperAttackHitbox(p, collisionRect);
    }

    // Update dash cooldown
    if (p->physics.dashCooldownTimer > 0) {
        p->physics.dashCooldownTimer -= dt;
    }

    // Handle dash input
    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_SHIFT)) && 
        !p->physics.isDashing && p->physics.dashCooldownTimer <= 0) {
        
        // Only dash if we have a movement direction
        if (moveDir.x != 0.0f || moveDir.y != 0.0f) {
            p->physics.isDashing = 1;
            p->physics.dashTimer = p->physics.dashDuration;
            p->physics.dashDirection = moveDir;
            p->state = PLAYER_STATE_DASH;
        }
    }

    // Update dash if active
    if (p->physics.isDashing) {
        p->physics.dashTimer -= dt;
        if (p->physics.dashTimer > 0) {
            // Store old position for collision
            Vector2 oldDashPos = p->physics.position;
            
            // Apply dash movement
            p->physics.position.x += p->physics.dashDirection.x * p->physics.dashSpeed;
            Rectangle playerRect = GetPlayerCollisionRect(p);
            if (CheckCollisionObjects(map, playerRect, PIXEL_SCALE)) {
                p->physics.position.x = oldDashPos.x;
            }
            
            p->physics.position.y += p->physics.dashDirection.y * p->physics.dashSpeed;
            playerRect = GetPlayerCollisionRect(p);
            if (CheckCollisionObjects(map, playerRect, PIXEL_SCALE)) {
                p->physics.position.y = oldDashPos.y;
            }
        } else {
            // End dash
            p->physics.isDashing = 0;
            p->physics.dashCooldownTimer = p->physics.dashCooldown;
            p->state = PLAYER_STATE_IDLE;
        }
    }
    SelectActiveSprite(p);
    
    switch (p->state) {
        case PLAYER_STATE_IDLE:
            p->currentFrame = 0;  // Always show first frame when idle
            break;
        case PLAYER_STATE_WALK:
            if (isMoving && p->frameTime >= p->frameDelay) {
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
        case PLAYER_STATE_ATTACK:
            if (p->frameTime >= p->frameDelay) {
                p->currentFrame++;
                p->frameTime = 0.0f;
                if (p->currentFrame >= p->sprite.columns) {
                    p->currentFrame = 0;
                    p->state = PLAYER_STATE_IDLE;
                }
            }
            break;
        case PLAYER_STATE_DASH:
            // Use the walking animation frames for dash
            if (p->frameTime >= p->frameDelay) {
                p->currentFrame = (p->currentFrame + 1) % p->sprite.columns;
                p->frameTime = 0.0f;
            }
            break;
    }
    
    // Update attack timer
    if (p->physics.isAttacking) {
        p->physics.attackTimer -= dt;
        if (p->physics.attackTimer <= 0) {
            p->physics.isAttacking = false;
            p->state = PLAYER_STATE_IDLE;
        }
    }
    
    // Update hit flash timer
    if (p->physics.hitFlashTimer > 0) {
        p->physics.hitFlashTimer -= dt;
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
    
    // Draw debug collision and attack boxes
    Rectangle collisionRect = GetPlayerCollisionRect(p);
    Color boxColor = p->physics.hitFlashTimer > 0 ? p->physics.hitFlashColor : GREEN;
    DrawRectangleLines((int)collisionRect.x, (int)collisionRect.y, 
                       (int)collisionRect.width, (int)collisionRect.height, boxColor);
    
    if (p->physics.isAttacking) {
        DrawRectangleLines((int)p->physics.attackHitbox.x, (int)p->physics.attackHitbox.y,
                          (int)p->physics.attackHitbox.width, (int)p->physics.attackHitbox.height, RED);
    }

    // Add dash effect
    if (p->physics.isDashing) {
        // Draw a trail effect or something similar
        Color dashColor = (Color){255, 255, 255, 128}; // Semi-transparent white
        Vector2 trailOffset = {
            -p->physics.dashDirection.x * 20.0f,
            -p->physics.dashDirection.y * 20.0f
        };
        
        Rectangle trailDestRec = destRec;
        trailDestRec.x += trailOffset.x;
        trailDestRec.y += trailOffset.y;
        
        DrawTexturePro(p->sprite.texture, srcRec, trailDestRec, 
                      (Vector2){0, 0}, 0.0f, dashColor);
    }
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

void PlayerTakeDamage(Player* p, int damage) {
    p->physics.currentHealth -= damage;
    if (p->physics.currentHealth < 0) {
        p->physics.currentHealth = 0;
    }
    // Trigger hit flash effect
    p->physics.hitFlashTimer = 0.2f;  // 0.2 seconds of flash
    p->physics.hitFlashColor = RED;
}

int IsPlayerAlive(const Player* p) {
    return p->physics.currentHealth > 0 ? 1 : 0;
}
