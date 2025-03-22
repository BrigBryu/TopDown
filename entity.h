#ifndef ENTITY_H
#define ENTITY_H

#include "raylib.h"
#include "tiled_loader.h"
#include "monster_types.h"

// Forward declaration to avoid circular dependency
typedef struct Entity Entity;

// Add debug rendering control
#define DEBUG_DRAW_ENTITY_COLLISION 1
#define DEBUG_DRAW_ATTACK_HITBOX 1

typedef struct EntitySprite {
    Texture2D texture;
    int rows;
    int columns;
    int frameWidth;
    int frameHeight;
    int currentFrame;
    float frameTime;
    float frameDelay;
    int currentRow;
} EntitySprite;

typedef struct EntityPhysics {
    Vector2 position;
    float scale;
    float speed;
    float collisionShrinkFactor;
    Rectangle attackHitbox;  // Add attack hitbox
    int isAttacking;       // Changed from bool to int
    float attackDuration;   // How long the attack lasts
    float attackTimer;      // Current attack time
    Color hitFlashColor;    // Color to flash when hit
    float hitFlashTimer;    // Timer for hit flash effect
} EntityPhysics;

// Function pointer types for entity behaviors
typedef void (*UpdateFn)(Entity* entity, GameMap* map, float dt);
typedef void (*DrawFn)(Entity* entity);
typedef void (*OnCollisionFn)(Entity* entity, Entity* other);

typedef struct Entity {
    EntitySprite sprite;
    EntityPhysics physics;
    
    // Behavior function pointers
    UpdateFn update;
    DrawFn draw;
    OnCollisionFn onCollision;
    
    // Entity type for identification
    int type;
    
    // Entity state
    int active;
    int isAlive;
    
    // Custom data pointer for specific entity types
    void* data;
} Entity;

// Monster specific data
typedef struct MonsterData {
    float moveTimer;
    float moveInterval;
    Vector2 moveDirection;
    int health;
    MonsterStats* stats;
} MonsterData;

// Entity type identifiers
enum EntityType {
    ENTITY_TYPE_NONE = 0,
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_MONSTER_BASIC,
    ENTITY_TYPE_MONSTER_AGGRESSIVE,
    // Add more entity types as needed
};

// Global movement control
extern int ENTITIES_CAN_MOVE;

// Basic entity functions
void InitEntitySprite(EntitySprite* sprite, const char* texturePath, int rows, int columns, float frameDelay);
void UnloadEntitySprite(EntitySprite* sprite);
Rectangle GetEntityCollisionRect(const Entity* entity);

// Monster creation functions
Entity* CreateBasicMonster(Vector2 position, float scale, const char* texturePath);
Entity* CreateAggressiveMonster(Vector2 position, float scale, const char* texturePath);

// Monster behavior functions (examples)
void UpdateBasicMonster(Entity* entity, GameMap* map, float dt);
void UpdateAggressiveMonster(Entity* entity, GameMap* map, float dt);
void DrawMonster(Entity* entity);
void MonsterOnCollision(Entity* entity, Entity* other);

// Entity management
void DestroyEntity(Entity* entity);

// Add these function declarations
void RenderEntityDebug(const Entity* entity);
void EntityStartAttack(Entity* entity);
void EntityTakeHit(Entity* entity);

// Helper function for collision detection
int CheckCollisionPolyRectangle(Vector2* poly, int polyCount, Rectangle rec);

#endif 
