#include "Player.h"
#include "Bullet.h"
#include "Gun.h"
#include <stdlib.h>

#define CAM_ROTATE_SPEED   1.0f
#define MOVE_SPEED         8.0f
#define SPRINT_MULTIPLIER  2.0f
#define JUMP_FORCE         6.0f
#define EYE_HEIGHT         1.6f

#define WEAPON_PISTOL  0
#define WEAPON_AK47    1

#define PISTOL_COOLDOWN    0.15f
#define PISTOL_AMMO        9.0f
#define PISTOL_RELOAD      1.8f
#define PISTOL_SPEED       120.0f
#define PISTOL_SPREAD_MAX  0.01f
#define PISTOL_RECOIL      0.03f
#define PISTOL_RECOIL_FIRST 0.035f

#define AK_COOLDOWN        0.1f
#define AK_AMMO            30.0f
#define AK_RELOAD          3.1f
#define AK_SPEED           180.0f
#define AK_SPREAD_MAX      0.06f
#define AK_SPREAD_RATE     0.012f
#define AK_SPREAD_DECAY    0.08f
#define AK_RECOIL          0.05f
#define AK_RECOIL_FIRST    0.10f

#define RECOIL_RECOVERY_SPEED 2.0f

static b8 s_wasEscDown = false;
static b8 s_physicsInitialized = false;
static b8 s_mouseCapturedOnStart = false;

// Field order must match the indices used in playerUpdate.
DSAPI FieldInfo Player_fields[] = {
    { "PositionX", sizeof(f32), FIELD_TEMP_HOT },
    { "PositionY", sizeof(f32), FIELD_TEMP_HOT },
    { "PositionZ", sizeof(f32), FIELD_TEMP_HOT },
    { "Rotation", sizeof(Vec4), FIELD_TEMP_HOT },
    { "Scale", sizeof(Vec3), FIELD_TEMP_HOT },
    { "LinearVelocityX", sizeof(f32), FIELD_TEMP_COLD },
    { "LinearVelocityY", sizeof(f32), FIELD_TEMP_COLD },
    { "LinearVelocityZ", sizeof(f32), FIELD_TEMP_COLD },
    { "ForceX", sizeof(f32), FIELD_TEMP_COLD },
    { "ForceY", sizeof(f32), FIELD_TEMP_COLD },
    { "ForceZ", sizeof(f32), FIELD_TEMP_COLD },
    { "PhysicsBodyType", sizeof(u32), FIELD_TEMP_COLD },
    { "Mass", sizeof(f32), FIELD_TEMP_COLD },
    { "InvMass", sizeof(f32), FIELD_TEMP_COLD },
    { "Restitution", sizeof(f32), FIELD_TEMP_COLD },
    { "LinearDamping", sizeof(f32), FIELD_TEMP_COLD },
    { "SphereRadius", sizeof(f32), FIELD_TEMP_COLD },
    { "ColliderHalfX", sizeof(f32), FIELD_TEMP_COLD },
    { "ColliderHalfY", sizeof(f32), FIELD_TEMP_COLD },
    { "ColliderHalfZ", sizeof(f32), FIELD_TEMP_COLD },
    { "Yaw", sizeof(f32), FIELD_TEMP_COLD },
    { "Pitch", sizeof(f32), FIELD_TEMP_COLD },
    { "IsGrounded", sizeof(b8), FIELD_TEMP_COLD },
    { "ModelID", sizeof(u32), FIELD_TEMP_COLD },
    { "WeaponType", sizeof(u32), FIELD_TEMP_COLD },
    { "FireCooldown", sizeof(f32), FIELD_TEMP_COLD },
    { "CurrentSpread", sizeof(f32), FIELD_TEMP_COLD },
    { "RecoilRecovery", sizeof(f32), FIELD_TEMP_COLD },
    { "WasFireDown", sizeof(b8), FIELD_TEMP_COLD },
    { "ReloadCooldown", sizeof(f32), FIELD_TEMP_COLD},
    { "AmmoPistol", sizeof(f32), FIELD_TEMP_COLD},
    { "AmmoAK", sizeof(f32), FIELD_TEMP_COLD},
    { "HasReloaded", sizeof(b8), FIELD_TEMP_COLD}
};

