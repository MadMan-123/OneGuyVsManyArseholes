#include "Player.h"
#include "Bullet.h"
#include "GameConfig.h"
#include "game.h"
#include <stdlib.h>

#define CAM_ROTATE_SPEED      1.0f
#define MOVE_SPEED            8.0f
#define SPRINT_MULTIPLIER     2.0f
#define JUMP_FORCE            6.0f
#define EYE_HEIGHT            1.6f
#define RECOIL_RECOVERY_SPEED 2.0f

#define BTN_FIRE MOUSE_RIGHT
#define BTN_ADS  MOUSE_X1

#define FOV_NORMAL 70.0f
#define FOV_ADS    45.0f

// Per-weapon muzzle tip offsets (hip)
#define MUZZLE_PISTOL_HIP_X  0.30f
#define MUZZLE_PISTOL_HIP_Y -0.25f
#define MUZZLE_PISTOL_HIP_Z -0.90f
#define MUZZLE_AK_HIP_X      0.10f
#define MUZZLE_AK_HIP_Y     -0.20f
#define MUZZLE_AK_HIP_Z     -1.00f
#define MUZZLE_SUOMI_HIP_X      0.10f
#define MUZZLE_SUOMI_HIP_Y     -0.20f
#define MUZZLE_SUOMI_HIP_Z     -1.00f
// Per-weapon muzzle tip offsets (ADS)
#define MUZZLE_PISTOL_ADS_X  0.00f
#define MUZZLE_PISTOL_ADS_Y -0.10f
#define MUZZLE_PISTOL_ADS_Z -0.90f
#define MUZZLE_AK_ADS_X      0.00f
#define MUZZLE_AK_ADS_Y     -0.10f
#define MUZZLE_AK_ADS_Z     -1.00f
#define MUZZLE_SUOMI_ADS_X      0.00f
#define MUZZLE_SUOMI_ADS_Y     -0.10f
#define MUZZLE_SUOMI_ADS_Z     -1.00f

#define WEAPON_PISTOL  0
#define WEAPON_AK47    1
#define WEAPON_SUOMI   2


DEFINE_ARCHETYPE(Player, PLAYER_FIELDS)

static b8  s_wasEscDown          = false;
static b8  s_mouseCapturedOnStart = false;
static f32 s_currentFov           = FOV_NORMAL;
static f32 s_cachedAspectRatio    = 16.0f / 9.0f;
static b8  s_wasYDown             = false;
static b8  s_wasXDown             = false;
static b8  s_wasSquareDown        = false;


//=============================================================================
// Helpers

static void playerLook(f32 *Yaw, f32 *Pitch, f32 *RecoilR, Vec4 *Rot, f32 dt)
{
    Yaw[0]   += xLookAxis * CAM_ROTATE_SPEED * dt;
    Pitch[0] += yLookAxis * CAM_ROTATE_SPEED * dt;
    Pitch[0]  = clamp(Pitch[0], -radians(89.0f), radians(89.0f));

    if (RecoilR[0] > 0.0f)
    {
        f32 recover = RECOIL_RECOVERY_SPEED * dt;
        if (recover > RecoilR[0]) recover = RecoilR[0];
        Pitch[0]   -= recover;
        RecoilR[0] -= recover;
    }

    Vec4 yawQ   = quatFromAxisAngle(v3Up, Yaw[0]);
    Vec4 pitchQ = quatFromAxisAngle(v3Right, Pitch[0]);
    Rot[0] = quatNormalize(quatMul(yawQ, pitchQ));
}

