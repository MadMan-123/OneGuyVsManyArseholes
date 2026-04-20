#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GUN_FIELDS(FIELD)                                               \
    FIELD(GF_POS_X,        "PositionX",    f32,  HOT)                  \
    FIELD(GF_POS_Y,        "PositionY",    f32,  HOT)                  \
    FIELD(GF_POS_Z,        "PositionZ",    f32,  HOT)                  \
    FIELD(GF_ROT,          "Rotation",     Vec4, HOT)                  \
    FIELD(GF_SCALE,        "Scale",        Vec3, HOT)                  \
    FIELD(GF_MODEL_ID,     "ModelID",      u32,  COLD)                 \
    FIELD(GF_COOLDOWN,     "Cooldown",     f32,  COLD)                 \
    FIELD(GF_AMMO,         "Ammo",         f32,  COLD)                 \
    FIELD(GF_RELOAD,       "ReloadTime",   f32,  COLD)                 \
    FIELD(GF_SPEED,        "BulletSpeed",  f32,  COLD)                 \
    FIELD(GF_SPREAD_MAX,   "SpreadMax",    f32,  COLD)                 \
    FIELD(GF_SPREAD_RATE,  "SpreadRate",   f32,  COLD)                 \
    FIELD(GF_SPREAD_DECAY, "SpreadDecay",  f32,  COLD)                 \
    FIELD(GF_RECOIL,       "Recoil",       f32,  COLD)                 \
    FIELD(GF_RECOIL_FIRST, "RecoilFirst",  f32,  COLD)

DECLARE_ARCHETYPE(Gun, GUN_FIELDS)

DSAPI void gunInit(Archetype *arch);
DSAPI void gunUpdate(Archetype *arch, f32 dt);
DSAPI void gunDestroy(void);
DSAPI void druidGetECSSystem_Gun(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif
