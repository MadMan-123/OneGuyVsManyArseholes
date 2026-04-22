#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_STATE_IDLE       0u
#define AI_STATE_AGITATED   1u
#define AI_STATE_CHASE      2u
#define AI_STATE_SCARED     3u

// Model faces backward — offset applied to yaw when setting rotation
#define ZOMBIE_YAW_OFFSET   3.14159265f

#define ENEMY_FIELDS(FIELD)                                                   \
    FIELD(EF_ALIVE,        "Alive",            b8,   COLD)                    \
    FIELD(EF_POS_X,        "PositionX",        f32,  HOT)                     \
    FIELD(EF_POS_Y,        "PositionY",        f32,  HOT)                     \
    FIELD(EF_POS_Z,        "PositionZ",        f32,  HOT)                     \
    FIELD(EF_ROT,          "Rotation",         Vec4, HOT)                     \
    FIELD(EF_SCALE,        "Scale",            Vec3, HOT)                     \
    FIELD(EF_VEL_X,        "LinearVelocityX",  f32,  HOT)                     \
    FIELD(EF_VEL_Y,        "LinearVelocityY",  f32,  HOT)                     \
    FIELD(EF_VEL_Z,        "LinearVelocityZ",  f32,  HOT)                     \
    FIELD(EF_FORCE_X,      "ForceX",           f32,  HOT)                     \
    FIELD(EF_FORCE_Y,      "ForceY",           f32,  HOT)                     \
    FIELD(EF_FORCE_Z,      "ForceZ",           f32,  HOT)                     \
    FIELD(EF_BODY_TYPE,    "PhysicsBodyType",  u32,  COLD)                    \
    FIELD(EF_MASS,         "Mass",             f32,  COLD)                    \
    FIELD(EF_INV_MASS,     "InvMass",          f32,  COLD)                    \
    FIELD(EF_RESTITUTION,  "Restitution",      f32,  COLD)                    \
    FIELD(EF_DAMPING,      "LinearDamping",    f32,  COLD)                    \
    FIELD(EF_SPHERE_R,     "SphereRadius",     f32,  COLD)                    \
    FIELD(EF_HALF_X,       "ColliderHalfX",    f32,  COLD)                    \
    FIELD(EF_HALF_Y,       "ColliderHalfY",    f32,  COLD)                    \
    FIELD(EF_HALF_Z,       "ColliderHalfZ",    f32,  COLD)                    \
    FIELD(EF_MODEL_ID,     "ModelID",          u32,  COLD)                    \
    FIELD(EF_STATE,        "AIState",          u32,  COLD)                    \
    FIELD(EF_PREV_STATE,   "AIPrevState",      u32,  COLD)                    \
    FIELD(EF_STATE_TIMER,  "AIStateTimer",     f32,  COLD)                    \
    FIELD(EF_HEALTH_ID,    "HealthID",         u32,  COLD)                    \
    FIELD(EF_ATTACK_CD,    "AttackCooldown",   f32,  COLD)                    \
    FIELD(EF_VISION_RANGE, "VisionRange",      f32,  COLD)                    \
    FIELD(EF_VISION_COS,   "VisionFovCos",     f32,  COLD)                    \
    FIELD(EF_HEARING,      "HearingRange",     f32,  COLD)                    \
    FIELD(EF_LAST_SEEN_X,   "LastSeenX",        f32,  COLD)                    \
    FIELD(EF_LAST_SEEN_Y,   "LastSeenY",        f32,  COLD)                    \
    FIELD(EF_LAST_SEEN_Z,   "LastSeenZ",        f32,  COLD)                    \
    FIELD(EF_LAST_SEEN_AGE, "LastSeenAge",      f32,  COLD)                    \
    FIELD(EF_WANDER_X,      "WanderTargetX",    f32,  COLD)                    \
    FIELD(EF_WANDER_Z,      "WanderTargetZ",    f32,  COLD)                    \
    FIELD(EF_WANDER_TIMER,  "WanderTimer",      f32,  COLD)                    \
    FIELD(EF_YAW,           "Yaw",              f32,  COLD)                    \
    FIELD(EF_IS_GROUNDED,   "IsGrounded",       b8,   COLD)

DECLARE_ARCHETYPE(Enemy, ENEMY_FIELDS)

DSAPI void enemyInit(Archetype *arch);
DSAPI void enemyUpdate(Archetype *arch, f32 dt);
DSAPI void enemyDestroy(void);
DSAPI u32  enemySpawnAt(Vec3 position);   // returns poolIdx, (u32)-1 on failure
DSAPI Archetype *enemyGetArchetype(void);

#ifdef __cplusplus
}
#endif