DSAPI StructLayout Player_layout = {
    "Player",
    Player_fields,
    sizeof(Player_fields) / sizeof(FieldInfo)
};

// DRUID_FLAGS 0x05
// isSingle
// isPhysicsBody

static u32 s_ibSlot = (u32)-1;

void playerInit(void)
{
    s_ibSlot = rendererAcquireInstanceBuffer(renderer, 1024);
}

void playerUpdate(Archetype *arch, f32 dt)
{
    void **fields = getArchetypeFields(arch, 0);
    if (!fields || arch->arena[0].count == 0) return;

    f32  *PosX    = (f32 *)fields[0];
    f32  *PosY    = (f32 *)fields[1];
    f32  *PosZ    = (f32 *)fields[2];
    Vec4 *Rot     = (Vec4 *)fields[3];
    f32  *VelX    = (f32 *)fields[5];
    f32  *VelY    = (f32 *)fields[6];
    f32  *VelZ    = (f32 *)fields[7];
    f32  *Yaw     = (f32 *)fields[20];
    f32  *Pitch   = (f32 *)fields[21];
    b8   *IsGnd   = (b8 *)fields[22];
    u32  *Weapon  = (u32 *)fields[24];
    f32  *FirCD   = (f32 *)fields[25];
    f32  *Spread  = (f32 *)fields[26];
    f32  *RecoilR = (f32 *)fields[27];
    b8   *WasFire = (b8 *)fields[28];
    f32 *ReloadCD = (f32 *)fields[29];
    f32 *AmmoPistol = (f32 *)fields[30];
    f32 *AmmoAK = (f32 *)fields[31];
    b8 *HasReloaded = (b8 *)fields[32];

    // Hide local player mesh in first-person.
    u32 *ModelID = (u32 *)fields[23];
    ModelID[0] = (u32)-1;

    // Ensure physics defaults once.
    if (!s_physicsInitialized)
    {
        u32  *BodyType   = (u32 *)fields[11];
        f32  *Mass       = (f32 *)fields[12];
        f32  *InvMass    = (f32 *)fields[13];
        f32  *Restit     = (f32 *)fields[14];
        f32  *Damp       = (f32 *)fields[15];
        f32  *ColHalfX   = (f32 *)fields[17];
        f32  *ColHalfY   = (f32 *)fields[18];
        f32  *ColHalfZ   = (f32 *)fields[19];

        BodyType[0] = PHYS_BODY_DYNAMIC;
        Mass[0]     = 80.0f;
        InvMass[0]  = 1.0f / 80.0f;
        Restit[0]   = 0.0f;
        Damp[0]     = 0.1f;
        ColHalfX[0] = 0.4f;
        ColHalfY[0] = 0.9f;
        ColHalfZ[0] = 0.4f;

        s_physicsInitialized = true;
        INFO("Player physics init: bt=%u mass=%.1f invMass=%.4f damp=%.2f half=(%.2f,%.2f,%.2f)",
             BodyType[0], Mass[0], InvMass[0], Damp[0], ColHalfX[0], ColHalfY[0], ColHalfZ[0]);
    }

    // Capture mouse on first frame.
    if (!s_mouseCapturedOnStart)
    {
        setMouseCaptured(true);
        s_mouseCapturedOnStart = true;
    }

    // Escape toggles mouse capture.
    b8 escDown = isKeyDown(KEY_ESCAPE);
    if (escDown && !s_wasEscDown)
        setMouseCaptured(!isMouseCaptured());
    s_wasEscDown = escDown;

    // Camera look input.
    Yaw[0]   += xLookAxis * CAM_ROTATE_SPEED * dt;
    Pitch[0] += yLookAxis * CAM_ROTATE_SPEED * dt;
    Pitch[0]  = clamp(Pitch[0], -radians(89.0f), radians(89.0f));

    // Recoil recovery.
    if (RecoilR[0] > 0.0f)
    {
        f32 recover = RECOIL_RECOVERY_SPEED * dt;
        if (recover > RecoilR[0]) recover = RecoilR[0];
        Pitch[0]  -= recover;
        RecoilR[0] -= recover;
    }

    // Build orientation from yaw/pitch.
    Vec4 yawQ   = quatFromAxisAngle(v3Up, Yaw[0]);
    Vec4 pitchQ = quatFromAxisAngle(v3Right, Pitch[0]);
    Rot[0] = quatNormalize(quatMul(yawQ, pitchQ));

    // Movement input.
    Vec3 forward = quatRotateVec3(Rot[0], v3Forward);
    Vec3 right   = quatRotateVec3(Rot[0], v3Right);

    // Keep movement on XZ plane.
    forward.y = 0.0f; forward = v3Norm(forward);
    right.y   = 0.0f; right   = v3Norm(right);

    f32 speed = MOVE_SPEED;
    if (isKeyDown(KEY_LSHIFT) || isButtonDown(0, BUTTON_LEFTSTICK)) speed *= SPRINT_MULTIPLIER;

    Vec3 move = v3Add(v3Scale(right, -xInputAxis), v3Scale(forward, yInputAxis));
    f32 len = v3Mag(move);
    if (len > 0.001f)
        move = v3Scale(move, speed / len);
    else
        move = (Vec3){0, 0, 0};

    // Let physics keep vertical velocity.
    VelX[0] = move.x;
    VelZ[0] = move.z;

    // Ground check ray.
    f32 halfH = 0.9f;
    f32 skinDist = 0.15f;
    IsGnd[0] = false;
    if (physicsWorld)
    {
        PhysRay ray;
        ray.origin    = (Vec3){PosX[0], PosY[0], PosZ[0]};
        ray.direction = (Vec3){0.0f, -1.0f, 0.0f};
        ray.maxDistance = halfH + skinDist;
        ray.layerMask  = 0xFFFFFFFF;
        PhysRayHit hit = physRaycast(physicsWorld, ray);
        if (hit.hit)
            IsGnd[0] = true;
    }

    // Jump: use south-face button mapping (Cross/X) instead of east-face (Circle/B).
    if (IsGnd[0] && (isKeyDown(KEY_SPACE) || isButtonDown(0, BUTTON_CROSS)))
        VelY[0] = JUMP_FORCE;

    // Weapon switch.
    if (isKeyDown(KEY_1) || isButtonDown(0, BUTTON_LEFTSHOULDER) && Weapon[0] != WEAPON_PISTOL) 
    { 
        Weapon[0] = WEAPON_PISTOL;
        AmmoPistol[0] = PISTOL_AMMO;
        HasReloaded[0] = true;
    }
    if (isKeyDown(KEY_2) || isButtonDown(0, BUTTON_RIGHTSHOULDER) && Weapon[0] != WEAPON_AK47) 
    {
        Weapon[0] = WEAPON_AK47;
        AmmoAK[0] = AK_AMMO;
        HasReloaded[0] = true;
    }

    // Fire cooldown.
    if (FirCD[0] > 0.0f)
        FirCD[0] -= dt;

    // Reload 
    if (isKeyDown(KEY_R) && HasReloaded[0])
    {
        if (Weapon[0] == WEAPON_AK47) ReloadCD[0] += AK_RELOAD;
        if (Weapon[0] == WEAPON_PISTOL) ReloadCD[0] += PISTOL_RELOAD;
    }

    // Reload cooldown
    if (ReloadCD[0] > 0.0f)
        {
            ReloadCD[0] -= dt;
            HasReloaded[0] = false;
        }

    if (ReloadCD[0] <= 0.0f && !HasReloaded[0]) 
    {
        ReloadCD[0] = 0.0f;
        if (Weapon[0] == WEAPON_PISTOL) 
        {
            AmmoPistol[0] = PISTOL_AMMO;
        }

        if (Weapon[0] == WEAPON_AK47)
        {
            AmmoAK[0] = AK_AMMO;
        }

        HasReloaded[0] = true;
    }

    // Shooting.
    Vec2 triggers = getJoystickAxis(0, JOYSTICK_TRIGGER_LEFT, JOYSTICK_TRIGGER_RIGHT);
    // Include keyboard fallback and both left-button encodings for compatibility.
    b8 fireDown = isMouseDown(MOUSE_LEFT) || isMouseDown(MOUSE_LEFT + 1) ||
                  isKeyDown(KEY_LCTRL) || isKeyDown(KEY_RCTRL) ||
                  (triggers.y < -0.5f);

    b8 canFire = false;
    if (Weapon[0] == WEAPON_PISTOL)
        canFire = fireDown && !WasFire[0] && FirCD[0] <= 0.0f && HasReloaded[0];
    else
        canFire = fireDown && FirCD[0] <= 0.0f && HasReloaded[0];

    if (canFire)
    {
        f32 bulletSpeed = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_SPEED : AK_SPEED;
        f32 cooldown    = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_COOLDOWN : AK_COOLDOWN;
        f32 recoilKick  = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_RECOIL : AK_RECOIL;
        f32 firstRecoil = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_RECOIL_FIRST : AK_RECOIL_FIRST;


        f32 spreadX = Spread[0] * ((f32)(rand() % 2001 - 1000) / 1000.0f);
        f32 spreadY = Spread[0] * ((f32)(rand() % 2001 - 1000) / 1000.0f);

        Vec3 dir = quatRotateVec3(Rot[0], v3Forward);
        dir.x += spreadX;
        dir.y += spreadY;
        dir = v3Norm(dir);

        Vec3 eyePos = {PosX[0], PosY[0] + EYE_HEIGHT, PosZ[0]};
        Vec3 spawnPos = v3Add(eyePos, v3Scale(dir, 0.5f));


        bulletSpawn(spawnPos, dir, bulletSpeed);

        FirCD[0] = cooldown;
        if (Spread[0] == 0.0f) 
        {
            RecoilR[0] += firstRecoil;
            Pitch[0] += firstRecoil;
        }
        else
        {
            RecoilR[0] += recoilKick;
            Pitch[0]   += recoilKick;
        }

        if (Weapon[0] == WEAPON_AK47)
        {
            Spread[0] += AK_SPREAD_RATE;
            if (Spread[0] > AK_SPREAD_MAX) Spread[0] = AK_SPREAD_MAX;
            AmmoAK[0] -= 1;
            if (AmmoAK[0] == 0) ReloadCD[0] += AK_RELOAD;
        }
        else
        {
            Spread[0] = PISTOL_SPREAD_MAX;
            AmmoPistol[0] -= 1;
            if (AmmoPistol[0] == 0) ReloadCD[0] += PISTOL_RELOAD;
        }
    }

    if (!fireDown && Spread[0] > 0.0f)
    {
        Spread[0] -= AK_SPREAD_DECAY * dt;
        if (Spread[0] < 0.0f) Spread[0] = 0.0f;
    }

    

    WasFire[0] = fireDown;

    // Sync camera to eye position.
    if (renderer)
    {
        Camera *cam = rendererGetCamera(renderer, renderer->activeCamera);
        if (cam)
        {
            cam->pos         = (Vec3){PosX[0], PosY[0] + EYE_HEIGHT, PosZ[0]};
            cam->orientation = Rot[0];
        }
    }
}

// Use default renderer by leaving out->render = NULL.

void playerDestroy(void)
{
    if (s_ibSlot != (u32)-1) { rendererReleaseInstanceBuffer(renderer, s_ibSlot); s_ibSlot = (u32)-1; }
    setMouseCaptured(false);
    s_physicsInitialized = false;
    s_mouseCapturedOnStart = false;
}

void druidGetECSSystem_Player(ECSSystemPlugin *out)
{
    out->init    = playerInit;
    out->update  = playerUpdate;
    out->render  = NULL;
    out->destroy = playerDestroy;
}
