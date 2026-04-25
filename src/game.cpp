#include "game.h"
#include "GameConfig.h"
#include "GameAudio.h"
#include "Player.h"
#include "Bullet.h"
#include "Gun.h"
#include "Enemy.h"
#include "Collisions.h"
#include "AISpawn.h"
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

static Vec3 g_spawnPoints[MAX_SPAWN_POINTS];
static u32  g_spawnPointCount  = 0;
static b8   g_spawnPointsReady = false;
static u32  g_killCount  = 0;
static u32  g_maxEnemies = WAVE_INITIAL_CAP;
static f32  g_spawnTimer = 0.0f;
static u32  g_nextSpawn  = 0;
static u32  g_zombiePrefabIdx = (u32)-1;

static void spawnAndRegisterEnemy(Vec3 pos)
{
    u32 poolIdx;
    if (g_zombiePrefabIdx != (u32)-1)
    {
        // prefabSpawn stamps all stored visual/physics data from the prefab,
        // then overrides PositionX/Y/Z with pos. We only reset the fields
        // that are pure runtime state and can't be meaningfully stored.
        poolIdx = prefabSpawn(&g_enemyArch, g_zombiePrefabIdx, pos);
        if (poolIdx == (u32)-1) return;

        u32 chunkIdx = poolIdx / g_enemyArch.chunkCapacity;
        u32 localIdx = poolIdx % g_enemyArch.chunkCapacity;
        void **fields = getArchetypeFields(&g_enemyArch, chunkIdx);
        if (!fields) return;

        ((b8  *)fields[ENEMY_ALIVE])[localIdx]            = true;
        ((u32 *)fields[ENEMY_MODEL_ID])[localIdx]         = enemyGetModelID();
        // AI state — always start idle
        ((u32 *)fields[ENEMY_AI_STATE])[localIdx]         = AI_STATE_IDLE;
        ((u32 *)fields[ENEMY_AI_PREV_STATE])[localIdx]    = AI_STATE_IDLE;
        ((f32 *)fields[ENEMY_AI_STATE_TIMER])[localIdx]   = 0.0f;
        ((f32 *)fields[ENEMY_ATTACK_COOLDOWN])[localIdx]  = 0.0f;
        ((f32 *)fields[ENEMY_LAST_SEEN_AGE])[localIdx]    = 9999.0f;
        ((f32 *)fields[ENEMY_WANDER_TARGET_X])[localIdx]  = pos.x;
        ((f32 *)fields[ENEMY_WANDER_TARGET_Z])[localIdx]  = pos.z;
        ((f32 *)fields[ENEMY_WANDER_TIMER])[localIdx]     = 0.0f;
        ((b8  *)fields[ENEMY_IS_GROUNDED])[localIdx]      = false;
        // Perception — prefab may have been saved with zeros, always use design values
        ((f32 *)fields[ENEMY_VISION_RANGE])[localIdx]     = 25.0f;
        ((f32 *)fields[ENEMY_VISION_FOV_COS])[localIdx]   = 0.574f; // cos(55 deg)
        ((f32 *)fields[ENEMY_HEARING_RANGE])[localIdx]    = 12.0f;

        HealthID hid = healthRegister(&g_healthManager, ENEMY_START_HP);
        if (hid == HEALTH_ID_INVALID) return;
        ((u32 *)fields[ENEMY_HEALTH_ID])[localIdx] = (u32)hid;
    }
    else
    {
        // No prefab — enemySpawnAt sets all fields from hardcoded defaults
        poolIdx = enemySpawnAt(pos);
        if (poolIdx == (u32)-1) return;

        u32 chunkIdx = poolIdx / g_enemyArch.chunkCapacity;
        u32 localIdx = poolIdx % g_enemyArch.chunkCapacity;
        void **fields = getArchetypeFields(&g_enemyArch, chunkIdx);
        if (!fields) return;

        HealthID hid = healthRegister(&g_healthManager, ENEMY_START_HP);
        if (hid == HEALTH_ID_INVALID) return;
        ((u32 *)fields[ENEMY_HEALTH_ID])[localIdx] = (u32)hid;
    }
}

// Returns enemies live for cap comparisons.
static u32 countLiveEnemies(void)
{
    u32 live = 0;
    for (u32 c = 0; c < g_enemyArch.activeChunkCount; c++)
    {
        void **fields = getArchetypeFields(&g_enemyArch, c);
        if (!fields) continue;
        b8 *alive = (b8 *)fields[ENEMY_ALIVE];
        for (u32 i = 0; i < g_enemyArch.arena[c].count; i++)
            if (alive[i]) live++;
    }
    return live;
}

// Max live enemies as a function of total kills — all thresholds and caps are
// controlled by the WAVE_* macros in GameConfig.h.
static u32 maxEnemiesFromKills(u32 kills)
{
    u32 cap;
    if (kills < WAVE_TIER1_KILLS)
        cap = WAVE_INITIAL_CAP + (kills / WAVE_TIER0_STEP_SIZE) * WAVE_TIER0_STEP_ADD;
    else if (kills < WAVE_TIER2_KILLS)
        cap = WAVE_TIER1_CAP   + ((kills - WAVE_TIER1_KILLS) / WAVE_TIER1_STEP_SIZE) * WAVE_TIER1_STEP_ADD;
    else
        cap = WAVE_TIER2_CAP   + ((kills - WAVE_TIER2_KILLS) / WAVE_TIER2_STEP_SIZE) * WAVE_TIER2_STEP_ADD;
    return cap < WAVE_MAX_CAP ? cap : WAVE_MAX_CAP;
}

