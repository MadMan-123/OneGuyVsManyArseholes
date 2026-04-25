#pragma once
#include <druid.h>

#define WEAPON_PISTOL  0
#define WEAPON_AK47    1

#ifdef __cplusplus
extern "C" {
#endif

DSAPI void playerInit(void);
DSAPI void playerUpdate(Archetype *arch, f32 dt);
DSAPI void playerRender(Archetype *arch, Renderer *r);
DSAPI void playerDestroy(void);
DSAPI void druidGetECSSystem_Player(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
// <DRUID_GEN_BEGIN Player>
// DRUID_FLAGS 0x05
// isSingle
// isPhysicsBody

#define PLAYER_FIELDS(FIELD) \
    FIELD(PLAYER_POSITION_X, "PositionX", f32, HOT) \
    FIELD(PLAYER_POSITION_Y, "PositionY", f32, HOT) \
    FIELD(PLAYER_POSITION_Z, "PositionZ", f32, HOT) \
    FIELD(PLAYER_ROTATION, "Rotation", Vec4, HOT) \
    FIELD(PLAYER_SCALE, "Scale", Vec3, HOT) \
    FIELD(PLAYER_LINEAR_VELOCITY_X, "LinearVelocityX", f32, COLD) \
    FIELD(PLAYER_LINEAR_VELOCITY_Y, "LinearVelocityY", f32, COLD) \
    FIELD(PLAYER_LINEAR_VELOCITY_Z, "LinearVelocityZ", f32, COLD) \
    FIELD(PLAYER_FORCE_X, "ForceX", f32, COLD) \
    FIELD(PLAYER_FORCE_Y, "ForceY", f32, COLD) \
    FIELD(PLAYER_FORCE_Z, "ForceZ", f32, COLD) \
    FIELD(PLAYER_PHYSICS_BODY_TYPE, "PhysicsBodyType", u32, COLD) \
    FIELD(PLAYER_MASS, "Mass", f32, COLD) \
    FIELD(PLAYER_INV_MASS, "InvMass", f32, COLD) \
    FIELD(PLAYER_RESTITUTION, "Restitution", f32, COLD) \
    FIELD(PLAYER_LINEAR_DAMPING, "LinearDamping", f32, COLD) \
    FIELD(PLAYER_SPHERE_RADIUS, "SphereRadius", f32, COLD) \
    FIELD(PLAYER_COLLIDER_HALF_X, "ColliderHalfX", f32, COLD) \
    FIELD(PLAYER_COLLIDER_HALF_Y, "ColliderHalfY", f32, COLD) \
    FIELD(PLAYER_COLLIDER_HALF_Z, "ColliderHalfZ", f32, COLD) \
    FIELD(PLAYER_YAW, "Yaw", f32, COLD) \
    FIELD(PLAYER_PITCH, "Pitch", f32, COLD) \
    FIELD(PLAYER_IS_GROUNDED, "IsGrounded", b8, COLD) \
    FIELD(PLAYER_MODEL_ID, "ModelID", u32, COLD) \
    FIELD(PLAYER_WEAPON_TYPE, "WeaponType", u32, COLD) \
    FIELD(PLAYER_FIRE_COOLDOWN, "FireCooldown", f32, COLD) \
    FIELD(PLAYER_CURRENT_SPREAD, "CurrentSpread", f32, COLD) \
    FIELD(PLAYER_RECOIL_RECOVERY, "RecoilRecovery", f32, COLD) \
    FIELD(PLAYER_WAS_FIRE_DOWN, "WasFireDown", b8, COLD) \
    FIELD(PLAYER_RELOAD_COOLDOWN, "ReloadCooldown", f32, COLD) \
    FIELD(PLAYER_AMMO_PISTOL, "AmmoPistol", f32, COLD) \
    FIELD(PLAYER_AMMO_AK, "AmmoAK", f32, COLD) \
    FIELD(PLAYER_HAS_RELOADED, "HasReloaded", b8, COLD) \
    FIELD(PLAYER_IS_AIMING, "IsAiming", b8, COLD) \
    FIELD(PLAYER_HEALTH_ID, "HealthID", u32, COLD) \
    FIELD(PLAYER_COLLIDER_OFFSET_X, "ColliderOffsetX", f32, COLD) \
    FIELD(PLAYER_COLLIDER_OFFSET_Y, "ColliderOffsetY", f32, COLD) \
    FIELD(PLAYER_COLLIDER_OFFSET_Z, "ColliderOffsetZ", f32, COLD)

DECLARE_ARCHETYPE(Player, PLAYER_FIELDS)
// <DRUID_GEN_END Player>
#ifdef __cplusplus
}
#endif
