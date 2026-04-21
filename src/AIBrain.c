#include "AIBrain.h"
#include "AISteering.h"
#include "Enemy.h"
#include "Player.h"
#include "Bullet.h"
#include "game.h"
#include <math.h>

// --- tunables --------------------------------------------------------------
#define MAX_FORCE          400.0f
#define MAX_SPEED_IDLE       1.5f
#define MAX_SPEED_AGITATED   3.5f
#define MAX_SPEED_CHASE      5.5f
#define MAX_SPEED_SCARED     6.5f
#define ATTACK_RANGE_MAX    18.0f
#define ATTACK_RANGE_MIN     2.0f
#define ATTACK_FIRE_CD       0.5f
#define BULLET_SPEED_ENEMY 80.0f
#define SCARED_HP_FRAC       0.25f
#define OBSTACLE_PROBE       2.5f
#define SEPARATION_RADIUS    2.0f
#define HYSTERESIS           0.10f
#define GROUND_PROBE         0.9f + 0.15f

// --- helpers ---------------------------------------------------------------

static Vec3 playerPos(void)
{
    if (!g_playerArch.arena) return v3Zero;
    void **f = getArchetypeFields(&g_playerArch, 0);
    if (!f) return v3Zero;
    return (Vec3){
        ((f32 *)f[PF_POS_X])[0],
        ((f32 *)f[PF_POS_Y])[0],
        ((f32 *)f[PF_POS_Z])[0],
    };
}

static Vec3 playerVel(void)
{
    if (!g_playerArch.arena) return v3Zero;
    void **f = getArchetypeFields(&g_playerArch, 0);
    if (!f) return v3Zero;
    return (Vec3){
        ((f32 *)f[PF_VEL_X])[0],
        ((f32 *)f[PF_VEL_Y])[0],
        ((f32 *)f[PF_VEL_Z])[0],
    };
}

static b8 rayHitsPlayer(Vec3 eye, Vec3 dir, f32 maxDist)
{
    if (!physicsWorld) return true; // fail-open: assume visible if no physics yet.
    PhysRay ray = { eye, dir, maxDist, 0xFFFFFFFFu };
    PhysRayHit hit = physRaycast(physicsWorld, ray);
    if (!hit.hit) return true;
    // If the raycast hit something significantly before the target, LOS is blocked.
    return hit.distance >= (maxDist - 0.5f);
}

