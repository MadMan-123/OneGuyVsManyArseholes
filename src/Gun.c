#include "Gun.h"
#include "Player.h"
#include "game.h"


#define EYE_HEIGHT 1.6f

#define GUN_IDX_PISTOL 1
#define GUN_IDX_AK47   0

DEFINE_ARCHETYPE(Gun, GUN_FIELDS)

static u32 s_cachedModelIDs[2];
static b8  s_modelsCached = false;

void gunInit(Archetype *arch)
{
    (void)arch;
    s_modelsCached = false;
}

void gunUpdate(Archetype *arch, f32 dt)
{
    (void)dt;
    if (!arch || arch->arena[0].count < 2) return;

    void **pf = getArchetypeFields(&g_playerArch, 0);
    if (!pf) return;
    f32  *posX   = (f32  *)pf[PF_POS_X];
    f32  *posY   = (f32  *)pf[PF_POS_Y];
    f32  *posZ   = (f32  *)pf[PF_POS_Z];
    Vec4 *rot    = (Vec4 *)pf[PF_ROT];
    u32  *weapon   = (u32  *)pf[PF_WEAPON];
    b8   *isAiming = (b8   *)pf[PF_IS_AIMING];
    if (!posX || !posY || !posZ || !rot) return;

    void **gf = getArchetypeFields(arch, 0);
    if (!gf) return;
    f32  *gX      = (f32  *)gf[GF_POS_X];
    f32  *gY      = (f32  *)gf[GF_POS_Y];
    f32  *gZ      = (f32  *)gf[GF_POS_Z];
    Vec4 *gRot    = (Vec4 *)gf[GF_ROT];
    u32  *modelID = (u32  *)gf[GF_MODEL_ID];
    if (!gX || !gY || !gZ || !gRot) return;

    if (!s_modelsCached && modelID)
    {
        s_cachedModelIDs[0] = modelID[GUN_IDX_PISTOL];
        s_cachedModelIDs[1] = modelID[GUN_IDX_AK47];
        s_modelsCached = true;
    }

    u32 active = weapon ? weapon[0] : 0;  // WEAPON_PISTOL=0, WEAPON_AK47=1

    Vec3 eyePos = {posX[0], posY[0] + EYE_HEIGHT, posZ[0]};
    Vec3 offset = (isAiming && isAiming[0])
                      ? (Vec3){GUN_ADS_X, GUN_ADS_Y, GUN_ADS_Z}
                      : (Vec3){GUN_HIP_X, GUN_HIP_Y, GUN_HIP_Z};
    Vec3 gunPos = v3Add(eyePos, quatRotateVec3(rot[0], offset));

    gX[GUN_IDX_PISTOL]   = gunPos.x;
    gY[GUN_IDX_PISTOL]   = gunPos.y;
    gZ[GUN_IDX_PISTOL]   = gunPos.z;
    gRot[GUN_IDX_PISTOL] = rot[0];

    gX[GUN_IDX_AK47]   = gunPos.x;
    gY[GUN_IDX_AK47]   = gunPos.y;
    gZ[GUN_IDX_AK47]   = gunPos.z;
    gRot[GUN_IDX_AK47] = rot[0];

    if (modelID)
    {
        modelID[GUN_IDX_PISTOL] = (active == WEAPON_PISTOL) ? s_cachedModelIDs[0] : (u32)-1;
        modelID[GUN_IDX_AK47]   = (active == WEAPON_AK47)   ? s_cachedModelIDs[1] : (u32)-1;
    }
}

void gunDestroy(void)
{
    s_modelsCached = false;
}

static void gunInitPlugin(void) {}

void druidGetECSSystem_Gun(ECSSystemPlugin *out)
{
    out->init    = gunInitPlugin;
    out->update  = gunUpdate;
    out->render  = NULL;
    out->destroy = gunDestroy;
}