static void playerMove(f32 *VelX, f32 *VelY, f32 *VelZ, b8 *IsGnd,
                       Vec4 Rot, f32 posX, f32 posY, f32 posZ)
{
    Vec3 forward = quatRotateVec3(Rot, v3Forward);
    Vec3 right   = quatRotateVec3(Rot, v3Right);
    forward.y = 0.0f; forward = v3Norm(forward);
    right.y   = 0.0f; right   = v3Norm(right);

    f32 speed = MOVE_SPEED;
    if (isKeyDown(KEY_LSHIFT) || isButtonDown(0, BUTTON_LEFTSTICK)) speed *= SPRINT_MULTIPLIER;

    Vec3 move = v3Add(v3Scale(forward, yInputAxis), v3Scale(right, xInputAxis));
    f32 len = v3Mag(move);
    move = (len > 0.001f) ? v3Scale(move, speed / len) : (Vec3){0, 0, 0};

    VelX[0] = move.x;
    VelZ[0] = move.z;

    IsGnd[0] = false;
    if (physicsWorld)
    {
        PhysRay ray = {
            .origin      = (Vec3){posX, posY, posZ},
            .direction   = (Vec3){0.0f, -1.0f, 0.0f},
            .maxDistance = 0.9f + 0.15f,
            .layerMask   = 0xFFFFFFFF,
        };
        if (physRaycast(physicsWorld, ray).hit) IsGnd[0] = true;
    }
    b8 xDown = isButtonDown(0, SDL_GAMEPAD_BUTTON_SOUTH);
    if (IsGnd[0] && ((isKeyDown(KEY_SPACE)) || xDown && !s_wasXDown))
    {
        VelY[0] = JUMP_FORCE;
    }
    s_wasXDown = xDown;
        
}

static void playerShoot(u32 *Weapon, f32 *FirCD, f32 *Spread, f32 *RecoilR, f32 *Pitch,
                        b8 *WasFire, f32 *ReloadCD, f32 *AmmoPistol, f32 *AmmoAK,
                        b8 *HasReloaded, Vec4 *Rot, f32 posX, f32 posY, f32 posZ,
                        b8 isAiming, f32 dt)
{
    if (FirCD[0] > 0.0f) FirCD[0] -= dt;

    // Reload cooldown
    if (ReloadCD[0] > 0.0f)
    {
        ReloadCD[0] -= dt;
        HasReloaded[0] = false;
    }
    if (ReloadCD[0] <= 0.0f && !HasReloaded[0])
    {
        ReloadCD[0] = 0.0f;
        if (Weapon[0] == WEAPON_PISTOL) AmmoPistol[0] = PISTOL_CLIP_SIZE;
        if (Weapon[0] == WEAPON_AK47)   AmmoAK[0]     = AK_CLIP_SIZE;
        if (Weapon[0] == WEAPON_SUOMI)  AmmoSuomi[0]  = SUOMI_CLIP_SIZE;
        HasReloaded[0] = true;
    }

    // Manual reload
    b8 squareDown = isButtonDown(0, SDL_GAMEPAD_BUTTON_WEST);
    if (HasReloaded[0] && (isKeyDown(KEY_R) || squareDown && !s_wasSquareDown))
    {
        ReloadCD[0] = (Weapon[0] == WEAPON_AK47) ? AK_RELOAD_TIME : PISTOL_RELOAD_TIME;
    }
    s_wasSquareDown = squareDown;

    Vec2 triggers = getJoystickAxis(0, JOYSTICK_TRIGGER_LEFT, JOYSTICK_TRIGGER_RIGHT);
    b8 fireDown = isMouseDown(BTN_FIRE) ||
                  isKeyDown(KEY_LCTRL) || isKeyDown(KEY_RCTRL) ||
                  (triggers.y < -0.5f);

    b8 canFire = HasReloaded[0] && FirCD[0] <= 0.0f &&
                 ((Weapon[0] == WEAPON_PISTOL) ? (fireDown && !WasFire[0]) : fireDown);

    if (canFire)
    {
        f32 bulletSpeed  = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_BULLET_SPEED        : AK_BULLET_SPEED;
        f32 cooldown     = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_FIRE_RATE     : AK_FIRE_RATE;
        f32 recoilKick   = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_RECOIL       : AK_RECOIL;
        f32 firstRecoil  = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_RECOIL_FIRST : AK_RECOIL_FIRST;

        f32 spreadX = Spread[0] * ((f32)(rand() % 2001 - 1000) / 1000.0f);
        f32 spreadY = Spread[0] * ((f32)(rand() % 2001 - 1000) / 1000.0f);
        Vec3 dir = v3Norm((Vec3){
            quatRotateVec3(Rot[0], v3Forward).x + spreadX,
            quatRotateVec3(Rot[0], v3Forward).y + spreadY,
            quatRotateVec3(Rot[0], v3Forward).z,
        });

        Vec3 eyePos = {posX, posY + EYE_HEIGHT, posZ};
        static const Vec3 muzzleHip[2] = {
            {MUZZLE_PISTOL_HIP_X, MUZZLE_PISTOL_HIP_Y, MUZZLE_PISTOL_HIP_Z},
            {MUZZLE_AK_HIP_X,     MUZZLE_AK_HIP_Y,     MUZZLE_AK_HIP_Z},
        };
        static const Vec3 muzzleADS[2] = {
            {MUZZLE_PISTOL_ADS_X, MUZZLE_PISTOL_ADS_Y, MUZZLE_PISTOL_ADS_Z},
            {MUZZLE_AK_ADS_X,     MUZZLE_AK_ADS_Y,     MUZZLE_AK_ADS_Z},
        };
        const Vec3 *muzzleTable = isAiming ? muzzleADS : muzzleHip;
        Vec3 spawnPos = v3Add(eyePos, quatRotateVec3(Rot[0], muzzleTable[Weapon[0]]));
        bulletSpawn(spawnPos, dir, bulletSpeed);

        f32 kick = (Spread[0] == 0.0f) ? firstRecoil : recoilKick;
        RecoilR[0] += kick;
        Pitch[0]   += kick;
        FirCD[0]    = cooldown;

        if (Weapon[0] == WEAPON_AK47)
        {
            Spread[0] += AK_SPREAD_RATE;
            if (Spread[0] > AK_SPREAD_MAX) Spread[0] = AK_SPREAD_MAX;
            AmmoAK[0] -= 1.0f;
            if (AmmoAK[0] <= 0.0f) ReloadCD[0] += AK_RELOAD_TIME;
        }
        else
        {
            Spread[0] = PISTOL_SPREAD_MAX;
            AmmoPistol[0] -= 1.0f;
            if (AmmoPistol[0] <= 0.0f) ReloadCD[0] += PISTOL_RELOAD_TIME;
        }
    }

    if (!fireDown && Spread[0] > 0.0f)
    {
        Spread[0] -= AK_SPREAD_DECAY * dt;
        if (Spread[0] < 0.0f) Spread[0] = 0.0f;
    }

    WasFire[0] = fireDown;
}

