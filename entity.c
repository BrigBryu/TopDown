#include "entity.h"
#include "constants.h"  // Add this for PIXEL_SCALE
#include <stdlib.h>
#include <math.h>
#include "map_manager.h"

// Define the global movement control variable
int ENTITIES_CAN_MOVE = 1;

void InitEntitySprite(EntitySprite* sprite, const char* texturePath, int rows, int columns, float frameDelay) {
    sprite->texture = LoadTexture(texturePath);
    sprite->rows = rows;
    sprite->columns = columns;
    sprite->frameWidth = sprite->texture.width / columns;
    sprite->frameHeight = sprite->texture.height / rows;
    sprite->currentFrame = 0;
    sprite->frameTime = 0;
    sprite->frameDelay = frameDelay;
    sprite->currentRow = 0;
}

void UnloadEntitySprite(EntitySprite* sprite) {
    if (sprite->texture.id != 0) {
        UnloadTexture(sprite->texture);
        sprite->texture.id = 0;
    }
}

Rectangle GetEntityCollisionRect(const Entity* entity) {
    float fullW = entity->sprite.frameWidth * entity->physics.scale;
    float fullH = entity->sprite.frameHeight * entity->physics.scale;
    float collW = fullW / entity->physics.collisionShrinkFactor;
    float collH = fullH / entity->physics.collisionShrinkFactor;
    float offsetX = (fullW - collW) / 2.0f;
    float offsetY = (fullH - collH) / 2.0f;
    return (Rectangle){ 
        entity->physics.position.x + offsetX, 
        entity->physics.position.y + offsetY, 
        collW, 
        collH 
    };
}

// Basic monster behavior: moves randomly
void UpdateBasicMonster(Entity* entity, GameMap* map, float dt) {
    MonsterData* data = (MonsterData*)entity->data;
    
    // Update animation regardless of movement
    entity->sprite.frameTime += dt;
    if (entity->sprite.frameTime >= entity->sprite.frameDelay) {
        entity->sprite.currentFrame = (entity->sprite.currentFrame + 1) % entity->sprite.columns;
        entity->sprite.frameTime = 0;
    }
    
    // Only update position if movement is enabled
    if (ENTITIES_CAN_MOVE) {
        data->moveTimer += dt;
        if (data->moveTimer >= data->moveInterval) {
            data->moveDirection.x = (float)(rand() % 3 - 1);
            data->moveDirection.y = (float)(rand() % 3 - 1);
            data->moveTimer = 0;
        }
        
        Vector2 oldPos = entity->physics.position;
        
        // Move monster
        entity->physics.position.x += data->moveDirection.x * entity->physics.speed * dt;
        entity->physics.position.y += data->moveDirection.y * entity->physics.speed * dt;
        
        // Check collision with map
        Rectangle entityRect = GetEntityCollisionRect(entity);
        for (int i = 0; i < map->collisionLayer.count; i++) {
            Polygon poly = map->collisionLayer.polygons[i];
            if (poly.pointCount < 2) continue;
            
            Vector2 worldPoly[128];
            for (int j = 0; j < poly.pointCount; j++) {
                worldPoly[j].x = poly.points[j].x * PIXEL_SCALE;
                worldPoly[j].y = poly.points[j].y * PIXEL_SCALE;
            }
            
            if (CheckCollisionPolyRectangle(worldPoly, poly.pointCount, entityRect)) {
                entity->physics.position = oldPos;
                data->moveTimer = data->moveInterval; // Force new direction
                break;
            }
        }
    }
    
    // Update timers
    if (entity->physics.isAttacking) {
        entity->physics.attackTimer -= dt;
        if (entity->physics.attackTimer <= 0) {
            entity->physics.isAttacking = false;
        }
    }
    
    if (entity->physics.hitFlashTimer > 0) {
        entity->physics.hitFlashTimer -= dt;
    }
}

