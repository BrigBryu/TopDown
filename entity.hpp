#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "raylib.h"
#include "animation.hpp"

class Entity {
    public:
        // Constructor and destructor
        Entity(Vector2 position, float scale, Animation* idleAnimation, Animation* walkingAnimations[8]);
        ~Entity();

        //Physics
        Vector2 position;
        Vector2 velocity;
        float speed;
        float scale;
        Rectangle hitbox;

        //Behavior
        void Update(float deltaTime);
        void Draw();
        void OnCollision(Entity* other);
    private:
        void renderEntityDebug();

        //Sprite
        Animation* idleAnimation;
        Animation* walkingAnimations[8];
};

#endif