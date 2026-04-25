#pragma once
#include <druid.h>

#define AI_STATE_IDLE       0u
#define AI_STATE_AGITATED   1u
#define AI_STATE_CHASE      2u
#define AI_STATE_SCARED     3u

// Model faces backward — offset applied to yaw when setting rotation
#define ZOMBIE_YAW_OFFSET   3.14159265f

#ifdef __cplusplus
extern "C" {
#endif

DSAPI void enemyInit(Archetype *arch);
DSAPI void enemyUpdate(Archetype *arch, f32 dt);
DSAPI void enemyDestroy(void);
DSAPI u32  enemySpawnAt(Vec3 position);   // returns poolIdx, (u32)-1 on failure
DSAPI void enemyInitFields(void **fields, u32 i, Vec3 position); // reset runtime state after prefab stamp
DSAPI Archetype *enemyGetArchetype(void);
DSAPI u32  enemyGetModelID(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
// <DRUID_GEN_BEGIN Enemy>
// DRUID_FLAGS 0x0C
// isBuffered
// isPhysicsBody
#define POOL_CAPACITY 100

#define ENEMY_FIELDS(FIELD) \
    FIELD(ENEMY_ALIVE, "Alive", b8, COLD) \
    FIELD(ENEMY_POSITION_X, "PositionX", f32, HOT) \
    FIELD(ENEMY_POSITION_Y, "PositionY", f32, HOT) \
    FIELD(ENEMY_POSITION_Z, "PositionZ", f32, HOT) \
    FIELD(ENEMY_ROTATION, "Rotation", Vec4, HOT) \
    FIELD(ENEMY_SCALE, "Scale", Vec3, HOT) \
    FIELD(ENEMY_LINEAR_VELOCITY_X, "LinearVelocityX", f32, COLD) \
    FIELD(ENEMY_LINEAR_VELOCITY_Y, "LinearVelocityY", f32, COLD) \
    FIELD(ENEMY_LINEAR_VELOCITY_Z, "LinearVelocityZ", f32, COLD) \
    FIELD(ENEMY_FORCE_X, "ForceX", f32, COLD) \
    FIELD(ENEMY_FORCE_Y, "ForceY", f32, COLD) \
    FIELD(ENEMY_FORCE_Z, "ForceZ", f32, COLD) \
    FIELD(ENEMY_PHYSICS_BODY_TYPE, "PhysicsBodyType", u32, COLD) \
    FIELD(ENEMY_MASS, "Mass", f32, COLD) \
    FIELD(ENEMY_INV_MASS, "InvMass", f32, COLD) \
    FIELD(ENEMY_RESTITUTION, "Restitution", f32, COLD) \
    FIELD(ENEMY_LINEAR_DAMPING, "LinearDamping", f32, COLD) \
    FIELD(ENEMY_SPHERE_RADIUS, "SphereRadius", f32, COLD) \
    FIELD(ENEMY_COLLIDER_HALF_X, "ColliderHalfX", f32, COLD) \
    FIELD(ENEMY_COLLIDER_HALF_Y, "ColliderHalfY", f32, COLD) \
    FIELD(ENEMY_COLLIDER_HALF_Z, "ColliderHalfZ", f32, COLD) \
    FIELD(ENEMY_COLLIDER_OFFSET_Y, "ColliderOffsetY", f32, COLD) \
    FIELD(ENEMY_MODEL_ID, "ModelID", u32, COLD) \
    FIELD(ENEMY_AI_STATE, "AIState", u32, COLD) \
    FIELD(ENEMY_AI_PREV_STATE, "AIPrevState", u32, COLD) \
    FIELD(ENEMY_AI_STATE_TIMER, "AIStateTimer", f32, COLD) \
    FIELD(ENEMY_HEALTH_ID, "HealthID", u32, COLD) \
    FIELD(ENEMY_ATTACK_COOLDOWN, "AttackCooldown", f32, COLD) \
    FIELD(ENEMY_VISION_RANGE, "VisionRange", f32, COLD) \
    FIELD(ENEMY_VISION_FOV_COS, "VisionFovCos", f32, COLD) \
    FIELD(ENEMY_HEARING_RANGE, "HearingRange", f32, COLD) \
    FIELD(ENEMY_LAST_SEEN_X, "LastSeenX", f32, COLD) \
    FIELD(ENEMY_LAST_SEEN_Y, "LastSeenY", f32, COLD) \
    FIELD(ENEMY_LAST_SEEN_Z, "LastSeenZ", f32, COLD) \
    FIELD(ENEMY_LAST_SEEN_AGE, "LastSeenAge", f32, COLD) \
    FIELD(ENEMY_WANDER_TARGET_X, "WanderTargetX", f32, COLD) \
    FIELD(ENEMY_WANDER_TARGET_Z, "WanderTargetZ", f32, COLD) \
    FIELD(ENEMY_WANDER_TIMER, "WanderTimer", f32, COLD) \
    FIELD(ENEMY_YAW, "Yaw", f32, COLD) \
    FIELD(ENEMY_IS_GROUNDED, "IsGrounded", b8, COLD) \
    FIELD(ENEMY_COLLIDER_OFFSET_X, "ColliderOffsetX", f32, COLD) \
    FIELD(ENEMY_COLLIDER_OFFSET_Z, "ColliderOffsetZ", f32, COLD)

DECLARE_ARCHETYPE(Enemy, ENEMY_FIELDS)
// <DRUID_GEN_END Enemy>
#ifdef __cplusplus
}
#endif
