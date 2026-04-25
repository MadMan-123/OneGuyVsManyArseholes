#include "Enemy.h"
#include "AIBrain.h"
#include <math.h>

DEFINE_ARCHETYPE(Enemy, ENEMY_FIELDS)

// Collider box half-extents (metres)
#define ENEMY_HALF_X         0.375f
#define ENEMY_HALF_Y         1.0625f
#define ENEMY_HALF_Z         0.375f
#define ENEMY_MASS_KG        70.0f
#define ENEMY_DAMPING        6.0f
#define ENEMY_VISION_RANGE_M 25.0f
#define ENEMY_VISION_FOV_DEG 110.0f
#define ENEMY_HEARING_RANGE_M 12.0f
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

void enemyInitFields(void **fields, u32 i, Vec3 position)
{
    ((f32  *)fields[ENEMY_POSITION_X])[i]          = position.x;
    ((f32  *)fields[ENEMY_POSITION_Y])[i]          = position.y;
    ((f32  *)fields[ENEMY_POSITION_Z])[i]          = position.z;
    ((f32  *)fields[ENEMY_COLLIDER_OFFSET_Y])[i]   = ENEMY_HALF_Y;
    ((Vec4 *)fields[ENEMY_ROTATION])[i]            = (Vec4){0, 0, 0, 1};
    ((Vec3 *)fields[ENEMY_SCALE])[i]               = (Vec3){ZOMBIE_RENDER_SCALE, ZOMBIE_RENDER_SCALE, ZOMBIE_RENDER_SCALE};
    ((f32  *)fields[ENEMY_LINEAR_VELOCITY_X])[i]   = 0.0f;
    ((f32  *)fields[ENEMY_LINEAR_VELOCITY_Y])[i]   = 0.0f;
    ((f32  *)fields[ENEMY_LINEAR_VELOCITY_Z])[i]   = 0.0f;
    ((f32  *)fields[ENEMY_FORCE_X])[i]             = 0.0f;
    ((f32  *)fields[ENEMY_FORCE_Y])[i]             = 0.0f;
    ((f32  *)fields[ENEMY_FORCE_Z])[i]             = 0.0f;
    ((u32  *)fields[ENEMY_PHYSICS_BODY_TYPE])[i]   = PHYS_BODY_DYNAMIC;
    ((f32  *)fields[ENEMY_MASS])[i]                = ENEMY_MASS_KG;
    ((f32  *)fields[ENEMY_INV_MASS])[i]            = 1.0f / ENEMY_MASS_KG;
    ((f32  *)fields[ENEMY_RESTITUTION])[i]         = 0.0f;
    ((f32  *)fields[ENEMY_LINEAR_DAMPING])[i]      = ENEMY_DAMPING;
    ((f32  *)fields[ENEMY_SPHERE_RADIUS])[i]       = 0.0f;
    ((f32  *)fields[ENEMY_COLLIDER_HALF_X])[i]     = ENEMY_HALF_X;
    ((f32  *)fields[ENEMY_COLLIDER_HALF_Y])[i]     = ENEMY_HALF_Y;
    ((f32  *)fields[ENEMY_COLLIDER_HALF_Z])[i]     = ENEMY_HALF_Z;
    ((u32  *)fields[ENEMY_MODEL_ID])[i]            = s_modelID;
    ((b8   *)fields[ENEMY_ALIVE])[i]               = true;

    ((u32  *)fields[ENEMY_AI_STATE])[i]            = AI_STATE_IDLE;
    ((u32  *)fields[ENEMY_AI_PREV_STATE])[i]       = AI_STATE_IDLE;
    ((f32  *)fields[ENEMY_AI_STATE_TIMER])[i]      = 0.0f;
    ((u32  *)fields[ENEMY_HEALTH_ID])[i]           = (u32)-1;
    ((f32  *)fields[ENEMY_ATTACK_COOLDOWN])[i]     = 0.0f;
    ((f32  *)fields[ENEMY_VISION_RANGE])[i]        = ENEMY_VISION_RANGE_M;
    ((f32  *)fields[ENEMY_VISION_FOV_COS])[i]      = cosf(radians(ENEMY_VISION_FOV_DEG * 0.5f));
    ((f32  *)fields[ENEMY_HEARING_RANGE])[i]       = ENEMY_HEARING_RANGE_M;
    ((f32  *)fields[ENEMY_LAST_SEEN_X])[i]         = 0.0f;
    ((f32  *)fields[ENEMY_LAST_SEEN_Y])[i]         = 0.0f;
    ((f32  *)fields[ENEMY_LAST_SEEN_Z])[i]         = 0.0f;
    ((f32  *)fields[ENEMY_LAST_SEEN_AGE])[i]       = 9999.0f;
    ((f32  *)fields[ENEMY_WANDER_TARGET_X])[i]     = position.x;
    ((f32  *)fields[ENEMY_WANDER_TARGET_Z])[i]     = position.z;
    ((f32  *)fields[ENEMY_WANDER_TIMER])[i]        = 0.0f;
    ((f32  *)fields[ENEMY_YAW])[i]                 = 0.0f;
    ((b8   *)fields[ENEMY_IS_GROUNDED])[i]         = false;
}

u32 enemySpawnAt(Vec3 position)
{
    if (!s_arch) { WARN("enemySpawnAt: archetype not initialised"); return (u32)-1; }

    u32 poolIdx = 0, i = 0;
    void **fields = NULL;
    if (!archetypePoolSpawnFields(s_arch, &poolIdx, &i, &fields))
    { WARN("enemySpawnAt: pool spawn failed (pool full?)"); return (u32)-1; }
    INFO("enemySpawnAt: spawned poolIdx=%u localIdx=%u modelID=%u", poolIdx, i, s_modelID);

    enemyInitFields(fields, i, position);
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

u32 enemyGetModelID(void)
{
    return s_modelID;
}
