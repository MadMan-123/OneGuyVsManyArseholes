#include "Bullet.h"
#include <math.h>

#define BULLET_LIFETIME_SECS 3.0f
DEFINE_ARCHETYPE(Bullet, BULLET_FIELDS)

static Archetype *s_arch = NULL;

void bulletInit(Archetype *arch)
{
    s_arch = arch;
}

void bulletSpawn(Vec3 position, Vec3 direction, f32 speed)
{
    if (!s_arch) return;

    u32 poolIdx = prefabSpawn(s_arch, 0, position);
    if (poolIdx == (u32)-1) return;

    // Override velocity with the caller's direction and speed
    u32 chunkIdx = poolIdx / s_arch->chunkCapacity;
    u32 localIdx = poolIdx % s_arch->chunkCapacity;
    void **fields = getArchetypeFields(s_arch, chunkIdx);
    if (!fields) return;

    Vec3 dir = v3Norm(direction);
    ((f32 *)fields[BULLET_LINEAR_VELOCITY_X])[localIdx] = dir.x * speed;
    ((f32 *)fields[BULLET_LINEAR_VELOCITY_Y])[localIdx] = dir.y * speed;
    ((f32 *)fields[BULLET_LINEAR_VELOCITY_Z])[localIdx] = dir.z * speed;

    // Rotate from +Y (bullet model's default orientation) to travel direction
    Vec3 fwd     = v3Up;
    Vec3 axis    = v3Cross(fwd, dir);
    f32  axisLen = v3Mag(axis);
    Vec4 lookRot;
    if (axisLen < 0.0001f)
        lookRot = (v3Dot(fwd, dir) > 0.0f) ? quatIdentity()
                                           : quatFromAxisAngle(v3Forward, 3.14159265f);
    else
        lookRot = quatFromAxisAngle(v3Norm(axis),
                                    acosf(clamp(v3Dot(fwd, dir), -1.0f, 1.0f)));
    ((Vec4 *)fields[BULLET_ROTATION])[localIdx] = lookRot;

    ((f32 *)fields[BULLET_LIFETIME])[localIdx]  = 0.0f;
}

void bulletUpdate(Archetype *arch, f32 dt)
{
    for (u32 ch = 0; ch < arch->activeChunkCount; ch++)
    {
        void **fields = getArchetypeFields(arch, ch);
        if (!fields) continue;
        u32 count = arch->arena[ch].count;

        b8  *alive    = (b8  *)fields[BULLET_ALIVE];
        f32 *lifetime = (f32 *)fields[BULLET_LIFETIME];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;
            lifetime[i] += dt;
            if (lifetime[i] >= BULLET_LIFETIME_SECS)
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
