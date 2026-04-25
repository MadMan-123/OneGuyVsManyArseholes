#include "Collisions.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Player.h"
#include "GameAudio.h"

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
    // Guard against re-entry if the bullet was already despawned this step
    if (!archetypePoolIsAlive(info->self.arch, info->self.poolIdx)) return;

    // Despawn the bullet
    archetypePoolDespawn(info->self.arch, info->self.poolIdx);

    // If the other archetype is the enemy, queue damage on its health ID
    Archetype *enemyArch = enemyGetArchetype();
    if (!enemyArch || info->other.arch != enemyArch) return;

    void **fields = getArchetypeFields(info->other.arch,
                                       info->other.poolIdx / info->other.arch->chunkCapacity);
    if (!fields) return;
    u32 localIdx = info->other.poolIdx % info->other.arch->chunkCapacity;
    HealthID hid = ((u32 *)fields[ENEMY_HEALTH_ID])[localIdx];
    damageEnqueue(&g_damageQueue, hid, BULLET_DAMAGE);
    gameAudioPlayEnemyHit();

    // Pain alert — enemy immediately knows player's position regardless of vision
    extern Archetype g_playerArch;
    void **pf = getArchetypeFields(&g_playerArch, 0);
    if (pf)
    {
        ((f32 *)fields[ENEMY_LAST_SEEN_X])[localIdx] = ((f32 *)pf[PLAYER_POSITION_X])[0];
        ((f32 *)fields[ENEMY_LAST_SEEN_Y])[localIdx] = ((f32 *)pf[PLAYER_POSITION_Y])[0];
        ((f32 *)fields[ENEMY_LAST_SEEN_Z])[localIdx] = ((f32 *)pf[PLAYER_POSITION_Z])[0];
        ((f32 *)fields[ENEMY_LAST_SEEN_AGE])[localIdx] = 0.0f;
    }
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
    f32 *cd = (f32 *)fields[ENEMY_ATTACK_COOLDOWN];
    if (cd[localIdx] > 0.0f) return;
    cd[localIdx] = ENEMY_MELEE_CD;

    // Queue damage on the player's health ID
    void **pFields = getArchetypeFields(info->other.arch, 0);
    if (!pFields) return;
    HealthID hid = ((u32 *)pFields[PLAYER_HEALTH_ID])[0];
    damageEnqueue(&g_damageQueue, hid, ENEMY_MELEE_DAMAGE);
    gameAudioPlayPlayerHit();
}
