#ifndef MONSTER_HPP
#define MONSTER_HPP

#include "raylib.h"
#include "entity.hpp"

enum MonsterState {
    MONSTER_STATE_IDLE,
    MONSTER_STATE_WALKING,
    MONSTER_STATE_ATTACKING,
    MONSTER_STATE_DEAD
};

enum MonsterType {
    MONSTER_TYPE_SLIME,
    MONSTER_TYPE_BAT,
    MONSTER_TYPE_SKELETON
};

class MonsterStats {
    public:
        int maxHealth;
        int currentHealth;
        float attackDamage;
        Rectangle attackHitbox;
        float detectionRange;
        MonsterType type;
        Animation* idleAnimation;
        Animation* walkingAnimations[8];
};

class Monster : public Entity {
    public:
        Monster(Vector2 position, float scale, MonsterStats* stats);
        ~Monster();

        //Monster specific data
        float moveTimer;
        float moveInterval;
        Vector2 moveDirection;
        int health;
        MonsterStats* stats;
        MonsterState state;
        
        void Update(float deltaTime);
        void Draw();
        void OnCollision(Entity* other);

    private:
};



#endif