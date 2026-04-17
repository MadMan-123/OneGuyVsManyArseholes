#include "Bullet.h"

#define POOL_CAPACITY   256
#define BULLET_LIFETIME 3.0f
#define BULLET_RADIUS   0.05f
#define BULLET_MASS     0.01f
DEFINE_ARCHETYPE(Bullet, BULLET_FIELDS)

// DRUID_FLAGS 0x0C
// isBuffered
// isPhysicsBody

static u32 s_ibSlot = (u32)-1;
static Archetype *s_arch = NULL;
static u32 s_sphereModelID = (u32)-1;

//=============================================================================
// Helpers

static b8 spawnBufferedEntity(Archetype *arch, u32 *outChunkIdx, u32 *outLocalIdx,
                              void ***outFields, u32 *outPoolIdx)
{
    if (!arch) return false;

    u32 poolIdx = archetypePoolSpawn(arch);
    if (poolIdx == (u32)-1)
        return false;

    u32 chunkIdx = poolIdx / arch->chunkCapacity;
    void **fields = getArchetypeFields(arch, chunkIdx);
    if (!fields)
    {
        archetypePoolDespawn(arch, poolIdx);
        return false;
    }

    if (outChunkIdx) *outChunkIdx = chunkIdx;
    if (outLocalIdx) *outLocalIdx = poolIdx % arch->chunkCapacity;
    if (outFields)   *outFields   = fields;
    if (outPoolIdx)  *outPoolIdx  = poolIdx;
    return true;
}

static void spawnBulletInArch(Archetype *arch, Vec3 position, Vec3 direction, f32 speed)
{
    if (!arch) return;

    // lazy model lookup
    if (s_sphereModelID == (u32)-1 && resources)
    {
        Model *sphere = resGetModelByName("Sphere");
        if (sphere)
            s_sphereModelID = (u32)(sphere - resources->modelBuffer);
        else if (resources->modelUsed > 0)
            s_sphereModelID = 0;
    }

    u32 chunkIdx = 0;
    u32 i = 0;
    void **fields = NULL;
    u32 poolIdx = 0;
    if (!spawnBufferedEntity(arch, &chunkIdx, &i, &fields, &poolIdx))
    {
        ERROR("bulletSpawn: archetypePoolSpawn/getArchetypeFields failed");
        return;
    }

    f32  *posX    = (f32  *)fields[BF_POS_X];
    f32  *posY    = (f32  *)fields[BF_POS_Y];
    f32  *posZ    = (f32  *)fields[BF_POS_Z];
    Vec4 *rot     = (Vec4 *)fields[BF_ROT];
    Vec3 *scl     = (Vec3 *)fields[BF_SCALE];
    f32  *velX    = (f32  *)fields[BF_VEL_X];
    f32  *velY    = (f32  *)fields[BF_VEL_Y];
    f32  *velZ    = (f32  *)fields[BF_VEL_Z];
    f32  *forceX  = (f32  *)fields[BF_FORCE_X];
    f32  *forceY  = (f32  *)fields[BF_FORCE_Y];
    f32  *forceZ  = (f32  *)fields[BF_FORCE_Z];
    u32  *bodyType = (u32 *)fields[BF_BODY_TYPE];
    f32  *mass    = (f32  *)fields[BF_MASS];
    f32  *restit  = (f32  *)fields[BF_RESTITUTION];
    f32  *damping = (f32  *)fields[BF_DAMPING];
    f32  *radius  = (f32  *)fields[BF_SPHERE_R];
    f32  *halfX   = (f32  *)fields[BF_HALF_X];
    f32  *halfY   = (f32  *)fields[BF_HALF_Y];
    f32  *halfZ   = (f32  *)fields[BF_HALF_Z];
    f32  *lifetime = (f32 *)fields[BF_LIFETIME];
    u32  *modelID = (u32  *)fields[BF_MODEL_ID];

    Vec3 dir = v3Norm(direction);

    // alive is already set by the buffered spawn helper
    posX[i] = position.x;
    posY[i] = position.y;
    posZ[i] = position.z;
    rot[i]  = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
    scl[i]  = (Vec3){BULLET_RADIUS * 2.0f, BULLET_RADIUS * 2.0f, BULLET_RADIUS * 2.0f};

    velX[i]   = dir.x * speed;
    velY[i]   = dir.y * speed;
    velZ[i]   = dir.z * speed;
    forceX[i] = 0.0f;
    forceY[i] = 0.0f;
    forceZ[i] = 0.0f;

    bodyType[i] = PHYS_BODY_DYNAMIC;
    mass[i]     = BULLET_MASS;
    // invMass is auto-computed by physics on first step
    restit[i]  = 0.0f;
    damping[i] = 0.0f;
    radius[i]  = BULLET_RADIUS;
    halfX[i]   = BULLET_RADIUS;
    halfY[i]   = BULLET_RADIUS;
    halfZ[i]   = BULLET_RADIUS;

    lifetime[i] = 0.0f;
    modelID[i]  = s_sphereModelID;
}

