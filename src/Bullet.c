#include "Bullet.h"

#define POOL_CAPACITY   256
#define BULLET_LIFETIME 3.0f
#define BULLET_RADIUS   0.05f
#define BULLET_MASS     0.01f
#define MAX_PENDING_BULLET_SPAWNS 256

// Field order must match the indices used in spawn/update/render.
DSAPI FieldInfo Bullet_fields[] = {
    { "Alive",           sizeof(b8),   FIELD_TEMP_COLD },
    { "PositionX",       sizeof(f32),  FIELD_TEMP_HOT },
    { "PositionY",       sizeof(f32),  FIELD_TEMP_HOT },
    { "PositionZ",       sizeof(f32),  FIELD_TEMP_HOT },
    { "Rotation",        sizeof(Vec4), FIELD_TEMP_HOT },
    { "Scale",           sizeof(Vec3), FIELD_TEMP_HOT },
    { "LinearVelocityX", sizeof(f32),  FIELD_TEMP_HOT },
    { "LinearVelocityY", sizeof(f32),  FIELD_TEMP_HOT },
    { "LinearVelocityZ", sizeof(f32),  FIELD_TEMP_HOT },
    { "ForceX",          sizeof(f32),  FIELD_TEMP_HOT },
    { "ForceY",          sizeof(f32),  FIELD_TEMP_HOT },
    { "ForceZ",          sizeof(f32),  FIELD_TEMP_HOT },
    { "PhysicsBodyType", sizeof(u32),  FIELD_TEMP_HOT },
    { "Mass",            sizeof(f32),  FIELD_TEMP_HOT },
    { "InvMass",         sizeof(f32),  FIELD_TEMP_HOT },
    { "Restitution",     sizeof(f32),  FIELD_TEMP_HOT },
    { "LinearDamping",   sizeof(f32),  FIELD_TEMP_HOT },
    { "SphereRadius",    sizeof(f32),  FIELD_TEMP_HOT },
    { "ColliderHalfX",   sizeof(f32),  FIELD_TEMP_HOT },
    { "ColliderHalfY",   sizeof(f32),  FIELD_TEMP_HOT },
    { "ColliderHalfZ",   sizeof(f32),  FIELD_TEMP_HOT },
    { "Lifetime",        sizeof(f32),  FIELD_TEMP_HOT },
    { "ModelID",         sizeof(u32),  FIELD_TEMP_COLD }
};

DSAPI StructLayout Bullet_layout = {
    "Bullet",
    Bullet_fields,
    sizeof(Bullet_fields) / sizeof(FieldInfo)
};

// DRUID_FLAGS 0x0C
// isBuffered
// isPhysicsBody

static u32 s_ibSlot = (u32)-1;
static Archetype *s_arch = NULL;
static u32 s_sphereModelID = (u32)-1;

typedef struct
{
    Vec3 position;
    Vec3 direction;
    f32 speed;
} PendingBulletSpawn;

static PendingBulletSpawn s_pendingSpawns[MAX_PENDING_BULLET_SPAWNS];
static u32 s_pendingSpawnCount = 0;

static void spawnBulletInArch(Archetype *arch, Vec3 position, Vec3 direction, f32 speed)
{
    if (!arch)
        return;

    if (s_sphereModelID == (u32)-1 && resources)
    {
        Model *sphere = resGetModelByName("Sphere");
        if (sphere)
            s_sphereModelID = (u32)(sphere - resources->modelBuffer);
    }

    u32 idx = archetypePoolSpawn(arch);
    if (idx == (u32)-1)
    {
        ERROR("bulletSpawn: archetypePoolSpawn failed");
        return;
    }

    if (arch->chunkCapacity == 0)
    {
        ERROR("bulletSpawn: invalid chunk capacity");
        return;
    }

    u32 chunkIdx = idx / arch->chunkCapacity;
    u32 localIdx = idx % arch->chunkCapacity;

    if (chunkIdx >= arch->arenaCount || localIdx >= arch->arena[chunkIdx].count)
    {
        ERROR("bulletSpawn: failed to map idx=%u to valid chunk/local", idx);
        return;
    }

    void **fields = getArchetypeFields(arch, chunkIdx);
    if (!fields)
    {
        ERROR("bulletSpawn: getArchetypeFields failed for chunk %u", chunkIdx);
        return;
    }

    b8   *alive    = (b8  *)fields[0];
    f32  *posX     = (f32 *)fields[1];
    f32  *posY     = (f32 *)fields[2];
    f32  *posZ     = (f32 *)fields[3];
    Vec4 *rot      = (Vec4 *)fields[4];
    Vec3 *scl      = (Vec3 *)fields[5];
    f32  *velX     = (f32 *)fields[6];
    f32  *velY     = (f32 *)fields[7];
    f32  *velZ     = (f32 *)fields[8];
    f32  *forceX   = (f32 *)fields[9];
    f32  *forceY   = (f32 *)fields[10];
    f32  *forceZ   = (f32 *)fields[11];
    u32  *bodyType = (u32 *)fields[12];
    f32  *mass     = (f32 *)fields[13];
    f32  *invMass  = (f32 *)fields[14];
    f32  *restit   = (f32 *)fields[15];
    f32  *damping  = (f32 *)fields[16];
    f32  *radius   = (f32 *)fields[17];
    f32  *halfX    = (f32 *)fields[18];
    f32  *halfY    = (f32 *)fields[19];
    f32  *halfZ    = (f32 *)fields[20];
    f32  *lifetime = (f32 *)fields[21];
    u32  *modelID  = (u32 *)fields[22];

    Vec3 dir = v3Norm(direction);

    alive[localIdx] = true;
    posX[localIdx] = position.x;
    posY[localIdx] = position.y;
    posZ[localIdx] = position.z;
    rot[localIdx] = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
    scl[localIdx] = (Vec3){BULLET_RADIUS * 2.0f, BULLET_RADIUS * 2.0f, BULLET_RADIUS * 2.0f};

    velX[localIdx] = dir.x * speed;
    velY[localIdx] = dir.y * speed;
    velZ[localIdx] = dir.z * speed;
    forceX[localIdx] = 0.0f;
    forceY[localIdx] = 0.0f;
    forceZ[localIdx] = 0.0f;

    bodyType[localIdx] = PHYS_BODY_DYNAMIC;
    mass[localIdx] = BULLET_MASS;
    invMass[localIdx] = (BULLET_MASS > 0.0f) ? (1.0f / BULLET_MASS) : 0.0f;
    restit[localIdx] = 0.0f;
    damping[localIdx] = 0.0f;
    radius[localIdx] = BULLET_RADIUS;
    halfX[localIdx] = BULLET_RADIUS;
    halfY[localIdx] = BULLET_RADIUS;
    halfZ[localIdx] = BULLET_RADIUS;

    lifetime[localIdx] = 0.0f;
    modelID[localIdx] = s_sphereModelID;
}

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
        {
            s_sphereModelID = (u32)(sphere - resources->modelBuffer);
        }
        else
        {
            ERROR("bulletInit: Sphere model not found!");
        }
    }
    else
    {
        ERROR("bulletInit: resources is NULL");
    }
}

