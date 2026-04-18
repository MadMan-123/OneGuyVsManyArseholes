#include "Gun.h"

// DRUID_FLAGS 0x00

#define PISTOL_COOLDOWN    0.15f
#define PISTOL_AMMO        9.0f
#define PISTOL_RELOAD      1.8f
#define PISTOL_SPEED       120.0f
#define PISTOL_SPREAD_MAX  0.01f
#define PISTOL_RECOIL      0.03f
#define PISTOL_RECOIL_FIRST 0.035f

#define AK_COOLDOWN        0.1f
#define AK_AMMO            30.0f
#define AK_RELOAD          3.1f
#define AK_SPEED           180.0f
#define AK_SPREAD_MAX      0.06f
#define AK_SPREAD_RATE     0.012f
#define AK_SPREAD_DECAY    0.08f
#define AK_RECOIL          0.05f
#define AK_RECOIL_FIRST    0.10f

DEFINE_ARCHETYPE(Gun, GUN_FIELDS)

static u32 s_ibSlot = (u32)-1;

void gunInit(void)
{
    s_ibSlot = rendererAcquireInstanceBuffer(renderer, 1024);
}

void gunUpdate(Archetype *arch, f32 dt)
{
    for (u32 _ch = 0; _ch < arch->activeChunkCount; _ch++)
    {
        void **fields = getArchetypeFields(arch, _ch);
        if (!fields) continue;
        u32 count = arch->arena[_ch].count;

        f32 *PositionX = (f32 *)fields[GUN_POSITION_X];
        f32 *PositionY = (f32 *)fields[GUN_POSITION_Y];
        f32 *PositionZ = (f32 *)fields[GUN_POSITION_Z];
        Vec4 *Rotation = (Vec4 *)fields[GUN_ROTATION];
        Vec3 *Scale = (Vec3 *)fields[GUN_SCALE];
        f32 *COOLDOWN = (f32 *)fields[GUN_COOLDOWN];
        u32 *AMMO = (u32 *)fields[GUN_AMMO];
        f32 *RELOAD = (f32 *)fields[GUN_RELOAD];
        f32 *SPEED = (f32 *)fields[GUN_SPEED];
        f32 *SPREAD_MAX = (f32 *)fields[GUN_SPREAD_MAX];
        f32 *SPREAD_RATE = (f32 *)fields[GUN_SPREAD_RATE];
        f32 *SPREAD_DECAY = (f32 *)fields[GUN_SPREAD_DECAY];
        f32 *RECOIL = (f32 *)fields[GUN_RECOIL];
        f32 *RECOIL_FIRST = (f32 *)fields[GUN_RECOIL_FIRST];

        for (u32 i = 0; i < count; i++)
        {
        }
    }
}

void spawnGunInArch(Archetype *arch)
{
    if (!arch)
        return;

    
}

// Optional: implement custom rendering for this archetype.
// When render is NULL (see druidGetECSSystem below), the engine uses a
// default forward pass based on Position/Rotation/Scale/ModelID fields.
// Uncomment and set out->render = gunRender to take full control.

/*
void gunRender(Archetype *arch, Renderer *r)
{
    (void)r;
    for (u32 _ch = 0; _ch < arch->activeChunkCount; _ch++)
    {
        void **fields = getArchetypeFields(arch, _ch);
        if (!fields) continue;
        u32 count = arch->arena[_ch].count;

        f32 *PositionX = (f32 *)fields[GUN_POSITION_X];
        f32 *PositionY = (f32 *)fields[GUN_POSITION_Y];
        f32 *PositionZ = (f32 *)fields[GUN_POSITION_Z];
        Vec4 *Rotation = (Vec4 *)fields[GUN_ROTATION];
        Vec3 *Scale = (Vec3 *)fields[GUN_SCALE];
        f32 *COOLDOWN = (f32 *)fields[GUN_COOLDOWN];
        u32 *AMMO = (u32 *)fields[GUN_AMMO];
        f32 *RELOAD = (f32 *)fields[GUN_RELOAD];
        f32 *SPEED = (f32 *)fields[GUN_SPEED];
        f32 *SPREAD_MAX = (f32 *)fields[GUN_SPREAD_MAX];
        f32 *SPREAD_RATE = (f32 *)fields[GUN_SPREAD_RATE];
        f32 *SPREAD_DECAY = (f32 *)fields[GUN_SPREAD_DECAY];
        f32 *RECOIL = (f32 *)fields[GUN_RECOIL];
        f32 *RECOIL_FIRST = (f32 *)fields[GUN_RECOIL_FIRST];

        for (u32 i = 0; i < count; i++)
        {
        }
    }
}
*/

void gunDestroy(void)
{
    if (s_ibSlot != (u32)-1) { rendererReleaseInstanceBuffer(renderer, s_ibSlot); s_ibSlot = (u32)-1; }
}

void druidGetECSSystem_Gun(ECSSystemPlugin *out)
{
    out->init    = gunInit;
    out->update  = gunUpdate;
    out->render  = NULL;  // default forward pass; set to gunRender for custom
    out->destroy = gunDestroy;
}