// Refills enemies up to g_maxEnemies, one per WAVE_SPAWN_INTERVAL seconds.
// Cycles through spawn points round-robin.
static void waveUpdate(f32 dt)
{
    if (g_spawnPointCount == 0) return;
    if (countLiveEnemies() >= g_maxEnemies) return;

    g_spawnTimer -= dt;
    if (g_spawnTimer > 0.0f) return;
    g_spawnTimer = WAVE_SPAWN_INTERVAL;

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
        ((f32  *)fields[PLAYER_POSITION_X])[0]       = 0.0f;
        ((f32  *)fields[PLAYER_POSITION_Y])[0]       = 5.0f;
        ((f32  *)fields[PLAYER_POSITION_Z])[0]       = 0.0f;
        ((Vec4 *)fields[PLAYER_ROTATION])[0]         = Vec4{0, 0, 0, 1};
        ((Vec3 *)fields[PLAYER_SCALE])[0]            = Vec3{0.8f, 1.8f, 0.8f};
        ((u32  *)fields[PLAYER_PHYSICS_BODY_TYPE])[0] = PHYS_BODY_DYNAMIC;
        ((f32  *)fields[PLAYER_MASS])[0]             = 80.0f;
        ((f32  *)fields[PLAYER_RESTITUTION])[0]      = 0.0f;
        ((f32  *)fields[PLAYER_LINEAR_DAMPING])[0]   = 0.1f;
        ((f32  *)fields[PLAYER_COLLIDER_HALF_X])[0]  = 0.4f;
        ((f32  *)fields[PLAYER_COLLIDER_HALF_Y])[0]  = 0.9f;
        ((f32  *)fields[PLAYER_COLLIDER_HALF_Z])[0]  = 0.4f;
        ((u32  *)fields[PLAYER_WEAPON_TYPE])[0]      = 0;
        ((f32  *)fields[PLAYER_AMMO_PISTOL])[0]      = PISTOL_CLIP_SIZE;
        ((f32  *)fields[PLAYER_AMMO_AK])[0]          = AK_CLIP_SIZE;
        ((b8   *)fields[PLAYER_HAS_RELOADED])[0]     = true;

        HealthID hid = healthRegister(&g_healthManager, PLAYER_START_HP);
        ((u32  *)fields[PLAYER_HEALTH_ID])[0]        = (u32)hid;
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

    {
        CollisionCallbacks cbs = {};
        cbs.onCollideEnter = onBulletCollideEnter;
        archetypeSetCollisionCallbacks(&g_bulletArch, cbs);
    }

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

    gameAudioInit();
    collisionsInit();

    setupGun();
    setupBullets();
    setupEnemy();
    setupPlayer();

    // Find the Zombie prefab (loaded by runtimeCreate from prefabs/)
    {
        PrefabBucket *b = prefabGetBucket(&g_enemyArch);
        if (b)
        {
            for (u32 i = 0; i < b->prefabCount; i++)
            {
                if (strcmp(b->prefabs[i].name, "Zombie") == 0)
                {
                    g_zombiePrefabIdx = i;
                    INFO("gameInit: Zombie prefab found at index %u", i);
                    break;
                }
            }
            if (g_zombiePrefabIdx == (u32)-1)
                WARN("gameInit: Zombie prefab not found — falling back to hardcoded spawn");
        }
        else
            WARN("gameInit: no Enemy prefab bucket — falling back to hardcoded spawn");
    }

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
        b8  *alive    = (b8  *)fields[ENEMY_ALIVE];
        u32 *healthId = (u32 *)fields[ENEMY_HEALTH_ID];
        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;
            HealthID hid = (HealthID)healthId[i];
            if (hid == HEALTH_ID_INVALID) continue;
            if (!healthIsAlive(&g_healthManager, hid))
            {
                u32 poolIdx = c * g_enemyArch.chunkCapacity + i;
                archetypePoolDespawn(&g_enemyArch, poolIdx);
                gameAudioPlayEnemyDeath();
                killed++;
            }
        }
    }
    return killed;
}

static void gameUpdate(f32 dt)
{
    gameAudioUpdate(dt);
    if (g_playerCreated) playerUpdate(&g_playerArch, dt);
    if (g_bulletCreated) bulletUpdate(&g_bulletArch, dt);
    if (g_enemyCreated)  enemyUpdate(&g_enemyArch, dt);

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

    // Player death check
    static b8 s_playerDead = false;
    if (!s_playerDead && g_playerCreated)
    {
        void **pf = getArchetypeFields(&g_playerArch, 0);
        if (pf)
        {
            HealthID hid = (HealthID)((u32 *)pf[PLAYER_HEALTH_ID])[0];
            if (!healthIsAlive(&g_healthManager, hid))
            {
                s_playerDead = true;
                DEBUG("PLAYER DIED");
            }
        }
    }

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
    g_zombiePrefabIdx  = (u32)-1;
    runtimeDestroy(runtime);
}

void druidGetPlugin(GamePlugin *out)
{
    out->init    = gameInit;
    out->update  = gameUpdate;
    out->render  = gameRender;
    out->destroy = gameDestroy;
}
