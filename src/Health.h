#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generational handle — index(16b) | generation(16b)
typedef u32 HealthID;
#define HEALTH_ID_INVALID ((HealthID)0xFFFFFFFFu)

#define HEALTH_MANAGER_CAPACITY 1024

typedef struct
{
    f32 hp[HEALTH_MANAGER_CAPACITY];
    f32 hpMax[HEALTH_MANAGER_CAPACITY];
    u16 generation[HEALTH_MANAGER_CAPACITY];
    b8  alive[HEALTH_MANAGER_CAPACITY];
    u32 freeList[HEALTH_MANAGER_CAPACITY];
    u32 freeCount;
} HealthManager;

// Damage queue — decouples detection from effect application so callbacks
// can queue damage without mutating state mid-physics-step.
#define DAMAGE_QUEUE_CAPACITY 512

typedef struct
{
    HealthID ids[DAMAGE_QUEUE_CAPACITY];
    f32      amounts[DAMAGE_QUEUE_CAPACITY];
    u32      count;
} DamageQueue;

DSAPI void       healthManagerCreate(HealthManager *mgr);
DSAPI HealthID   healthRegister(HealthManager *mgr, f32 maxHp);
DSAPI void       healthUnregister(HealthManager *mgr, HealthID id);
DSAPI f32        healthGet(const HealthManager *mgr, HealthID id);
DSAPI b8         healthIsAlive(const HealthManager *mgr, HealthID id);
DSAPI void       healthApplyDamage(HealthManager *mgr, HealthID id, f32 damage);

DSAPI void       damageEnqueue(DamageQueue *q, HealthID id, f32 damage);
DSAPI void       damageFlush(DamageQueue *q, HealthManager *mgr);

#ifdef __cplusplus
}
#endif
