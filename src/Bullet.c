#include "Bullet.h"

#define POOL_CAPACITY   256
#define BULLET_LIFETIME 3.0f
#define BULLET_RADIUS   0.05f
#define BULLET_MASS     0.01f
DEFINE_ARCHETYPE(Bullet, BULLET_FIELDS)

static Archetype *s_arch = NULL;
static u32 s_sphereModelID = (u32)-1;

void bulletInit(Archetype *arch)
{
    s_arch = arch;

    if (resources)
    {
        Model *sphere = resGetModelByName("Sphere");
        if (sphere)
            s_sphereModelID = (u32)(sphere - resources->modelBuffer);
        else if (resources->modelUsed > 0)
        {
            s_sphereModelID = 0;
            WARN("bulletInit: Sphere model not found, using model 0 as fallback");
        }
        else
            ERROR("bulletInit: no models available");
    }
}

void bulletSpawn(Vec3 position, Vec3 direction, f32 speed)
{
    if (!s_arch) return;

    u32 poolIdx = 0, i = 0;
    void **fields = NULL;
    if (!archetypePoolSpawnFields(s_arch, &poolIdx, &i, &fields)) return;

    Vec3 dir = v3Norm(direction);

    ((f32  *)fields[BF_POS_X])[i]     = position.x;
    ((f32  *)fields[BF_POS_Y])[i]     = position.y;
    ((f32  *)fields[BF_POS_Z])[i]     = position.z;
    ((Vec4 *)fields[BF_ROT])[i]       = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
    ((Vec3 *)fields[BF_SCALE])[i]     = (Vec3){BULLET_RADIUS * 2.0f, BULLET_RADIUS * 2.0f, BULLET_RADIUS * 2.0f};
    ((f32  *)fields[BF_VEL_X])[i]     = dir.x * speed;
    ((f32  *)fields[BF_VEL_Y])[i]     = dir.y * speed;
    ((f32  *)fields[BF_VEL_Z])[i]     = dir.z * speed;
    ((f32  *)fields[BF_FORCE_X])[i]   = 0.0f;
    ((f32  *)fields[BF_FORCE_Y])[i]   = 0.0f;
    ((f32  *)fields[BF_FORCE_Z])[i]   = 0.0f;
    ((u32  *)fields[BF_BODY_TYPE])[i] = PHYS_BODY_DYNAMIC;
    ((f32  *)fields[BF_MASS])[i]      = BULLET_MASS;
    ((f32  *)fields[BF_RESTITUTION])[i] = 0.0f;
    ((f32  *)fields[BF_DAMPING])[i]   = 0.0f;
    ((f32  *)fields[BF_SPHERE_R])[i]  = BULLET_RADIUS;
    ((f32  *)fields[BF_HALF_X])[i]    = BULLET_RADIUS;
    ((f32  *)fields[BF_HALF_Y])[i]    = BULLET_RADIUS;
    ((f32  *)fields[BF_HALF_Z])[i]    = BULLET_RADIUS;
    ((f32  *)fields[BF_LIFETIME])[i]  = 0.0f;
    ((u32  *)fields[BF_MODEL_ID])[i]  = s_sphereModelID;
}

void bulletUpdate(Archetype *arch, f32 dt)
{
    for (u32 ch = 0; ch < arch->activeChunkCount; ch++)
    {
        void **fields = getArchetypeFields(arch, ch);
        if (!fields) continue;
        u32 count = arch->arena[ch].count;

        b8  *alive    = (b8  *)fields[BF_ALIVE];
        f32 *lifetime = (f32 *)fields[BF_LIFETIME];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;
            lifetime[i] += dt;
            if (lifetime[i] >= BULLET_LIFETIME)
                archetypePoolDespawn(arch, (ch * arch->chunkCapacity) + i);
        }
    }
}

void bulletDestroy(void)
{
    s_arch = NULL;
}

Archetype *bulletGetArchetype(void)
{
    return s_arch;
}

static void bulletInitPlugin(void) {}

void druidGetECSSystem_Bullet(ECSSystemPlugin *out)
{
    out->init    = bulletInitPlugin;
    out->update  = NULL;
    out->render  = NULL;
    out->destroy = bulletDestroy;
}
