#pragma once
#include <druid.h>


#define WEAPON_PISTOL  0
#define WEAPON_AK47    1
#define WEAPON_SUOMI   2


#ifdef __cplusplus
extern "C" {
#endif

#define PLAYER_FIELDS(FIELD)                                                  \
    FIELD(PF_POS_X,      "PositionX",       f32,  HOT)                       \
    FIELD(PF_POS_Y,      "PositionY",       f32,  HOT)                       \
    FIELD(PF_POS_Z,      "PositionZ",       f32,  HOT)                       \
    FIELD(PF_ROT,        "Rotation",        Vec4, HOT)                       \
    FIELD(PF_SCALE,      "Scale",           Vec3, HOT)                       \
    FIELD(PF_VEL_X,      "LinearVelocityX", f32,  COLD)                      \
    FIELD(PF_VEL_Y,      "LinearVelocityY", f32,  COLD)                      \
    FIELD(PF_VEL_Z,      "LinearVelocityZ", f32,  COLD)                      \
    FIELD(PF_FORCE_X,    "ForceX",          f32,  COLD)                      \
    FIELD(PF_FORCE_Y,    "ForceY",          f32,  COLD)                      \
    FIELD(PF_FORCE_Z,    "ForceZ",          f32,  COLD)                      \
    FIELD(PF_BODY_TYPE,  "PhysicsBodyType", u32,  COLD)                      \
    FIELD(PF_MASS,       "Mass",            f32,  COLD)                      \
    FIELD(PF_INV_MASS,   "InvMass",         f32,  COLD)                      \
    FIELD(PF_RESTITUTION,"Restitution",     f32,  COLD)                      \
    FIELD(PF_DAMPING,    "LinearDamping",   f32,  COLD)                      \
    FIELD(PF_SPHERE_R,   "SphereRadius",    f32,  COLD)                      \
    FIELD(PF_HALF_X,     "ColliderHalfX",   f32,  COLD)                      \
    FIELD(PF_HALF_Y,     "ColliderHalfY",   f32,  COLD)                      \
    FIELD(PF_HALF_Z,     "ColliderHalfZ",   f32,  COLD)                      \
    FIELD(PF_YAW,        "Yaw",             f32,  COLD)                      \
    FIELD(PF_PITCH,      "Pitch",           f32,  COLD)                      \
    FIELD(PF_IS_GROUNDED,"IsGrounded",      b8,   COLD)                      \
    FIELD(PF_MODEL_ID,   "ModelID",         u32,  COLD)                      \
    FIELD(PF_WEAPON,     "WeaponType",      u32,  COLD)                      \
    FIELD(PF_FIRE_CD,    "FireCooldown",    f32,  COLD)                      \
    FIELD(PF_SPREAD,     "CurrentSpread",   f32,  COLD)                      \
    FIELD(PF_RECOIL_R,   "RecoilRecovery",  f32,  COLD)                      \
    FIELD(PF_WAS_FIRE,   "WasFireDown",     b8,   COLD)                       \
    FIELD(PF_RELOAD_CD,  "ReloadCooldown",  f32,  COLD)                       \
    FIELD(PF_AMMO_PISTOL,"AmmoPistol",      f32,  COLD)                       \
    FIELD(PF_AMMO_AK,    "AmmoAK",          f32,  COLD)                       \
    FIELD(PF_HAS_RELOADED,"HasReloaded",    b8,   COLD)                       \
    FIELD(PF_IS_AIMING,  "IsAiming",       b8,   COLD)                       \
    FIELD(PF_HEALTH_ID,  "HealthID",       u32,  COLD)                       

DECLARE_ARCHETYPE(Player, PLAYER_FIELDS)

DSAPI void playerInit(void);
DSAPI void playerUpdate(Archetype *arch, f32 dt);
DSAPI void playerRender(Archetype *arch, Renderer *r);
DSAPI void playerDestroy(void);
DSAPI void druidGetECSSystem_Player(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif
