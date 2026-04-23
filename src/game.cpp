#include "game.h"
#include "Player.h"
#include "Bullet.h"
#include "Gun.h"
#include "Enemy.h"
#include "Collisions.h"
#include "AISpawn.h"
#include <math.h>

Archetype g_playerArch = {0};
Archetype g_bulletArch = {0};
Archetype g_gunArch    = {0};
Archetype g_enemyArch  = {0};
static b8        g_playerCreated = false;
static b8        g_bulletCreated = false;
static b8        g_gunCreated    = false;
static b8        g_enemyCreated  = false;

// ── Wave system ──────────────────────────────────────────────────────────────
#define MAX_SPAWN_POINTS 64
#define SPAWN_INTERVAL   1.5f    // seconds between individual spawns

static Vec3 g_spawnPoints[MAX_SPAWN_POINTS];
static u32  g_spawnPointCount  = 0;
static b8   g_spawnPointsReady = false;
static u32  g_killCount  = 0;
static u32  g_maxEnemies = 2;
static f32  g_spawnTimer = 0.0f;
static u32  g_nextSpawn  = 0;

static b8 segmentVsAABB(f32 sx, f32 sy, f32 sz,
                         f32 ex, f32 ey, f32 ez,
                         f32 cx, f32 cy, f32 cz,
                         f32 hx, f32 hy, f32 hz)
{
    f32 tMin = 0.0f, tMax = 1.0f;
    f32 d, t1, t2, tmp;
#define SLAB(S,E,C,H)                                           \
    d = (E)-(S);                                                \
    if (fabsf(d) < 1e-9f) {                                     \
        if ((S) < (C)-(H) || (S) > (C)+(H)) return false;      \
    } else {                                                     \
        t1 = ((C)-(H)-(S))/d; t2 = ((C)+(H)-(S))/d;           \
        if (t1 > t2) { tmp=t1; t1=t2; t2=tmp; }                \
        if (t1 > tMin) tMin = t1;                               \
        if (t2 < tMax) tMax = t2;                               \
        if (tMin > tMax) return false;                          \
    }
    SLAB(sx, ex, cx, hx)
    SLAB(sy, ey, cy, hy)
    SLAB(sz, ez, cz, hz)
#undef SLAB
    return true;
}

static void bulletHitScan(f32 dt)
{
    for (u32 bc = 0; bc < g_bulletArch.activeChunkCount; bc++)
    {
        void **bf = getArchetypeFields(&g_bulletArch, bc);
        if (!bf) continue;
        u32 bCount = g_bulletArch.arena[bc].count;

        b8  *bAlive = (b8  *)bf[BF_ALIVE];
        f32 *bPosX  = (f32 *)bf[BF_POS_X];
        f32 *bPosY  = (f32 *)bf[BF_POS_Y];
        f32 *bPosZ  = (f32 *)bf[BF_POS_Z];
        f32 *bVelX  = (f32 *)bf[BF_VEL_X];
        f32 *bVelY  = (f32 *)bf[BF_VEL_Y];
        f32 *bVelZ  = (f32 *)bf[BF_VEL_Z];
        f32 *bSphR  = (f32 *)bf[BF_SPHERE_R];

        for (u32 bi = 0; bi < bCount; bi++)
        {
            if (!bAlive[bi]) continue;

            f32 sx = bPosX[bi], sy = bPosY[bi], sz = bPosZ[bi];
            f32 ex = sx + bVelX[bi] * dt;
            f32 ey = sy + bVelY[bi] * dt;
            f32 ez = sz + bVelZ[bi] * dt;
            f32 br = bSphR[bi];

            u32 hitChunk = (u32)-1, hitLocal = (u32)-1;

            for (u32 ec = 0; ec < g_enemyArch.activeChunkCount && hitChunk == (u32)-1; ec++)
            {
                void **ef = getArchetypeFields(&g_enemyArch, ec);
                if (!ef) continue;
                u32 eCount = g_enemyArch.arena[ec].count;

                b8  *eAlive  = (b8  *)ef[EF_ALIVE];
                f32 *ePosX   = (f32 *)ef[EF_POS_X];
                f32 *ePosY   = (f32 *)ef[EF_POS_Y];
                f32 *ePosZ   = (f32 *)ef[EF_POS_Z];
                f32 *eOffY   = (f32 *)ef[EF_COLLIDER_OFFSET_Y];
                f32 *eHX     = (f32 *)ef[EF_HALF_X];
                f32 *eHY     = (f32 *)ef[EF_HALF_Y];
                f32 *eHZ     = (f32 *)ef[EF_HALF_Z];

                for (u32 ei = 0; ei < eCount; ei++)
                {
                    if (!eAlive[ei]) continue;
                    f32 cy = ePosY[ei] + (eOffY ? eOffY[ei] : 0.0f);
                    if (segmentVsAABB(sx, sy, sz, ex, ey, ez,
                                      ePosX[ei], cy, ePosZ[ei],
                                      eHX[ei] + br, eHY[ei] + br, eHZ[ei] + br))
                    {
                        hitChunk = ec;
                        hitLocal = ei;
                        break;
                    }
                }
            }

            if (hitChunk != (u32)-1)
            {
                archetypePoolDespawn(&g_bulletArch, bc * g_bulletArch.chunkCapacity + bi);
                void **ef = getArchetypeFields(&g_enemyArch, hitChunk);
                if (ef)
                {
                    HealthID hid = ((u32 *)ef[EF_HEALTH_ID])[hitLocal];
                    damageEnqueue(&g_damageQueue, hid, BULLET_DAMAGE);
                }
            }
        }
    }
}

#define ENEMY_MAX_HP 50.0f

static void spawnAndRegisterEnemy(Vec3 pos)
{
    u32 poolIdx = enemySpawnAt(pos);
    if (poolIdx == (u32)-1) return;

    HealthID hid = healthRegister(&g_healthManager, ENEMY_MAX_HP);
    if (hid == HEALTH_ID_INVALID) return;

    u32 chunkIdx = poolIdx / g_enemyArch.chunkCapacity;
    u32 localIdx = poolIdx % g_enemyArch.chunkCapacity;
    void **fields = getArchetypeFields(&g_enemyArch, chunkIdx);
    if (fields) ((u32 *)fields[EF_HEALTH_ID])[localIdx] = (u32)hid;
}

// Returns enemies live for cap comparisons.
static u32 countLiveEnemies(void)
{
    u32 live = 0;
    for (u32 c = 0; c < g_enemyArch.activeChunkCount; c++)
    {
        void **fields = getArchetypeFields(&g_enemyArch, c);
        if (!fields) continue;
        b8 *alive = (b8 *)fields[EF_ALIVE];
        for (u32 i = 0; i < g_enemyArch.arena[c].count; i++)
            if (alive[i]) live++;
    }
    return live;
}

// Max live enemies as a function of total kills:
//   kills  0-9  : starts at 2, +1 every 3 kills
//   kills 10-39 : +2 every 5 kills   (reaches ~15 at 39 kills)
//   kills 40+   : +5 every 10 kills
static u32 maxEnemiesFromKills(u32 kills)
{
    if (kills < 10) return 2 + kills / 3;
    if (kills < 40) return 5 + ((kills - 10) / 5) * 2;
    return               17 + ((kills - 40) / 10) * 5;
}

// Refills enemies up to g_maxEnemies, one per SPAWN_INTERVAL seconds.
// Cycles through spawn points round-robin.
static void waveUpdate(f32 dt)
{
    if (g_spawnPointCount == 0) return;
    if (countLiveEnemies() >= g_maxEnemies) return;

    g_spawnTimer -= dt;
    if (g_spawnTimer > 0.0f) return;
    g_spawnTimer = SPAWN_INTERVAL;

    Vec3 pos = g_spawnPoints[g_nextSpawn % g_spawnPointCount];
    g_nextSpawn++;
    spawnAndRegisterEnemy(pos);
}