// Aggressive monster behavior: moves in straight lines, changes direction on collision
void UpdateAggressiveMonster(Entity* entity, GameMap* map, float dt) {
    MonsterData* data = (MonsterData*)entity->data;
    
    // Update animation regardless of movement
    entity->sprite.frameTime += dt;
    if (entity->sprite.frameTime >= entity->sprite.frameDelay) {
        entity->sprite.currentFrame = (entity->sprite.currentFrame + 1) % entity->sprite.columns;
        entity->sprite.frameTime = 0;
    }
    
    // Only update position if movement is enabled
    if (ENTITIES_CAN_MOVE) {
        // Store old position
        Vector2 oldPos = entity->physics.position;
        
        // Move monster
        entity->physics.position.x += data->moveDirection.x * entity->physics.speed * dt;
        entity->physics.position.y += data->moveDirection.y * entity->physics.speed * dt;
        
        // Check collision and bounce
        Rectangle entityRect = GetEntityCollisionRect(entity);
        for (int i = 0; i < map->collisionLayer.count; i++) {
            Polygon poly = map->collisionLayer.polygons[i];
            if (poly.pointCount < 2) continue;
            
            Vector2 worldPoly[128];
            for (int j = 0; j < poly.pointCount; j++) {
                worldPoly[j].x = poly.points[j].x * entity->physics.scale;
                worldPoly[j].y = poly.points[j].y * entity->physics.scale;
            }
            
            if (CheckCollisionRecs(entityRect, (Rectangle){
                worldPoly[0].x, worldPoly[0].y,
                worldPoly[1].x - worldPoly[0].x,
                worldPoly[1].y - worldPoly[0].y
            })) {
                entity->physics.position = oldPos;
                data->moveDirection.x *= -1;
                data->moveDirection.y *= -1;
                break;
            }
        }
    }
    
    // Update attack timer
    if (entity->physics.isAttacking) {
        entity->physics.attackTimer -= dt;
        if (entity->physics.attackTimer <= 0) {
            entity->physics.isAttacking = false;
        }
    }
    
    // Update hit flash timer
    if (entity->physics.hitFlashTimer > 0) {
        entity->physics.hitFlashTimer -= dt;
    }
}

