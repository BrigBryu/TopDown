#include "monster.h"
#include <stdlib.h>

// Helper function to set up common monster properties
static void InitializeMonsterData(Entity* monster, MonsterType type, int maxHealth, 
                                float attackDamage, float attackRange, float detectionRange) {
    MonsterData* data = (MonsterData*)monster->data;
    MonsterStats* stats = (MonsterStats*)malloc(sizeof(MonsterStats));
    
    stats->maxHealth = maxHealth;
    stats->currentHealth = maxHealth;
    stats->attackDamage = attackDamage;
    stats->attackRange = attackRange;
    stats->detectionRange = detectionRange;
    stats->state = MONSTER_STATE_IDLE;
    stats->type = type;
    
    // Store stats in the monster's data
    data->stats = stats;
}

Entity* CreateSlime(Vector2 position, float scale) {
    Entity* slime = CreateBasicMonster(position, scale, NULL);
    InitializeMonsterData(slime, MONSTER_TYPE_SLIME, 3, 1.0f, 32.0f, 100.0f);
    return slime;
}

Entity* CreateBat(Vector2 position, float scale) {
    Entity* bat = CreateAggressiveMonster(position, scale, NULL);
    InitializeMonsterData(bat, MONSTER_TYPE_BAT, 2, 1.0f, 16.0f, 150.0f);
    return bat;
}

Entity* CreateSkeleton(Vector2 position, float scale) {
    Entity* skeleton = CreateAggressiveMonster(position, scale, NULL);
    InitializeMonsterData(skeleton, MONSTER_TYPE_SKELETON, 5, 2.0f, 48.0f, 200.0f);
    return skeleton;
}

void MonsterTakeDamage(Entity* monster, int damage) {
    MonsterData* data = (MonsterData*)monster->data;
    MonsterStats* stats = (MonsterStats*)data->stats;
    
    stats->currentHealth -= damage;
    if (stats->currentHealth <= 0) {
        stats->currentHealth = 0;
        stats->state = MONSTER_STATE_DEAD;
        monster->isAlive = false;
    } else {
        stats->state = MONSTER_STATE_HURT;
    }
}

bool IsMonsterAlive(const Entity* monster) {
    MonsterData* data = (MonsterData*)monster->data;
    MonsterStats* stats = (MonsterStats*)data->stats;
    return stats->currentHealth > 0;
}

MonsterState GetMonsterState(const Entity* monster) {
    MonsterData* data = (MonsterData*)monster->data;
    MonsterStats* stats = (MonsterStats*)data->stats;
    return stats->state;
}

void SetMonsterState(Entity* monster, MonsterState state) {
    MonsterData* data = (MonsterData*)monster->data;
    MonsterStats* stats = (MonsterStats*)data->stats;
    stats->state = state;
} 