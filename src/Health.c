#include "Health.h"

void healthManagerCreate(HealthManager *mgr)
{
    memset(mgr, 0, sizeof(HealthManager));
    for (u32 i = 0; i < HEALTH_MANAGER_CAPACITY; i++)
        mgr->freeList[i] = HEALTH_MANAGER_CAPACITY - 1 - i;
    mgr->freeCount = HEALTH_MANAGER_CAPACITY;
}

static inline u32 hid_index(HealthID id)      { return id & 0xFFFFu; }
static inline u16 hid_gen(HealthID id)         { return (u16)(id >> 16); }
static inline HealthID hid_make(u32 idx, u16 gen) { return (((u32)gen) << 16) | (idx & 0xFFFFu); }

HealthID healthRegister(HealthManager *mgr, f32 maxHp)
{
    if (!mgr || mgr->freeCount == 0) return HEALTH_ID_INVALID;
    u32 idx = mgr->freeList[--mgr->freeCount];
    mgr->generation[idx]++;
    mgr->hp[idx]    = maxHp;
    mgr->hpMax[idx] = maxHp;
    mgr->alive[idx] = true;
    return hid_make(idx, mgr->generation[idx]);
}

void healthUnregister(HealthManager *mgr, HealthID id)
{
    if (!mgr || id == HEALTH_ID_INVALID) return;
    u32 idx = hid_index(id);
    if (idx >= HEALTH_MANAGER_CAPACITY) return;
    if (!mgr->alive[idx]) return;
    mgr->alive[idx] = false;
    mgr->hp[idx]    = 0.0f;
    if (mgr->freeCount < HEALTH_MANAGER_CAPACITY)
        mgr->freeList[mgr->freeCount++] = idx;
}

static inline b8 hid_valid(const HealthManager *mgr, HealthID id)
{
    if (id == HEALTH_ID_INVALID) return false;
    u32 idx = hid_index(id);
    if (idx >= HEALTH_MANAGER_CAPACITY) return false;
    return mgr->alive[idx] && mgr->generation[idx] == hid_gen(id);
}

f32 healthGet(const HealthManager *mgr, HealthID id)
{
    return hid_valid(mgr, id) ? mgr->hp[hid_index(id)] : 0.0f;
}

b8 healthIsAlive(const HealthManager *mgr, HealthID id)
{
    return hid_valid(mgr, id);
}

void healthApplyDamage(HealthManager *mgr, HealthID id, f32 damage)
{
    if (!hid_valid(mgr, id)) return;
    u32 idx = hid_index(id);
    mgr->hp[idx] -= damage;
    if (mgr->hp[idx] <= 0.0f)
    {
        mgr->hp[idx]    = 0.0f;
        mgr->alive[idx] = false;
        if (mgr->freeCount < HEALTH_MANAGER_CAPACITY)
            mgr->freeList[mgr->freeCount++] = idx;
    }
}

void damageEnqueue(DamageQueue *q, HealthID id, f32 damage)
{
    if (!q || q->count >= DAMAGE_QUEUE_CAPACITY) return;
    q->ids[q->count]     = id;
    q->amounts[q->count] = damage;
    q->count++;
}

void damageFlush(DamageQueue *q, HealthManager *mgr)
{
    if (!q || !mgr) return;
    for (u32 i = 0; i < q->count; i++)
        healthApplyDamage(mgr, q->ids[i], q->amounts[i]);
    q->count = 0;
}