void DrawMonster(Entity* entity) {
    Rectangle srcRec = {
        entity->sprite.currentFrame * entity->sprite.frameWidth,
        entity->sprite.currentRow * entity->sprite.frameHeight,
        entity->sprite.frameWidth,
        entity->sprite.frameHeight
    };
    
    Rectangle destRec = {
        entity->physics.position.x,
        entity->physics.position.y,
        entity->sprite.frameWidth * entity->physics.scale,
        entity->sprite.frameHeight * entity->physics.scale
    };
    
    DrawTexturePro(entity->sprite.texture, srcRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
    
    // Draw debug collision box
    #if DEBUG_DRAW_ENTITY_COLLISION
        Rectangle collisionRect = GetEntityCollisionRect(entity);
        Color boxColor = entity->physics.hitFlashTimer > 0 ? 
                        entity->physics.hitFlashColor : GREEN;
        DrawRectangleLines(
            (int)collisionRect.x, 
            (int)collisionRect.y, 
            (int)collisionRect.width, 
            (int)collisionRect.height, 
            boxColor
        );
    #endif

    // Draw attack hitbox if attacking
    #if DEBUG_DRAW_ATTACK_HITBOX
        if (entity->physics.isAttacking) {
            DrawRectangleLines(
                (int)entity->physics.attackHitbox.x,
                (int)entity->physics.attackHitbox.y,
                (int)entity->physics.attackHitbox.width,
                (int)entity->physics.attackHitbox.height,
                RED
            );
        }
    #endif
}

void MonsterOnCollision(Entity* entity, Entity* other) {
    if (other->type == ENTITY_TYPE_PLAYER) {
        // Handle player collision
        MonsterData* data = (MonsterData*)entity->data;
        data->health--; // Example: reduce monster health on player collision
        if (data->health <= 0) {
            entity->isAlive = false;
        }
    }
}

Entity* CreateBasicMonster(Vector2 position, float scale, const char* texturePath) {
    Entity* monster = (Entity*)malloc(sizeof(Entity));
    
    // Initialize with empty sprite (texture will be set later if needed)
    monster->sprite = (EntitySprite){0};
    if (texturePath) {
        InitEntitySprite(&monster->sprite, texturePath, 4, 4, 0.1f);
    }
    
    monster->physics.position = position;
    monster->physics.scale = scale;
    monster->physics.speed = 50.0f;
    monster->physics.collisionShrinkFactor = 3.0f;
    
    monster->type = ENTITY_TYPE_MONSTER_BASIC;
    monster->active = true;
    monster->isAlive = true;
    
    // Set behavior functions
    monster->update = UpdateBasicMonster;
    monster->draw = DrawMonster;
    monster->onCollision = MonsterOnCollision;
    
    // Initialize monster data
    MonsterData* data = (MonsterData*)malloc(sizeof(MonsterData));
    data->moveTimer = 0;
    data->moveInterval = 2.0f;
    data->moveDirection = (Vector2){1, 0};
    data->health = 3;
    monster->data = data;
    
    monster->physics.isAttacking = false;
    monster->physics.attackDuration = 0.3f;
    monster->physics.attackTimer = 0;
    monster->physics.hitFlashTimer = 0;
    monster->physics.hitFlashColor = WHITE;
    
    return monster;
}

Entity* CreateAggressiveMonster(Vector2 position, float scale, const char* texturePath) {
    Entity* monster = (Entity*)malloc(sizeof(Entity));
    
    // Initialize with empty sprite (texture will be set later if needed)
    monster->sprite = (EntitySprite){0};
    if (texturePath) {
        InitEntitySprite(&monster->sprite, texturePath, 4, 4, 0.1f);
    }
    
    monster->physics.position = position;
    monster->physics.scale = scale;
    monster->physics.speed = 100.0f; // Faster than basic monster
    monster->physics.collisionShrinkFactor = 3.0f;
    
    monster->type = ENTITY_TYPE_MONSTER_AGGRESSIVE;
    monster->active = true;
    monster->isAlive = true;
    
    // Set behavior functions
    monster->update = UpdateAggressiveMonster;
    monster->draw = DrawMonster;
    monster->onCollision = MonsterOnCollision;
    
    // Initialize monster data
    MonsterData* data = (MonsterData*)malloc(sizeof(MonsterData));
    data->moveTimer = 0;
    data->moveInterval = 1.0f;
    data->moveDirection = (Vector2){1, 0};
    data->health = 5;
    monster->data = data;
    
    monster->physics.isAttacking = false;
    monster->physics.attackDuration = 0.3f;
    monster->physics.attackTimer = 0;
    monster->physics.hitFlashTimer = 0;
    monster->physics.hitFlashColor = WHITE;
    
    return monster;
}

void DestroyEntity(Entity* entity) {
    if (entity) {
        UnloadEntitySprite(&entity->sprite);
        free(entity->data);
        free(entity);
    }
}

void RenderEntityDebug(const Entity* entity) {
    #if DEBUG_DRAW_ENTITY_COLLISION
        // Draw collision box
        Rectangle collisionRect = GetEntityCollisionRect(entity);
        Color boxColor = entity->physics.hitFlashTimer > 0 ? 
                        entity->physics.hitFlashColor : GREEN;
        DrawRectangleLines(
            (int)collisionRect.x, 
            (int)collisionRect.y, 
            (int)collisionRect.width, 
            (int)collisionRect.height, 
            boxColor
        );
    #endif

    #if DEBUG_DRAW_ATTACK_HITBOX
        // Draw attack hitbox if attacking
        if (entity->physics.isAttacking) {
            DrawRectangleLines(
                (int)entity->physics.attackHitbox.x,
                (int)entity->physics.attackHitbox.y,
                (int)entity->physics.attackHitbox.width,
                (int)entity->physics.attackHitbox.height,
                RED
            );
        }
    #endif
}

void EntityStartAttack(Entity* entity) {
    entity->physics.isAttacking = true;
    entity->physics.attackTimer = entity->physics.attackDuration;
    
    // Update attack hitbox position based on entity facing direction
    Rectangle collisionRect = GetEntityCollisionRect(entity);
    float attackWidth = collisionRect.width * 1.5f;
    float attackHeight = collisionRect.height * 1.5f;
    
    // Position the attack hitbox based on the entity's current row (direction)
    switch (entity->sprite.currentRow) {
        case 0: // Down
            entity->physics.attackHitbox = (Rectangle){
                collisionRect.x - attackWidth/4,
                collisionRect.y + collisionRect.height,
                attackWidth,
                attackHeight/2
            };
            break;
        case 1: // Up
            entity->physics.attackHitbox = (Rectangle){
                collisionRect.x - attackWidth/4,
                collisionRect.y - attackHeight/2,
                attackWidth,
                attackHeight/2
            };
            break;
        case 2: // Left
            entity->physics.attackHitbox = (Rectangle){
                collisionRect.x - attackWidth/2,
                collisionRect.y - attackHeight/4,
                attackWidth/2,
                attackHeight
            };
            break;
        case 3: // Right
            entity->physics.attackHitbox = (Rectangle){
                collisionRect.x + collisionRect.width,
                collisionRect.y - attackHeight/4,
                attackWidth/2,
                attackHeight
            };
            break;
    }
}

void EntityTakeHit(Entity* entity) {
    entity->physics.hitFlashTimer = 0.2f; // Flash for 0.2 seconds
    entity->physics.hitFlashColor = RED;
}

int CheckCollisionPolyRectangle(Vector2* poly, int polyCount, Rectangle rec) {
    Vector2 corners[4] = {
        { rec.x, rec.y },
        { rec.x + rec.width, rec.y },
        { rec.x + rec.width, rec.y + rec.height },
        { rec.x, rec.y + rec.height }
    };
    
    // Check if any corner of the rectangle is inside the polygon
    for (int i = 0; i < 4; i++) {
        if (CheckCollisionPointPoly(corners[i], poly, polyCount)) {
            return 1;
        }
    }
    
    // Check if any polygon edge intersects with any rectangle edge
    for (int i = 0; i < polyCount; i++) {
        Vector2 p1 = poly[i];
        Vector2 p2 = poly[(i + 1) % polyCount];
        
        for (int j = 0; j < 4; j++) {
            Vector2 r1 = corners[j];
            Vector2 r2 = corners[(j + 1) % 4];
            
            if (CheckCollisionLines(p1, p2, r1, r2, NULL)) {
                return 1;
            }
        }
    }
    
    return 0;
} 