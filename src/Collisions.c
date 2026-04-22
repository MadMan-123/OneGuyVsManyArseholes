#include "Collisions.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Player.h"

HealthManager g_healthManager;
DamageQueue   g_damageQueue;

void collisionsInit(void)
{
    healthManagerCreate(&g_healthManager);
    memset(&g_damageQueue, 0, sizeof(g_damageQueue));
}

// Called when a bullet touches anything (self = bullet entity)
void onBulletCollideEnter(ContactInfo *info)
{
    // Despawn the bullet
    archetypePoolDespawn(info->self.arch, info->self.poolIdx);

    // If the other archetype is the enemy, queue damage on its health ID
    Archetype *enemyArch = enemyGetArchetype();
    if (!enemyArch || info->other.arch != enemyArch) return;

    void **fields = getArchetypeFields(info->other.arch,
                                       info->other.poolIdx / info->other.arch->chunkCapacity);
    if (!fields) return;
    u32 localIdx = info->other.poolIdx % info->other.arch->chunkCapacity;
    HealthID hid = ((u32 *)fields[EF_HEALTH_ID])[localIdx];
    damageEnqueue(&g_damageQueue, hid, BULLET_DAMAGE);
}

// Called when an enemy touches anything (self = enemy entity)
void onEnemyCollideEnter(ContactInfo *info)
{
    extern Archetype g_playerArch;
    if (info->other.arch != &g_playerArch) return;

    // Check attack cooldown
    void **fields = getArchetypeFields(info->self.arch,
                                       info->self.poolIdx / info->self.arch->chunkCapacity);
    if (!fields) return;
    u32 localIdx = info->self.poolIdx % info->self.arch->chunkCapacity;
    f32 *cd = (f32 *)fields[EF_ATTACK_CD];
    if (cd[localIdx] > 0.0f) return;
    cd[localIdx] = ENEMY_MELEE_CD;

    // Queue damage on the player's health ID
    void **pFields = getArchetypeFields(info->other.arch, 0);
    if (!pFields) return;
    HealthID hid = ((u32 *)pFields[PF_HEALTH_ID])[0];
    damageEnqueue(&g_damageQueue, hid, ENEMY_MELEE_DAMAGE);
}
