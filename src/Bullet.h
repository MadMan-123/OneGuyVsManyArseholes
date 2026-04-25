#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

DSAPI void bulletInit(Archetype *arch);
DSAPI void bulletUpdate(Archetype *arch, f32 dt);
DSAPI void bulletDestroy(void);

DSAPI void bulletSpawn(Vec3 position, Vec3 direction, f32 speed);
DSAPI Archetype *bulletGetArchetype(void);
DSAPI void druidGetECSSystem_Bullet(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
// <DRUID_GEN_BEGIN Bullet>
// DRUID_FLAGS 0x0C
// isBuffered
// isPhysicsBody
#define POOL_CAPACITY 256

#define BULLET_FIELDS(FIELD) \
    FIELD(BULLET_ALIVE, "Alive", b8, COLD) \
    FIELD(BULLET_POSITION_X, "PositionX", f32, HOT) \
    FIELD(BULLET_POSITION_Y, "PositionY", f32, HOT) \
    FIELD(BULLET_POSITION_Z, "PositionZ", f32, HOT) \
    FIELD(BULLET_ROTATION, "Rotation", Vec4, HOT) \
    FIELD(BULLET_SCALE, "Scale", Vec3, HOT) \
    FIELD(BULLET_LINEAR_VELOCITY_X, "LinearVelocityX", f32, COLD) \
    FIELD(BULLET_LINEAR_VELOCITY_Y, "LinearVelocityY", f32, COLD) \
    FIELD(BULLET_LINEAR_VELOCITY_Z, "LinearVelocityZ", f32, COLD) \
    FIELD(BULLET_FORCE_X, "ForceX", f32, COLD) \
    FIELD(BULLET_FORCE_Y, "ForceY", f32, COLD) \
    FIELD(BULLET_FORCE_Z, "ForceZ", f32, COLD) \
    FIELD(BULLET_PHYSICS_BODY_TYPE, "PhysicsBodyType", u32, COLD) \
    FIELD(BULLET_MASS, "Mass", f32, COLD) \
    FIELD(BULLET_INV_MASS, "InvMass", f32, COLD) \
    FIELD(BULLET_RESTITUTION, "Restitution", f32, COLD) \
    FIELD(BULLET_LINEAR_DAMPING, "LinearDamping", f32, COLD) \
    FIELD(BULLET_SPHERE_RADIUS, "SphereRadius", f32, COLD) \
    FIELD(BULLET_COLLIDER_HALF_X, "ColliderHalfX", f32, COLD) \
    FIELD(BULLET_COLLIDER_HALF_Y, "ColliderHalfY", f32, COLD) \
    FIELD(BULLET_COLLIDER_HALF_Z, "ColliderHalfZ", f32, COLD) \
    FIELD(BULLET_LIFETIME, "Lifetime", f32, COLD) \
    FIELD(BULLET_MODEL_ID, "ModelID", u32, COLD) \
    FIELD(BULLET_CCD_ENABLED, "CCDEnabled", b8, COLD) \
    FIELD(BULLET_COLLIDER_OFFSET_X, "ColliderOffsetX", f32, COLD) \
    FIELD(BULLET_COLLIDER_OFFSET_Y, "ColliderOffsetY", f32, COLD) \
    FIELD(BULLET_COLLIDER_OFFSET_Z, "ColliderOffsetZ", f32, COLD)

DECLARE_ARCHETYPE(Bullet, BULLET_FIELDS)
// <DRUID_GEN_END Bullet>
#ifdef __cplusplus
}
#endif