//=============================================================================
// Lifecycle

Archetype *bulletGetArchetype(void)
{
    return s_arch;
}

void bulletInit(Archetype *arch)
{
    s_arch = arch;

    s_ibSlot = rendererAcquireInstanceBuffer(renderer, 1024);

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
            ERROR("bulletInit: Sphere model not found and no models available");
    }
    else
    {
        ERROR("bulletInit: resources is NULL");
    }
}

void bulletSpawn(Vec3 position, Vec3 direction, f32 speed)
{
    if (!s_arch) return;  // arch not ready yet — drop the shot (imperceptible on frame 1)
    spawnBulletInArch(s_arch, position, direction, speed);
}

void bulletUpdate(Archetype *arch, f32 dt)
{
    s_arch = arch;

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
    if (s_ibSlot != (u32)-1) { rendererReleaseInstanceBuffer(renderer, s_ibSlot); s_ibSlot = (u32)-1; }
    s_arch = NULL;
}

void bulletRender(Archetype *arch, Renderer *r)
{
    if (!arch || !r || !resources) return;

    for (u32 ch = 0; ch < arch->activeChunkCount; ch++)
    {
        void **fields = getArchetypeFields(arch, ch);
        if (!fields) continue;

        u32   count   = arch->arena[ch].count;
        b8   *alive   = (b8  *)fields[BF_ALIVE];
        f32  *posX    = (f32 *)fields[BF_POS_X];
        f32  *posY    = (f32 *)fields[BF_POS_Y];
        f32  *posZ    = (f32 *)fields[BF_POS_Z];
        Vec4 *rot     = (Vec4 *)fields[BF_ROT];
        Vec3 *scl     = (Vec3 *)fields[BF_SCALE];
        u32  *modelID = (u32 *)fields[BF_MODEL_ID];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;
            u32 mID = modelID[i];
            if (mID >= resources->modelUsed) continue;

            Model *model = &resources->modelBuffer[mID];
            Transform t = {{posX[i], posY[i], posZ[i]}, rot[i], scl[i]};

            for (u32 m = 0; m < model->meshCount; m++)
            {
                u32 mi = model->meshIndices[m];
                if (mi >= resources->meshUsed) continue;
                updateShaderModel(r->defaultShader, t);
                drawMesh(&resources->meshBuffer[mi]);
            }
        }
    }
}

// standalone calls bulletInit directly
static void bulletInitPlugin(void) {}

void druidGetECSSystem_Bullet(ECSSystemPlugin *out)
{
    // game.cpp owns this archetype and calls bulletUpdate/bulletRender explicitly.
    // Returning NULL here prevents the editor's ECS loop from double-invoking them
    // and from stomping s_arch (which would redirect bulletSpawn to the wrong arch).
    out->init    = bulletInitPlugin;
    out->update  = NULL;
    out->render  = NULL;
    out->destroy = bulletDestroy;
}
