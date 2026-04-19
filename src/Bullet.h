#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BULLET_FIELDS(FIELD)                                                  \
    FIELD(BF_ALIVE,      "Alive",            b8,   COLD)                     \
    FIELD(BF_POS_X,      "PositionX",        f32,  HOT)                      \
    FIELD(BF_POS_Y,      "PositionY",        f32,  HOT)                      \
    FIELD(BF_POS_Z,      "PositionZ",        f32,  HOT)                      \
    FIELD(BF_ROT,        "Rotation",         Vec4, HOT)                      \
    FIELD(BF_SCALE,      "Scale",            Vec3, HOT)                      \
    FIELD(BF_VEL_X,      "LinearVelocityX",  f32,  HOT)                      \
    FIELD(BF_VEL_Y,      "LinearVelocityY",  f32,  HOT)                      \
    FIELD(BF_VEL_Z,      "LinearVelocityZ",  f32,  HOT)                      \
    FIELD(BF_FORCE_X,    "ForceX",           f32,  HOT)                      \
    FIELD(BF_FORCE_Y,    "ForceY",           f32,  HOT)                      \
    FIELD(BF_FORCE_Z,    "ForceZ",           f32,  HOT)                      \
    FIELD(BF_BODY_TYPE,  "PhysicsBodyType",  u32,  HOT)                      \
    FIELD(BF_MASS,       "Mass",             f32,  HOT)                      \
    FIELD(BF_INV_MASS,   "InvMass",          f32,  HOT)                      \
    FIELD(BF_RESTITUTION,"Restitution",      f32,  HOT)                      \
    FIELD(BF_DAMPING,    "LinearDamping",    f32,  HOT)                      \
    FIELD(BF_SPHERE_R,   "SphereRadius",     f32,  HOT)                      \
    FIELD(BF_HALF_X,     "ColliderHalfX",    f32,  HOT)                      \
    FIELD(BF_HALF_Y,     "ColliderHalfY",    f32,  HOT)                      \
    FIELD(BF_HALF_Z,     "ColliderHalfZ",    f32,  HOT)                      \
    FIELD(BF_LIFETIME,   "Lifetime",         f32,  HOT)                      \
    FIELD(BF_MODEL_ID,   "ModelID",          u32,  COLD)

DECLARE_ARCHETYPE(Bullet, BULLET_FIELDS)

DSAPI void bulletInit(Archetype *arch);
DSAPI void bulletUpdate(Archetype *arch, f32 dt);
DSAPI void bulletDestroy(void);

DSAPI void bulletSpawn(Vec3 position, Vec3 direction, f32 speed);
DSAPI Archetype *bulletGetArchetype(void);
DSAPI void druidGetECSSystem_Bullet(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif
