#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include "entity.h"
#include "monster.h"
#include "tiled_loader.h"

#define MAX_ENTITIES 100

typedef struct {
    Entity* entities[MAX_ENTITIES];
    int count;
    Entity* player; // Reference to player entity
} EntityManager;

// Creation and destruction
EntityManager* CreateEntityManager(void);
void DestroyEntityManager(EntityManager* manager);

// Entity management
int AddEntity(EntityManager* manager, Entity* entity);
void RemoveEntity(EntityManager* manager, int index);
void RemoveDeadEntities(EntityManager* manager);

// Update and render
void UpdateEntities(EntityManager* manager, GameMap* map, float dt);
void DrawEntities(EntityManager* manager);

// Entity queries
Entity* GetEntityAt(EntityManager* manager, Vector2 position);
Entity** GetEntitiesInRange(EntityManager* manager, Vector2 position, float range, int* count);
Entity** GetEntitiesByType(EntityManager* manager, int type, int* count);

// Collision detection
void CheckCollisions(EntityManager* manager);

#endif 