static void setupPlayer(void)
{
    g_playerArch.flags = 0;
    FLAG_SET(g_playerArch.flags, ARCH_SINGLE);
    FLAG_SET(g_playerArch.flags, ARCH_PHYSICS_BODY);

    if (!createArchetype(&Player_layout, 1, &g_playerArch))
    { ERROR("Failed to create Player archetype"); return; }
    g_playerCreated = true;

    u64 entity = 0;
    createEntityInArchetype(&g_playerArch, &entity);

    void **fields = getArchetypeFields(&g_playerArch, 0);
    if (fields)
    {
        ((f32  *)fields[PF_POS_X])[0]       = 0.0f;
        ((f32  *)fields[PF_POS_Y])[0]       = 5.0f;
        ((f32  *)fields[PF_POS_Z])[0]       = 0.0f;
        ((Vec4 *)fields[PF_ROT])[0]         = Vec4{0, 0, 0, 1};
        ((Vec3 *)fields[PF_SCALE])[0]       = Vec3{0.8f, 1.8f, 0.8f};
        ((u32  *)fields[PF_BODY_TYPE])[0]   = PHYS_BODY_DYNAMIC;
        ((f32  *)fields[PF_MASS])[0]        = 80.0f;
        ((f32  *)fields[PF_RESTITUTION])[0] = 0.0f;
        ((f32  *)fields[PF_DAMPING])[0]     = 0.1f;
        ((f32  *)fields[PF_HALF_X])[0]      = 0.4f;
        ((f32  *)fields[PF_HALF_Y])[0]      = 0.9f;
        ((f32  *)fields[PF_HALF_Z])[0]      = 0.4f;
        ((u32  *)fields[PF_WEAPON])[0]         = 0;
        ((f32  *)fields[PF_AMMO_PISTOL])[0]    = 9.0f;
        ((f32  *)fields[PF_AMMO_AK])[0]        = 30.0f;
        ((b8   *)fields[PF_HAS_RELOADED])[0]   = true;

        HealthID hid = healthRegister(&g_healthManager, 100.0f);
        ((u32  *)fields[PF_HEALTH_ID])[0]    = (u32)hid;
    }
    playerInit();
}

static void setupBullets(void)
{
    g_bulletArch.flags = 0;
    FLAG_SET(g_bulletArch.flags, ARCH_BUFFERED);
    FLAG_SET(g_bulletArch.flags, ARCH_PHYSICS_BODY);

    if (!createArchetype(&Bullet_layout, 256, &g_bulletArch))
    { ERROR("Failed to create Bullet archetype"); return; }
    g_bulletCreated = true;

    // Triggers pass through enemies without impulse response.
    // Damage is handled by bulletHitScan (swept segment), not physics callbacks.
    archetypeSetTrigger(&g_bulletArch, true);

    bulletInit(&g_bulletArch);
}

static void setupGun(void)
{
    g_gunArch.flags = 0;

    if (!createArchetype(&Gun_layout, 2, &g_gunArch))
    { ERROR("Failed to create Gun archetype"); return; }
    g_gunCreated = true;

    u64 pistolEntity = 0, akEntity = 0;
    createEntityInArchetype(&g_gunArch, &pistolEntity);
    createEntityInArchetype(&g_gunArch, &akEntity);

    gunInit(&g_gunArch);
}

static void setupEnemy(void)
{
    g_enemyArch.flags = 0;
    FLAG_SET(g_enemyArch.flags, ARCH_BUFFERED);
    FLAG_SET(g_enemyArch.flags, ARCH_PHYSICS_BODY);

    if (!createArchetype(&Enemy_layout, 512, &g_enemyArch))
    { ERROR("Failed to create Enemy archetype"); return; }
    g_enemyCreated = true;

    {
        CollisionCallbacks cbs = {};
        cbs.onCollideEnter = onEnemyCollideEnter;
        archetypeSetCollisionCallbacks(&g_enemyArch, cbs);
    }

    enemyInit(&g_enemyArch);
}

