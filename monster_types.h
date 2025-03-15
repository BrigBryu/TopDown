#ifndef MONSTER_TYPES_H
#define MONSTER_TYPES_H

// Monster states
typedef enum {
    MONSTER_STATE_IDLE,
    MONSTER_STATE_PATROL,
    MONSTER_STATE_CHASE,
    MONSTER_STATE_ATTACK,
    MONSTER_STATE_HURT,
    MONSTER_STATE_DEAD
} MonsterState;

// Monster types
typedef enum {
    MONSTER_TYPE_SLIME,
    MONSTER_TYPE_BAT,
    MONSTER_TYPE_SKELETON,
} MonsterType;

// Monster stats
typedef struct MonsterStats {
    int maxHealth;
    int currentHealth;
    float attackDamage;
    float attackRange;
    float detectionRange;
    MonsterState state;
    MonsterType type;
} MonsterStats;

#endif 