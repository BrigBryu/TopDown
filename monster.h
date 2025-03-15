#ifndef MONSTER_H
#define MONSTER_H

#include "entity.h"
#include "monster_types.h"

// Monster creation functions
Entity* CreateSlime(Vector2 position, float scale);
Entity* CreateBat(Vector2 position, float scale);
Entity* CreateSkeleton(Vector2 position, float scale);

// Monster behavior functions
void MonsterTakeDamage(Entity* monster, int damage);
bool IsMonsterAlive(const Entity* monster);
MonsterState GetMonsterState(const Entity* monster);
void SetMonsterState(Entity* monster, MonsterState state);

#endif 