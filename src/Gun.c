#include "Gun.h"
#include "Player.h"
#include "GameConfig.h"
#include "game.h"


#define EYE_HEIGHT 1.6f

#define GUN_IDX_PISTOL 1
#define GUN_IDX_AK47   0

DEFINE_ARCHETYPE(Gun, GUN_FIELDS)

static u32  s_cachedModelIDs[2];
static b8   s_modelsCached = false;
static Vec3 s_gunOffset    = {GUN_HIP_X, GUN_HIP_Y, GUN_HIP_Z};
static f32  s_reloadTiltT  = 0.0f;

void gunInit(Archetype *arch)
{
    (void)arch;
    s_modelsCached = false;
}

void gunUpdate(Archetype *arch, f32 dt)
{
    if (!arch || arch->arena[0].count < 2) return;

    void **pf = getArchetypeFields(&g_playerArch, 0);
    if (!pf) return;
    f32  *posX     = (f32  *)pf[PLAYER_POSITION_X];
    f32  *posY     = (f32  *)pf[PLAYER_POSITION_Y];
    f32  *posZ     = (f32  *)pf[PLAYER_POSITION_Z];
    Vec4 *rot      = (Vec4 *)pf[PLAYER_ROTATION];
    u32  *weapon   = (u32  *)pf[PLAYER_WEAPON_TYPE];
    b8   *isAiming = (b8   *)pf[PLAYER_IS_AIMING];
    f32  *reloadCD = (f32  *)pf[PLAYER_RELOAD_COOLDOWN];
    if (!posX || !posY || !posZ || !rot) return;

    void **gf = getArchetypeFields(arch, 0);
    if (!gf) return;
    f32  *gX      = (f32  *)gf[GUN_POSITION_X];
    f32  *gY      = (f32  *)gf[GUN_POSITION_Y];
    f32  *gZ      = (f32  *)gf[GUN_POSITION_Z];
    Vec4 *gRot    = (Vec4 *)gf[GUN_ROTATION];
    u32  *modelID = (u32  *)gf[GUN_MODEL_ID];
    if (!gX || !gY || !gZ || !gRot) return;

    if (!s_modelsCached && modelID)
    {
        s_cachedModelIDs[0] = modelID[GUN_IDX_PISTOL];
        s_cachedModelIDs[1] = modelID[GUN_IDX_AK47];
        s_modelsCached = true;
    }

    u32 active = weapon ? weapon[0] : 0;  // WEAPON_PISTOL=0, WEAPON_AK47=1

    Vec3 eyePos = {posX[0], posY[0] + EYE_HEIGHT, posZ[0]};
    Vec3 target = (isAiming && isAiming[0])
                      ? (Vec3){GUN_ADS_X, GUN_ADS_Y, GUN_ADS_Z}
                      : (Vec3){GUN_HIP_X, GUN_HIP_Y, GUN_HIP_Z};
    f32 t = clamp(ADS_LERP_SPEED * dt, 0.0f, 1.0f);
    s_gunOffset.x = lerp(s_gunOffset.x, target.x, t);
    s_gunOffset.y = lerp(s_gunOffset.y, target.y, t);
    s_gunOffset.z = lerp(s_gunOffset.z, target.z, t);
    Vec3 gunPos = v3Add(eyePos, quatRotateVec3(rot[0], s_gunOffset));

    // Reload tilt: slerp gun rotation up while reloading, back down when finished
    b8  isReloading = reloadCD && reloadCD[0] > 0.0f;
    f32 tiltSpeed   = isReloading ? RELOAD_TILT_UP_SPEED : RELOAD_TILT_DOWN_SPEED;
    s_reloadTiltT   = clamp(s_reloadTiltT + (isReloading ? 1.0f : -1.0f) * tiltSpeed * dt,
                            0.0f, 1.0f);
    Vec4 tiltedRot  = quatNormalize(quatMul(rot[0], quatFromAxisAngle(v3Right, RELOAD_TILT_ANGLE)));
    Vec4 gunRot     = quatSlerp(rot[0], tiltedRot, s_reloadTiltT);

    gX[GUN_IDX_PISTOL]   = gunPos.x;
    gY[GUN_IDX_PISTOL]   = gunPos.y;
    gZ[GUN_IDX_PISTOL]   = gunPos.z;
    gRot[GUN_IDX_PISTOL] = gunRot;

    gX[GUN_IDX_AK47]   = gunPos.x;
    gY[GUN_IDX_AK47]   = gunPos.y;
    gZ[GUN_IDX_AK47]   = gunPos.z;
    gRot[GUN_IDX_AK47] = gunRot;

    if (modelID)
    {
        modelID[GUN_IDX_PISTOL] = (active == WEAPON_PISTOL) ? s_cachedModelIDs[0] : (u32)-1;
        modelID[GUN_IDX_AK47]   = (active == WEAPON_AK47)   ? s_cachedModelIDs[1] : (u32)-1;
    }
}

void gunDestroy(void)
{
    s_modelsCached = false;
    s_gunOffset    = (Vec3){GUN_HIP_X, GUN_HIP_Y, GUN_HIP_Z};
    s_reloadTiltT  = 0.0f;
}

static void gunInitPlugin(void) {}

void druidGetECSSystem_Gun(ECSSystemPlugin *out)
{
    out->init    = gunInitPlugin;
    out->update  = gunUpdate;
    out->render  = NULL;
    out->destroy = gunDestroy;
}