//=============================================================================
// Lifecycle

void playerInit(void) {}

void playerUpdate(Archetype *arch, f32 dt)
{

    void **fields = getArchetypeFields(arch, 0);
    if (!fields || arch->arena[0].count == 0) return;

    f32  *PosX        = (f32  *)fields[PF_POS_X];
    f32  *PosY        = (f32  *)fields[PF_POS_Y];
    f32  *PosZ        = (f32  *)fields[PF_POS_Z];
    Vec4 *Rot         = (Vec4 *)fields[PF_ROT];
    f32  *VelX        = (f32  *)fields[PF_VEL_X];
    f32  *VelY        = (f32  *)fields[PF_VEL_Y];
    f32  *VelZ        = (f32  *)fields[PF_VEL_Z];
    f32  *Yaw         = (f32  *)fields[PF_YAW];
    f32  *Pitch       = (f32  *)fields[PF_PITCH];
    b8   *IsGnd       = (b8   *)fields[PF_IS_GROUNDED];
    u32* Weapon      = (u32  *)fields[PF_WEAPON];
    f32  *FirCD       = (f32  *)fields[PF_FIRE_CD];
    f32  *Spread      = (f32  *)fields[PF_SPREAD];
    f32  *RecoilR     = (f32  *)fields[PF_RECOIL_R];
    b8   *WasFire     = (b8   *)fields[PF_WAS_FIRE];
    f32  *ReloadCD    = (f32  *)fields[PF_RELOAD_CD];
    f32  *AmmoPistol  = (f32  *)fields[PF_AMMO_PISTOL];
    f32  *AmmoAK      = (f32  *)fields[PF_AMMO_AK];
    b8   *HasReloaded = (b8   *)fields[PF_HAS_RELOADED];
    b8   *IsAiming    = (b8   *)fields[PF_IS_AIMING];

    // Hide player mesh in first-person
    ((u32 *)fields[PF_MODEL_ID])[0] = (u32)-1;

    if (!s_mouseCapturedOnStart)
    {
        setMouseCaptured(true);
        s_mouseCapturedOnStart = true;
        if (display && display->screenHeight > 0)
            s_cachedAspectRatio = (f32)display->screenWidth / (f32)display->screenHeight;
    }

    b8 escDown = isKeyDown(KEY_ESCAPE);
    if (escDown && !s_wasEscDown) setMouseCaptured(!isMouseCaptured());
    s_wasEscDown = escDown;

    // Weapon switch
    if ((isKeyDown(KEY_1) || isButtonDown(0, BUTTON_LEFTSHOULDER)) && Weapon[0] != WEAPON_PISTOL)
    {
        Weapon[0]      = WEAPON_PISTOL;
        AmmoPistol[0]  = PISTOL_CLIP_SIZE;
        HasReloaded[0] = true;
        ReloadCD[0]    = 0.0f;
    }
    if ((isKeyDown(KEY_2) || isButtonDown(0, BUTTON_RIGHTSHOULDER)) && Weapon[0] != WEAPON_AK47)
    {
        Weapon[0]      = WEAPON_AK47;
        AmmoAK[0]      = AK_CLIP_SIZE;
        HasReloaded[0] = true;
        ReloadCD[0]    = 0.0f;
    }
    // Y/Triangle button toggles between weapons (only on press, not hold)
    b8 yDown = isButtonDown(0, BUTTON_Y);
    if (yDown && !s_wasYDown)
    {
        if (Weapon[0] == WEAPON_PISTOL)
        {
            Weapon[0]      = WEAPON_AK47;
            AmmoAK[0]      = AK_CLIP_SIZE;
            HasReloaded[0] = true;
            ReloadCD[0]    = 0.0f;
        }
        else
        {
            Weapon[0]      = WEAPON_PISTOL;
            AmmoPistol[0]  = PISTOL_CLIP_SIZE;
            HasReloaded[0] = true;
            ReloadCD[0]    = 0.0f;
        }
    }
    s_wasYDown = yDown;
    playerLook(Yaw, Pitch, RecoilR, Rot, dt);
    playerMove(VelX, VelY, VelZ, IsGnd, Rot[0], PosX[0], PosY[0], PosZ[0]);
    // ADS: right mouse or L2
    Vec2 triggers  = getJoystickAxis(0, JOYSTICK_TRIGGER_LEFT, JOYSTICK_TRIGGER_RIGHT);
    b8 wasAiming   = IsAiming[0];
    IsAiming[0]    = isMouseDown(BTN_ADS) || (triggers.x < -0.5f);

    playerShoot(Weapon, FirCD, Spread, RecoilR, Pitch, WasFire,
                ReloadCD, AmmoPistol, AmmoAK, HasReloaded,
                Rot, PosX[0], PosY[0], PosZ[0], IsAiming[0], dt);

    // Sync camera to eye + FOV for ADS
    if (renderer)
    {
        Camera *cam = rendererGetCamera(renderer, renderer->activeCamera);
        if (cam)
        {
            cam->pos         = (Vec3){PosX[0], PosY[0] + EYE_HEIGHT, PosZ[0]};
            cam->orientation = Rot[0];

            f32 targetFov = IsAiming[0] ? FOV_ADS : FOV_NORMAL;
            s_currentFov = lerp(s_currentFov, targetFov, clamp(FOV_LERP_SPEED * dt, 0.0f, 1.0f));
            cameraSetFov(cam, s_currentFov);
        }
    }
}



void playerRender(Archetype *arch, Renderer *r) { (void)arch; (void)r; }

void playerDestroy(void)
{
    setMouseCaptured(false);
    s_mouseCapturedOnStart = false;
    s_currentFov           = FOV_NORMAL;
}

void druidGetECSSystem_Player(ECSSystemPlugin *out)
{
    out->init    = playerInit;
    out->update  = NULL;
    out->render  = playerRender;
    out->destroy = playerDestroy;
}
