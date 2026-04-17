#include "Player.h"
#include "Bullet.h"
#include <stdlib.h>

#define CAM_ROTATE_SPEED   1.0f
#define MOVE_SPEED         8.0f
#define SPRINT_MULTIPLIER  2.0f
#define JUMP_FORCE         6.0f
#define EYE_HEIGHT         1.6f

#define WEAPON_PISTOL  0
#define WEAPON_AK47    1

#define PISTOL_COOLDOWN    0.15f
#define PISTOL_SPEED       120.0f
#define PISTOL_SPREAD_MAX  0.01f
#define PISTOL_RECOIL      0.03f

#define AK_COOLDOWN        0.1f
#define AK_SPEED           180.0f
#define AK_SPREAD_MAX      0.06f
#define AK_SPREAD_RATE     0.012f
#define AK_SPREAD_DECAY    0.08f
#define AK_RECOIL          0.05f

#define RECOIL_RECOVERY_SPEED 2.0f

// Temporary debug: draw movement basis axes at the player each frame.
#define DEBUG_DRAW_MOVE_AXES 1

static b8 s_wasEscDown = false;
static b8 s_physicsInitialized = false;
static b8 s_mouseCapturedOnStart = false;

DEFINE_ARCHETYPE(Player, PLAYER_FIELDS)

// DRUID_FLAGS 0x05
// isSingle
// isPhysicsBody

static u32 s_ibSlot = (u32)-1;

//=============================================================================
// Helpers

static void playerLook(f32 *Yaw, f32 *Pitch, f32 *RecoilR, Vec4 *Rot, f32 dt)
{
    Yaw[0]   += xLookAxis * CAM_ROTATE_SPEED * dt;
    Pitch[0] += yLookAxis * CAM_ROTATE_SPEED * dt;
    Pitch[0]  = clamp(Pitch[0], -radians(89.0f), radians(89.0f));

    // bleed off accumulated recoil
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

    // Keep movement on the ground plane even if the view is pitched up/down.
    forward.y = 0.0f; forward = v3Norm(forward);
    right.y   = 0.0f; right   = v3Norm(right);

    f32 speed = MOVE_SPEED;
    if (isKeyDown(KEY_LSHIFT) || isButtonDown(0, BUTTON_LEFTSTICK)) speed *= SPRINT_MULTIPLIER;

    Vec3 move = v3Add(v3Scale(forward, yInputAxis), v3Scale(right, -xInputAxis));
    f32 len = v3Mag(move);
    if (len > 0.001f)
        move = v3Scale(move, speed / len);
    else
        move = (Vec3){0, 0, 0};

    // leave vertical velocity to physics
    VelX[0] = move.x;
    VelZ[0] = move.z;

    // ground check
    IsGnd[0] = false;
    if (physicsWorld)
    {
        PhysRay ray;
        ray.origin      = (Vec3){posX, posY, posZ};
        ray.direction   = (Vec3){0.0f, -1.0f, 0.0f};
        ray.maxDistance = 0.9f + 0.15f;
        ray.layerMask   = 0xFFFFFFFF;
        PhysRayHit hit  = physRaycast(physicsWorld, ray);
        if (hit.hit) IsGnd[0] = true;
    }

    if (IsGnd[0] && (isKeyDown(KEY_SPACE) || isButtonDown(0, BUTTON_CROSS)))
        VelY[0] = JUMP_FORCE;
}

static void drawMovementAxesDebug(Vec3 origin, Vec4 rot)
{
#if DEBUG_DRAW_MOVE_AXES
    const f32 len = 1.5f;
    const f32 head = 0.12f;

    Vec3 forward = quatRotateVec3(rot, v3Forward);
    Vec3 right   = quatRotateVec3(rot, v3Right);
    forward.y = 0.0f;
    right.y   = 0.0f;
    forward = v3Norm(forward);
    right   = v3Norm(right);

    // Right (red), Up (green), Forward (blue)
    gizmoDrawArrow(origin, v3Add(origin, v3Scale(right, len)), head, GIZMO_RED);
    gizmoDrawArrow(origin, v3Add(origin, v3Scale(v3Up, len * 0.75f)), head, GIZMO_GREEN);
    gizmoDrawArrow(origin, v3Add(origin, v3Scale(forward, len)), head, GIZMO_BLUE);
#else
    (void)origin;
    (void)rot;
#endif
}