void bulletSpawn(Vec3 position, Vec3 direction, f32 speed)
{
    if (!s_arch)
    {
        if (s_pendingSpawnCount < MAX_PENDING_BULLET_SPAWNS)
        {
            s_pendingSpawns[s_pendingSpawnCount++] = (PendingBulletSpawn){position, direction, speed};
        }
        else
        {
            ERROR("bulletSpawn: pending spawn queue full");
        }
        return;
    }

    spawnBulletInArch(s_arch, position, direction, speed);
}

void bulletUpdate(Archetype *arch, f32 dt)
{
    s_arch = arch;

    if (s_pendingSpawnCount > 0)
    {
        for (u32 i = 0; i < s_pendingSpawnCount; i++)
        {
            spawnBulletInArch(arch,
                              s_pendingSpawns[i].position,
                              s_pendingSpawns[i].direction,
                              s_pendingSpawns[i].speed);
        }
        s_pendingSpawnCount = 0;
    }

    for (u32 _ch = 0; _ch < arch->activeChunkCount; _ch++)
    {
        void **fields = getArchetypeFields(arch, _ch);
        if (!fields) continue;
        u32 count = arch->arena[_ch].count;

        b8  *alive    = (b8 *)fields[0];
        f32 *lifetime = (f32 *)fields[21];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;

            lifetime[i] += dt;
            if (lifetime[i] >= BULLET_LIFETIME)
            {
                archetypePoolDespawn(arch, (_ch * arch->chunkCapacity) + i);
            }
        }
    }
}

void bulletDestroy(void)
{
    if (s_ibSlot != (u32)-1) { rendererReleaseInstanceBuffer(renderer, s_ibSlot); s_ibSlot = (u32)-1; }
    s_arch = NULL;
    s_pendingSpawnCount = 0;
}

void bulletRender(Archetype *arch, Renderer *r)
{
    if (!arch || !r || !resources) return;

    for (u32 _ch = 0; _ch < arch->activeChunkCount; _ch++)
    {
        void **fields = getArchetypeFields(arch, _ch);
        if (!fields) continue;
        
        u32 count = arch->arena[_ch].count;
        b8  *alive    = (b8 *)fields[0];
        f32 *posX     = (f32 *)fields[1];
        f32 *posY     = (f32 *)fields[2];
        f32 *posZ     = (f32 *)fields[3];
        Vec4 *rot     = (Vec4 *)fields[4];
        Vec3 *scl     = (Vec3 *)fields[5];
        u32 *modelID  = (u32 *)fields[22];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;

            u32 mID = modelID[i];
            if (mID >= resources->modelUsed) continue;

            Model *model = &resources->modelBuffer[mID];
            Transform t = {{posX[i], posY[i], posZ[i]}, rot[i], scl[i]};
            
            u32 prevShader = r->defaultShader;
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

// Plugin init shim.
static void bulletInitPlugin(void)
{
    // Standalone path calls bulletInit directly.
}

void druidGetECSSystem_Bullet(ECSSystemPlugin *out)
{
    out->init    = bulletInitPlugin;
    out->update  = bulletUpdate;
    out->render  = bulletRender;
    out->destroy = bulletDestroy;
}
