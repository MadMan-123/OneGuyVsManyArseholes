#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

DSAPI void gunInit(Archetype *arch);
DSAPI void gunUpdate(Archetype *arch, f32 dt);
DSAPI void gunDestroy(void);
DSAPI void druidGetECSSystem_Gun(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif
// <DRUID_GEN_BEGIN Gun>
// DRUID_FLAGS 0x00

#define GUN_FIELDS(FIELD) \
    FIELD(GUN_POSITION_X, "PositionX", f32, HOT) \
    FIELD(GUN_POSITION_Y, "PositionY", f32, HOT) \
    FIELD(GUN_POSITION_Z, "PositionZ", f32, HOT) \
    FIELD(GUN_ROTATION, "Rotation", Vec4, HOT) \
    FIELD(GUN_SCALE, "Scale", Vec3, HOT) \
    FIELD(GUN_MODEL_ID, "ModelID", u32, COLD) \
    FIELD(GUN_COOLDOWN, "Cooldown", f32, COLD) \
    FIELD(GUN_AMMO, "Ammo", f32, COLD) \
    FIELD(GUN_RELOAD_TIME, "ReloadTime", f32, COLD) \
    FIELD(GUN_BULLET_SPEED, "BulletSpeed", f32, COLD) \
    FIELD(GUN_SPREAD_MAX, "SpreadMax", f32, COLD) \
    FIELD(GUN_SPREAD_RATE, "SpreadRate", f32, COLD) \
    FIELD(GUN_SPREAD_DECAY, "SpreadDecay", f32, COLD) \
    FIELD(GUN_RECOIL, "Recoil", f32, COLD) \
    FIELD(GUN_RECOIL_FIRST, "RecoilFirst", f32, COLD)

DECLARE_ARCHETYPE(Gun, GUN_FIELDS)
// <DRUID_GEN_END Gun>
#ifdef __cplusplus
}
#endif
