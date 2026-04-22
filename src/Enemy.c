#include "Enemy.h"
#include "AIBrain.h"
#include <math.h>

DEFINE_ARCHETYPE(Enemy, ENEMY_FIELDS)

// Collider box half-extents (metres)
#define ENEMY_HALF_X      0.30f
#define ENEMY_HALF_Y      0.85f
#define ENEMY_HALF_Z      0.30f
#define ENEMY_MASS        70.0f
#define ENEMY_DAMPING     6.0f
#define ENEMY_VISION_RANGE 25.0f
#define ENEMY_VISION_FOV_DEG 110.0f
#define ENEMY_HEARING_RANGE 12.0f
// Uniform render scale for zombie.fbx — FBX is typically exported in cm; adjust
// if the model appears giant (try 0.01) or tiny (try 1.0).
#define ZOMBIE_RENDER_SCALE 0.0125f

static Archetype *s_arch = NULL;
static u32 s_modelID = (u32)-1;

void enemyInit(Archetype *arch)
{
    s_arch = arch;

    if (resources)
    {
        Model *m = resGetModelByName("zombie.fbx");
        if (!m) { WARN("enemyInit: zombie.fbx not found, falling back to primitive"); m = resGetModelByName("Cube"); }
        if (!m)   m = resGetModelByName("Sphere");
        if (m) s_modelID = (u32)(m - resources->modelBuffer);
        else if (resources->modelUsed > 0) { s_modelID = 0; WARN("enemyInit: using model 0 fallback"); }
        else ERROR("enemyInit: no models available");
    }
}

u32 enemySpawnAt(Vec3 position)
{
    if (!s_arch) { WARN("enemySpawnAt: archetype not initialised"); return (u32)-1; }

    u32 poolIdx = 0, i = 0;
    void **fields = NULL;
    if (!archetypePoolSpawnFields(s_arch, &poolIdx, &i, &fields))
    { WARN("enemySpawnAt: pool spawn failed (pool full?)"); return (u32)-1; }
    INFO("enemySpawnAt: spawned poolIdx=%u localIdx=%u modelID=%u", poolIdx, i, s_modelID);

    ((f32  *)fields[EF_POS_X])[i]       = position.x;
    ((f32  *)fields[EF_POS_Y])[i]       = position.y;
    ((f32  *)fields[EF_POS_Z])[i]       = position.z;
    ((Vec4 *)fields[EF_ROT])[i]         = (Vec4){0, 0, 0, 1};
    ((Vec3 *)fields[EF_SCALE])[i]       = (Vec3){ZOMBIE_RENDER_SCALE, ZOMBIE_RENDER_SCALE, ZOMBIE_RENDER_SCALE};
    ((f32  *)fields[EF_VEL_X])[i]       = 0.0f;
    ((f32  *)fields[EF_VEL_Y])[i]       = 0.0f;
    ((f32  *)fields[EF_VEL_Z])[i]       = 0.0f;
    ((f32  *)fields[EF_FORCE_X])[i]     = 0.0f;
    ((f32  *)fields[EF_FORCE_Y])[i]     = 0.0f;
    ((f32  *)fields[EF_FORCE_Z])[i]     = 0.0f;
    ((u32  *)fields[EF_BODY_TYPE])[i]   = PHYS_BODY_DYNAMIC;
    ((f32  *)fields[EF_MASS])[i]        = ENEMY_MASS;
    ((f32  *)fields[EF_INV_MASS])[i]    = 1.0f / ENEMY_MASS;
    ((f32  *)fields[EF_RESTITUTION])[i] = 0.0f;
    ((f32  *)fields[EF_DAMPING])[i]     = ENEMY_DAMPING;
    ((f32  *)fields[EF_SPHERE_R])[i]    = ENEMY_HALF_X;
    ((f32  *)fields[EF_HALF_X])[i]      = ENEMY_HALF_X;
    ((f32  *)fields[EF_HALF_Y])[i]      = ENEMY_HALF_Y;
    ((f32  *)fields[EF_HALF_Z])[i]      = ENEMY_HALF_Z;
    ((u32  *)fields[EF_MODEL_ID])[i]    = s_modelID;

    ((u32  *)fields[EF_STATE])[i]        = AI_STATE_IDLE;
    ((u32  *)fields[EF_PREV_STATE])[i]   = AI_STATE_IDLE;
    ((f32  *)fields[EF_STATE_TIMER])[i]  = 0.0f;
    ((u32  *)fields[EF_HEALTH_ID])[i]    = (u32)-1;  // caller registers health after spawn
    ((f32  *)fields[EF_ATTACK_CD])[i]   = 0.0f;
    ((f32  *)fields[EF_VISION_RANGE])[i] = ENEMY_VISION_RANGE;
    ((f32  *)fields[EF_VISION_COS])[i]   = cosf(radians(ENEMY_VISION_FOV_DEG * 0.5f));
    ((f32  *)fields[EF_HEARING])[i]      = ENEMY_HEARING_RANGE;
    ((f32  *)fields[EF_LAST_SEEN_X])[i]   = 0.0f;
    ((f32  *)fields[EF_LAST_SEEN_Y])[i]   = 0.0f;
    ((f32  *)fields[EF_LAST_SEEN_Z])[i]   = 0.0f;
    ((f32  *)fields[EF_LAST_SEEN_AGE])[i] = 9999.0f;
    ((f32  *)fields[EF_WANDER_X])[i]      = position.x;  // start at spawn, timer=0 will resample
    ((f32  *)fields[EF_WANDER_Z])[i]      = position.z;
    ((f32  *)fields[EF_WANDER_TIMER])[i]  = 0.0f;        // sample new target immediately
    ((f32  *)fields[EF_YAW])[i]           = 0.0f;
    ((b8   *)fields[EF_IS_GROUNDED])[i]  = false;
    return poolIdx;
}

void enemyUpdate(Archetype *arch, f32 dt)
{
    aiBrainTick(arch, dt);
}

void enemyDestroy(void)
{
    s_arch = NULL;
}

Archetype *enemyGetArchetype(void)
{
    return s_arch;
}