static f32 clamp01(f32 v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

static f32 smoothstep01(f32 edge0, f32 edge1, f32 x)
{
    f32 t = clamp01((x - edge0) / (edge1 - edge0 + 1e-6f));
    return t * t * (3.0f - 2.0f * t);
}

// --- main tick -------------------------------------------------------------

void aiBrainTick(Archetype *arch, f32 dt)
{
    if (!arch || !arch->arena) return;

    Vec3 pPos = playerPos();
    Vec3 pVel = playerVel();
    Vec3 eyeTarget = (Vec3){ pPos.x, pPos.y + 1.2f, pPos.z };

    for (u32 ch = 0; ch < arch->activeChunkCount; ch++)
    {
        void **fields = getArchetypeFields(arch, ch);
        if (!fields) continue;
        u32 count = arch->arena[ch].count;

        b8   *alive    = (b8  *)fields[EF_ALIVE];
        f32  *posX     = (f32 *)fields[EF_POS_X];
        f32  *posY     = (f32 *)fields[EF_POS_Y];
        f32  *posZ     = (f32 *)fields[EF_POS_Z];
        Vec4 *rot      = (Vec4 *)fields[EF_ROT];
        f32  *velX     = (f32 *)fields[EF_VEL_X];
        f32  *velY     = (f32 *)fields[EF_VEL_Y];
        f32  *velZ     = (f32 *)fields[EF_VEL_Z];
        f32  *mass     = (f32 *)fields[EF_MASS];
        u32  *state    = (u32 *)fields[EF_STATE];
        u32  *prev     = (u32 *)fields[EF_PREV_STATE];
        f32  *sTimer   = (f32 *)fields[EF_STATE_TIMER];
        f32  *health   = (f32 *)fields[EF_HEALTH];
        f32  *healthMx = (f32 *)fields[EF_HEALTH_MAX];
        f32  *visR     = (f32 *)fields[EF_VISION_RANGE];
        f32  *visCos   = (f32 *)fields[EF_VISION_COS];
        f32  *hearR    = (f32 *)fields[EF_HEARING];
        f32  *fireCD   = (f32 *)fields[EF_FIRE_CD];
        f32  *lsX      = (f32 *)fields[EF_LAST_SEEN_X];
        f32  *lsY      = (f32 *)fields[EF_LAST_SEEN_Y];
        f32  *lsZ      = (f32 *)fields[EF_LAST_SEEN_Z];
        f32  *lsAge    = (f32 *)fields[EF_LAST_SEEN_AGE];
        f32  *wAng     = (f32 *)fields[EF_WANDER_ANGLE];
        f32  *yawF     = (f32 *)fields[EF_YAW];
        b8   *grounded = (b8  *)fields[EF_IS_GROUNDED];

        for (u32 i = 0; i < count; i++)
        {
            if (!alive[i]) continue;

            Vec3 ePos = { posX[i], posY[i], posZ[i] };
            Vec3 eVel = { velX[i], velY[i], velZ[i] };
            Vec3 eye  = { ePos.x, ePos.y + 1.2f, ePos.z };

            // --- ground check
            grounded[i] = false;
            if (physicsWorld)
            {
                PhysRay gr = { ePos, v3Down, GROUND_PROBE, 0xFFFFFFFFu };
                if (physRaycast(physicsWorld, gr).hit) grounded[i] = true;
            }

            // --- perception
            Vec3 toPlayer = v3Sub(pPos, ePos);
            f32 dist = v3Mag(toPlayer);
            Vec3 dirToP = (dist > 1e-4f) ? v3Scale(toPlayer, 1.0f / dist) : v3Zero;

            Vec3 fwd = quatRotateVec3(rot[i], v3Forward);
            fwd.y = 0.0f; f32 fm = v3Mag(fwd);
            if (fm > 1e-4f) fwd = v3Scale(fwd, 1.0f / fm);

            f32 facing = v3Dot(fwd, dirToP);
            b8 withinVisionCone = (dist < visR[i]) && (facing > visCos[i]);
            b8 losClear         = false;
            if (withinVisionCone)
                losClear = rayHitsPlayer(eye, v3Norm(v3Sub(eyeTarget, eye)), dist);
            b8 sees = withinVisionCone && losClear;
            b8 hears = (dist < hearR[i]);

            if (sees)
            {
                lsX[i] = pPos.x; lsY[i] = pPos.y; lsZ[i] = pPos.z;
                lsAge[i] = 0.0f;
            }
            else
            {
                lsAge[i] += dt;
            }

            // --- utility scoring
            f32 hpFrac = (healthMx[i] > 0.01f) ? (health[i] / healthMx[i]) : 1.0f;
            f32 sawRecently = 1.0f - smoothstep01(0.0f, 8.0f, lsAge[i]);

            f32 uIdle   = 1.0f - (sees ? 1.0f : (hears ? 0.5f : sawRecently));
            f32 uAgit   = sees ? 0.0f : (0.6f * sawRecently + (hears ? 0.4f : 0.0f));
            f32 uAttack = sees ? (0.3f + 0.7f * smoothstep01(ATTACK_RANGE_MAX, ATTACK_RANGE_MIN, dist)) : 0.0f;
            f32 hurtFac = (1.0f - hpFrac); hurtFac *= hurtFac;
            f32 uScared = hurtFac + ((sees && hpFrac < SCARED_HP_FRAC) ? 0.3f : 0.0f);

            uIdle   = clamp01(uIdle);
            uAgit   = clamp01(uAgit);
            uAttack = clamp01(uAttack);
            uScared = clamp01(uScared);

            u32 current = state[i];
            f32 uCur = (current == AI_STATE_IDLE) ? uIdle :
                       (current == AI_STATE_AGITATED) ? uAgit :
                       (current == AI_STATE_ATTACKING) ? uAttack : uScared;

            f32 bestU = uCur; u32 best = current;
            if (uIdle   > bestU + HYSTERESIS) { bestU = uIdle;   best = AI_STATE_IDLE; }
            if (uAgit   > bestU + HYSTERESIS) { bestU = uAgit;   best = AI_STATE_AGITATED; }
            if (uAttack > bestU + HYSTERESIS) { bestU = uAttack; best = AI_STATE_ATTACKING; }
            if (uScared > bestU + HYSTERESIS) { bestU = uScared; best = AI_STATE_SCARED; }

            if (best != current)
            {
                prev[i] = current;
                state[i] = best;
                sTimer[i] = 0.0f;
            }
            else
            {
                sTimer[i] += dt;
            }

            // --- steering stack per state
            SteerSample samples[8];
            u32 nSamples = 0;
            f32 maxSpeed = MAX_SPEED_IDLE;

            samples[nSamples++] = (SteerSample){ steerAvoidObstacles(ePos, eVel, OBSTACLE_PROBE), 1.0f, 0 };

            switch (state[i])
            {
                case AI_STATE_IDLE:
                {
                    maxSpeed = MAX_SPEED_IDLE;
                    samples[nSamples++] = (SteerSample){
                        steerWander(eVel, &wAng[i], dt, maxSpeed), 1.0f, 2 };
                    break;
                }
                case AI_STATE_AGITATED:
                {
                    maxSpeed = MAX_SPEED_AGITATED;
                    Vec3 lastSeen = { lsX[i], lsY[i], lsZ[i] };
                    samples[nSamples++] = (SteerSample){
                        steerArrive(ePos, eVel, lastSeen, 2.0f, maxSpeed), 1.0f, 1 };
                    samples[nSamples++] = (SteerSample){
                        steerWander(eVel, &wAng[i], dt, maxSpeed), 0.3f, 2 };
                    break;
                }
                case AI_STATE_ATTACKING:
                {
                    maxSpeed = MAX_SPEED_CHASE;
                    samples[nSamples++] = (SteerSample){
                        steerPursue(ePos, eVel, pPos, pVel, maxSpeed), 1.0f, 1 };
                    // strafe: tangential to dirToP
                    Vec3 tang = { -dirToP.z, 0.0f, dirToP.x };
                    f32 strafeSign = (sinf(sTimer[i] * 1.3f) > 0.0f) ? 1.0f : -1.0f;
                    samples[nSamples++] = (SteerSample){
                        v3Scale(tang, maxSpeed * 0.4f * strafeSign), 0.5f, 2 };
                    break;
                }
                case AI_STATE_SCARED:
                {
                    maxSpeed = MAX_SPEED_SCARED;
                    samples[nSamples++] = (SteerSample){
                        steerFlee(ePos, eVel, pPos, maxSpeed), 1.2f, 1 };
                    break;
                }
            }

            Vec3 force = wtpsCombine(samples, nSamples, MAX_FORCE);

            // --- apply: interpret force as horizontal velocity target
            f32 invMass = (mass[i] > 0.01f) ? (1.0f / mass[i]) : 0.0f;
            Vec3 dv = v3Scale(force, invMass * dt);
            Vec3 hv = { velX[i] + dv.x, 0.0f, velZ[i] + dv.z };
            f32 hm = v3Mag(hv);
            if (hm > maxSpeed) hv = v3Scale(hv, maxSpeed / hm);

            velX[i] = hv.x;
            velZ[i] = hv.z;  // velY left alone so gravity applies

            // --- yaw to face movement direction (or player when attacking)
            Vec3 faceDir = (state[i] == AI_STATE_ATTACKING && dist > 0.1f) ? dirToP : hv;
            f32 fd = faceDir.x * faceDir.x + faceDir.z * faceDir.z;
            if (fd > 1e-4f)
            {
                yawF[i] = atan2f(-faceDir.x, -faceDir.z);
                rot[i] = quatFromAxisAngle(v3Up, yawF[i]);
            }

            // --- fire logic
            if (fireCD[i] > 0.0f) fireCD[i] -= dt;
            if (state[i] == AI_STATE_ATTACKING && sees &&
                dist < ATTACK_RANGE_MAX && fireCD[i] <= 0.0f)
            {
                Vec3 aimDir = v3Norm(v3Sub(eyeTarget, eye));
                bulletSpawn(eye, aimDir, BULLET_SPEED_ENEMY);
                fireCD[i] = ATTACK_FIRE_CD;
            }
        }
    }
}
