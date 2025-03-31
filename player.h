#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "tiled_loader.h"

typedef struct Player Player;

typedef Rectangle (*AttackHitboxFn)(const Player* p, Rectangle collisionRect);
typedef void (*DamageFn)(struct Entity* monster, int damage);

typedef enum {
    PLAYER_STATE_IDLE = 0,
    PLAYER_STATE_WALK,
    PLAYER_STATE_ATTACK,
    PLAYER_STATE_ACTION1,
    PLAYER_STATE_ACTION2,
    PLAYER_STATE_ACTION3,
    PLAYER_STATE_DASH
} PlayerState;

typedef struct PlayerSprite {
    Texture2D texture;
    int rows;//rows in sheet
    int columns;//cols in sheet
    int frameWidth;//texture.width / columns
    int frameHeight;//texture.height / rows
} PlayerSprite;

typedef enum {
    ATTACK_BASIC,
    ATTACK_STRONG,
    ATTACK_SUPER
} AttackType;

typedef struct PlayerPhysics {
    Vector2 position;
    float scale;// How big to draw
    float speed; //moce speed
    float collisionShrinkFactor;//Shrink factor for collision box
    Rectangle attackHitbox;  // Add attack hitbox
    int isAttacking;       // Track attack state
    float attackDuration;   // How long the attack lasts
    float attackTimer;      // Current attack time
    Color hitFlashColor;    // Color to flash when hit
    float hitFlashTimer;    // Timer for hit flash effect
    AttackHitboxFn createAttackHitbox;  // Function pointer for creating attack hitbox
    int maxHealth;         // Maximum health points
    int currentHealth;     // Current health points
    float attackDamage;    // Damage dealt to monsters
    Vector2 lastCursorPos;    // Store cursor position for basic attack
    float superAttackRadius;  // Radius for super attack
    int isDashing;           // Track dash state
    float dashTimer;         // How long the dash lasts
    float dashDuration;      // Maximum dash duration
    float dashSpeed;         // Speed during dash
    float dashCooldown;      // Time between dashes
    float dashCooldownTimer; // Current cooldown timer
    Vector2 dashDirection;   // Direction of the dash
} PlayerPhysics;

// Now define the actual Player struct
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

// Add these function declarations at the bottom
void PlayerTakeDamage(Player* p, int damage);
int IsPlayerAlive(const Player* p);

#endif