static void playerShoot(u32 *Weapon, f32 *FirCD, f32 *Spread, f32 *RecoilR,
                        f32 *Pitch, b8 *WasFire, Vec4 *Rot,
                        f32 posX, f32 posY, f32 posZ, f32 dt)
{
    if (FirCD[0] > 0.0f) FirCD[0] -= dt;

    Vec2 triggers = getJoystickAxis(0, JOYSTICK_TRIGGER_LEFT, JOYSTICK_TRIGGER_RIGHT);
    b8 fireDown = isMouseDown(MOUSE_LEFT) || isMouseDown(MOUSE_LEFT + 1) ||
                  isKeyDown(KEY_LCTRL)    || isKeyDown(KEY_RCTRL) ||
                  (triggers.y < -0.5f);

    b8 canFire = (Weapon[0] == WEAPON_PISTOL) ? (fireDown && !WasFire[0] && FirCD[0] <= 0.0f)
                                              : (fireDown && FirCD[0] <= 0.0f);

    if (canFire)
    {
        f32 bulletSpeed = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_SPEED    : AK_SPEED;
        f32 cooldown    = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_COOLDOWN : AK_COOLDOWN;
        f32 recoilKick  = (Weapon[0] == WEAPON_PISTOL) ? PISTOL_RECOIL   : AK_RECOIL;

        f32 spreadX = Spread[0] * ((f32)(rand() % 2001 - 1000) / 1000.0f);
        f32 spreadY = Spread[0] * ((f32)(rand() % 2001 - 1000) / 1000.0f);

        Vec3 dir = quatRotateVec3(Rot[0], v3Forward);
        dir.x += spreadX;
        dir.y += spreadY;
        dir = v3Norm(dir);

        Vec3 eyePos   = {posX, posY + EYE_HEIGHT, posZ};
        Vec3 spawnPos = v3Add(eyePos, v3Scale(dir, 0.5f));
        bulletSpawn(spawnPos, dir, bulletSpeed);

        FirCD[0]    = cooldown;
        RecoilR[0] += recoilKick;
        Pitch[0]   += recoilKick;

        if (Weapon[0] == WEAPON_AK47)
        {
            Spread[0] += AK_SPREAD_RATE;
            if (Spread[0] > AK_SPREAD_MAX) Spread[0] = AK_SPREAD_MAX;
        }
        else
        {
            Spread[0] = PISTOL_SPREAD_MAX;
        }
    }

    // spread decays while trigger is up
    if (!fireDown && Spread[0] > 0.0f)
    {
        Spread[0] -= AK_SPREAD_DECAY * dt;
        if (Spread[0] < 0.0f) Spread[0] = 0.0f;
    }

    WasFire[0] = fireDown;
}

//=============================================================================
// Lifecycle

void playerInit(void)
{
    s_ibSlot = rendererAcquireInstanceBuffer(renderer, 1024);
}