static void gameInit(const c8 *projectDir)
{
    runtimeCreate(projectDir, runtimeDefaultConfig());

    collisionsInit();

    setupGun();
    setupBullets();
    setupEnemy();
    setupPlayer();

    runtimeRegisterArchetype(runtime, &g_bulletArch);
    runtimeRegisterArchetype(runtime, &g_enemyArch);
    runtimeRegisterArchetype(runtime, &g_playerArch);

    setMouseCaptured(true);
}

static u32 despawnDeadEnemies(void)
{
    u32 killed = 0;
    for (u32 c = 0; c < g_enemyArch.activeChunkCount; c++)
    {
        void **fields = getArchetypeFields(&g_enemyArch, c);
        if (!fields) continue;
        u32 count = g_enemyArch.arena[c].count;
        b8  *alive    = (b8  *)fields[EF_ALIVE];
        u32 *healthId = (u32 *)fields[EF_HEALTH_ID];
        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;
            HealthID hid = (HealthID)healthId[i];
            if (hid == HEALTH_ID_INVALID) continue;
            if (!healthIsAlive(&g_healthManager, hid))
            {
                u32 poolIdx = c * g_enemyArch.chunkCapacity + i;
                archetypePoolDespawn(&g_enemyArch, poolIdx);
                killed++;
            }
        }
    }
    return killed;
}

static void gameUpdate(f32 dt)
{
    if (g_playerCreated) playerUpdate(&g_playerArch, dt);
    if (g_bulletCreated) bulletUpdate(&g_bulletArch, dt);
    if (g_enemyCreated)  enemyUpdate(&g_enemyArch, dt);

    if (g_bulletCreated && g_enemyCreated) bulletHitScan(dt);

    runtimeUpdate(runtime, dt);

    // Scene data is available after the first runtimeUpdate — retry every frame until scene is live.
    if (!g_spawnPointsReady && sceneRuntime && sceneRuntime->loaded)
    {
        g_spawnPointCount = aiReadSpawnPoints("EnemySpawn", g_spawnPoints, MAX_SPAWN_POINTS);
        g_spawnPointsReady = true;
        if (g_spawnPointCount == 0)
            WARN("gameUpdate: no EnemySpawn entities found in scene");
        else
            INFO("gameUpdate: loaded %u EnemySpawn points", g_spawnPointCount);
    }

    damageFlush(&g_damageQueue, &g_healthManager);

    if (g_enemyCreated)
    {
        u32 killed    = despawnDeadEnemies();
        g_killCount  += killed;
        g_maxEnemies  = maxEnemiesFromKills(g_killCount);
        waveUpdate(dt);
    }
}

static void gameRender(f32 dt)
{
    runtimeBeginScenePass(runtime, dt);
    if (g_playerCreated) rendererDefaultArchetypeRender(&g_playerArch, renderer);
    if (g_bulletCreated) rendererDefaultArchetypeRender(&g_bulletArch, renderer);
    if (g_gunCreated)    rendererDefaultArchetypeRender(&g_gunArch, renderer);
    if (g_enemyCreated)  rendererDefaultArchetypeRender(&g_enemyArch, renderer);
    runtimeEndScenePass(runtime);
}

static void gameDestroy(void)
{
    setMouseCaptured(false);
    if (g_playerCreated) { playerDestroy(); destroyArchetype(&g_playerArch); g_playerCreated = false; }
    if (g_bulletCreated) { bulletDestroy(); destroyArchetype(&g_bulletArch); g_bulletCreated = false; }
    if (g_gunCreated)    { gunDestroy();    destroyArchetype(&g_gunArch);    g_gunCreated    = false; }
    if (g_enemyCreated)  { enemyDestroy();  destroyArchetype(&g_enemyArch);  g_enemyCreated  = false; }
    g_spawnPointsReady = false;
    g_spawnPointCount  = 0;
    g_killCount        = 0;
    g_maxEnemies       = 2;
    g_spawnTimer       = 0.0f;
    g_nextSpawn        = 0;
    runtimeDestroy(runtime);
}

void druidGetPlugin(GamePlugin *out)
{
    out->init    = gameInit;
    out->update  = gameUpdate;
    out->render  = gameRender;
    out->destroy = gameDestroy;
}
