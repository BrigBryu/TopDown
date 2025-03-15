#include "entity_manager.h"
#include <stdlib.h>
#include <math.h>

EntityManager* CreateEntityManager(void) {
    EntityManager* manager = (EntityManager*)malloc(sizeof(EntityManager));
    if (manager) {
        manager->count = 0;
        manager->player = NULL;
        for (int i = 0; i < MAX_ENTITIES; i++) {
            manager->entities[i] = NULL;
        }
    }
    return manager;
}

void DestroyEntityManager(EntityManager* manager) {
    if (manager) {
        for (int i = 0; i < manager->count; i++) {
            if (manager->entities[i]) {
                DestroyEntity(manager->entities[i]);
            }
        }
        free(manager);
    }
}

int AddEntity(EntityManager* manager, Entity* entity) {
    if (manager->count >= MAX_ENTITIES) {
        return -1;
    }
    
    int index = manager->count;
    manager->entities[index] = entity;
    manager->count++;
    
    // If this is a player entity, store the reference
    if (entity->type == ENTITY_TYPE_PLAYER) {
        manager->player = entity;
    }
    
    return index;
}

void RemoveEntity(EntityManager* manager, int index) {
    if (index < 0 || index >= manager->count) return;
    
    // If removing player, clear the reference
    if (manager->entities[index] == manager->player) {
        manager->player = NULL;
    }
    
    DestroyEntity(manager->entities[index]);
    
    // Shift remaining entities
    for (int i = index; i < manager->count - 1; i++) {
        manager->entities[i] = manager->entities[i + 1];
    }
    
    manager->count--;
    manager->entities[manager->count] = NULL;
}

void RemoveDeadEntities(EntityManager* manager) {
    for (int i = manager->count - 1; i >= 0; i--) {
        if (!manager->entities[i]->isAlive) {
            RemoveEntity(manager, i);
        }
    }
}

void UpdateEntities(EntityManager* manager, GameMap* map, float dt) {
    for (int i = 0; i < manager->count; i++) {
        Entity* entity = manager->entities[i];
        if (entity->active && entity->isAlive && entity->update) {
            entity->update(entity, map, dt);
        }
    }
}

void DrawEntities(EntityManager* manager) {
    // Sort entities by Y position for proper depth
    // This is a simple bubble sort - could be optimized
    for (int i = 0; i < manager->count - 1; i++) {
        for (int j = 0; j < manager->count - i - 1; j++) {
            if (manager->entities[j]->physics.position.y > 
                manager->entities[j + 1]->physics.position.y) {
                Entity* temp = manager->entities[j];
                manager->entities[j] = manager->entities[j + 1];
                manager->entities[j + 1] = temp;
            }
        }
    }
    
    // Draw all entities
    for (int i = 0; i < manager->count; i++) {
        Entity* entity = manager->entities[i];
        if (entity->active && entity->isAlive && entity->draw) {
            entity->draw(entity);
        }
    }
}

Entity* GetEntityAt(EntityManager* manager, Vector2 position) {
    for (int i = 0; i < manager->count; i++) {
        Entity* entity = manager->entities[i];
        if (entity->active && entity->isAlive) {
            Rectangle rect = GetEntityCollisionRect(entity);
            if (CheckCollisionPointRec(position, rect)) {
                return entity;
            }
        }
    }
    return NULL;
}

Entity** GetEntitiesInRange(EntityManager* manager, Vector2 position, float range, int* count) {
    static Entity* results[MAX_ENTITIES];
    *count = 0;
    
    for (int i = 0; i < manager->count; i++) {
        Entity* entity = manager->entities[i];
        if (entity->active && entity->isAlive) {
            float dx = entity->physics.position.x - position.x;
            float dy = entity->physics.position.y - position.y;
            float distance = sqrt(dx * dx + dy * dy);
            
            if (distance <= range) {
                results[*count] = entity;
                (*count)++;
            }
        }
    }
    
    return results;
}

Entity** GetEntitiesByType(EntityManager* manager, int type, int* count) {
    static Entity* results[MAX_ENTITIES];
    *count = 0;
    
    for (int i = 0; i < manager->count; i++) {
        Entity* entity = manager->entities[i];
        if (entity->active && entity->isAlive && entity->type == type) {
            results[*count] = entity;
            (*count)++;
        }
    }
    
    return results;
}

void CheckCollisions(EntityManager* manager) {
    for (int i = 0; i < manager->count; i++) {
        Entity* e1 = manager->entities[i];
        if (!e1->active || !e1->isAlive) continue;
        
        Rectangle r1 = GetEntityCollisionRect(e1);
        
        for (int j = i + 1; j < manager->count; j++) {
            Entity* e2 = manager->entities[j];
            if (!e2->active || !e2->isAlive) continue;
            
            Rectangle r2 = GetEntityCollisionRect(e2);
            
            // Check attack hitbox collisions
            if (e1->physics.isAttacking) {
                if (CheckCollisionRecs(e1->physics.attackHitbox, r2)) {
                    EntityTakeHit(e2);
                }
            }
            
            if (e2->physics.isAttacking) {
                if (CheckCollisionRecs(e2->physics.attackHitbox, r1)) {
                    EntityTakeHit(e1);
                }
            }
            
            // Regular collision handling
            if (CheckCollisionRecs(r1, r2)) {
                if (e1->onCollision) e1->onCollision(e1, e2);
                if (e2->onCollision) e2->onCollision(e2, e1);
            }
        }
    }
} 