void playerUpdate(Archetype *arch, f32 dt)
{
    void **fields = getArchetypeFields(arch, 0);
    if (!fields || arch->arena[0].count == 0) return;

    f32  *PosX    = (f32 *)fields[PF_POS_X];
    f32  *PosY    = (f32 *)fields[PF_POS_Y];
    f32  *PosZ    = (f32 *)fields[PF_POS_Z];
    Vec4 *Rot     = (Vec4 *)fields[PF_ROT];
    f32  *VelX    = (f32 *)fields[PF_VEL_X];
    f32  *VelY    = (f32 *)fields[PF_VEL_Y];
    f32  *VelZ    = (f32 *)fields[PF_VEL_Z];
    f32  *Yaw     = (f32 *)fields[PF_YAW];
    f32  *Pitch   = (f32 *)fields[PF_PITCH];
    b8   *IsGnd   = (b8  *)fields[PF_IS_GROUNDED];
    u32  *Weapon  = (u32 *)fields[PF_WEAPON];
    f32  *FirCD   = (f32 *)fields[PF_FIRE_CD];
    f32  *Spread  = (f32 *)fields[PF_SPREAD];
    f32  *RecoilR = (f32 *)fields[PF_RECOIL_R];
    b8   *WasFire = (b8  *)fields[PF_WAS_FIRE];

    // First-person view: hide runtime self mesh to avoid seeing clipped body
    // parts while still allowing a separate placed scene model as a guide.
    u32 *ModelID = (u32 *)fields[PF_MODEL_ID];
    ModelID[0] = (u32)-1;

    // one-time physics defaults
    if (!s_physicsInitialized)
    {
        u32 *BodyType = (u32 *)fields[PF_BODY_TYPE];
        f32 *Mass     = (f32 *)fields[PF_MASS];
        f32 *Restit   = (f32 *)fields[PF_RESTITUTION];
        f32 *Damp     = (f32 *)fields[PF_DAMPING];
        f32 *ColHalfX = (f32 *)fields[PF_HALF_X];
        f32 *ColHalfY = (f32 *)fields[PF_HALF_Y];
        f32 *ColHalfZ = (f32 *)fields[PF_HALF_Z];

        BodyType[0] = PHYS_BODY_DYNAMIC;
        Mass[0]     = 80.0f;
        Restit[0]   = 0.0f;
        Damp[0]     = 0.1f;
        ColHalfX[0] = 0.4f;
        ColHalfY[0] = 0.9f;
        ColHalfZ[0] = 0.4f;

        s_physicsInitialized = true;
        INFO("Player physics init: bt=%u mass=%.1f damp=%.2f half=(%.2f,%.2f,%.2f)",
             BodyType[0], Mass[0], Damp[0], ColHalfX[0], ColHalfY[0], ColHalfZ[0]);
    }

    if (!s_mouseCapturedOnStart)
    {
        setMouseCaptured(true);
        s_mouseCapturedOnStart = true;
    }

    // escape toggles mouse capture
    b8 escDown = isKeyDown(KEY_ESCAPE);
    if (escDown && !s_wasEscDown)
        setMouseCaptured(!isMouseCaptured());
    s_wasEscDown = escDown;

    playerLook(Yaw, Pitch, RecoilR, Rot, dt);
    playerMove(VelX, VelY, VelZ, IsGnd, Rot[0], PosX[0], PosY[0], PosZ[0]);

    // weapon select
    if (isKeyDown(KEY_1) || isButtonDown(0, BUTTON_LEFTSHOULDER))  Weapon[0] = WEAPON_PISTOL;
    if (isKeyDown(KEY_2) || isButtonDown(0, BUTTON_RIGHTSHOULDER)) Weapon[0] = WEAPON_AK47;

    playerShoot(Weapon, FirCD, Spread, RecoilR, Pitch, WasFire, Rot,
                PosX[0], PosY[0], PosZ[0], dt);

    drawMovementAxesDebug((Vec3){PosX[0], PosY[0] + 0.05f, PosZ[0]}, Rot[0]);

    // sync camera to eye
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

void playerRender(Archetype *arch, Renderer *r)
{
    // Player is rendered via the game plugin's own archetype.
    // This no-op prevents the editor falling back to rendererDefaultArchetypeRender
    // on the scene-placed entity, which would show a duplicate without the game texture.
    (void)arch; (void)r;
}

void playerDestroy(void)
{
    if (s_ibSlot != (u32)-1) { rendererReleaseInstanceBuffer(renderer, s_ibSlot); s_ibSlot = (u32)-1; }
    setMouseCaptured(false);
    s_physicsInitialized = false;
    s_mouseCapturedOnStart = false;
}

void druidGetECSSystem_Player(ECSSystemPlugin *out)
{
    // game.cpp owns this archetype and calls playerUpdate explicitly.
    // update=NULL prevents the editor's ECS loop from double-invoking it (double bullets).
    // render=playerRender (no-op) prevents the editor from falling back to
    // rendererDefaultArchetypeRender on the scene entity (double player render).
    out->init    = playerInit;
    out->update  = NULL;
    out->render  = playerRender;
    out->destroy = playerDestroy;
}
