#include "AIBrain.h"
#include "AISteering.h"
#include "Enemy.h"
#include "Player.h"
#include "game.h"
#include <math.h>

// --- tunables --------------------------------------------------------------
#define MAX_FORCE            400.0f
#define MAX_SPEED_IDLE         1.5f
#define MAX_SPEED_AGITATED     3.0f
#define MAX_SPEED_CHASE        5.0f
#define MAX_SPEED_SCARED       6.5f
#define CHASE_RANGE_MAX       20.0f
#define SCARED_HP_FRAC         0.25f
#define OBSTACLE_PROBE         2.5f
#define SEPARATION_RADIUS      3.0f  // radius for zombie-zombie separation
#define SEP_WEIGHT_IDLE        1.2f  // separation strength per state
#define SEP_WEIGHT_AGITATED    1.8f
#define SEP_WEIGHT_CHASE       4.0f  // chase speed is 5 m/s so needs strong push to diverge
#define SEP_WEIGHT_SCARED      0.6f
#define HYSTERESIS             0.10f
#define GROUND_PROBE           0.9f + 0.15f
#define WANDER_MIN_DIST        5.0f  // min distance for new wander target
#define WANDER_MAX_DIST       14.0f  // max distance for new wander target
#define WANDER_ARRIVE_R        1.5f  // close-enough radius to count as arrived
#define WANDER_DWELL_TIME      4.0f  // seconds to stand at target before re-sampling
#define WANDER_TRAVEL_BUDGET   9.0f  // fallback: re-sample if still not arrived
#define STEER_RESPONSE         8.0f

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

    // Collect all alive positions up-front for flocking separation.
    // Stack-allocated; fine for typical enemy counts.
    Vec3  allPos[512];
    u32   allCount = 0;
    for (u32 c2 = 0; c2 < arch->activeChunkCount && allCount < 512; c2++)
    {
        void **f2 = getArchetypeFields(arch, c2);
        if (!f2) continue;
        b8  *a2 = (b8  *)f2[EF_ALIVE];
        f32 *x2 = (f32 *)f2[EF_POS_X];
        f32 *y2 = (f32 *)f2[EF_POS_Y];
        f32 *z2 = (f32 *)f2[EF_POS_Z];
        u32 n2  = arch->arena[c2].count;
        for (u32 j = 0; j < n2 && allCount < 512; j++)
            if (a2[j]) allPos[allCount++] = (Vec3){ x2[j], y2[j], z2[j] };
    }

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
        u32  *state    = (u32 *)fields[EF_STATE];
        u32  *prev     = (u32 *)fields[EF_PREV_STATE];
        f32  *sTimer   = (f32 *)fields[EF_STATE_TIMER];
        f32  *health   = (f32 *)fields[EF_HEALTH];
        f32  *healthMx = (f32 *)fields[EF_HEALTH_MAX];
        f32  *visR     = (f32 *)fields[EF_VISION_RANGE];
        f32  *visCos   = (f32 *)fields[EF_VISION_COS];
        f32  *hearR    = (f32 *)fields[EF_HEARING];
        f32  *lsX      = (f32 *)fields[EF_LAST_SEEN_X];
        f32  *lsY      = (f32 *)fields[EF_LAST_SEEN_Y];
        f32  *lsZ      = (f32 *)fields[EF_LAST_SEEN_Z];
        f32  *lsAge    = (f32 *)fields[EF_LAST_SEEN_AGE];
        f32  *wX       = (f32 *)fields[EF_WANDER_X];
        f32  *wZ       = (f32 *)fields[EF_WANDER_Z];
        f32  *wTimer   = (f32 *)fields[EF_WANDER_TIMER];
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

            // Use gameplay yaw directly — rot[i] has a model-offset baked in
            // (ZOMBIE_YAW_OFFSET) that would flip the vision cone 180°.
            Vec3 fwd = quatRotateVec3(quatFromAxisAngle(v3Up, yawF[i]), v3Forward);
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
            // Agitated: lost sight of player recently, or can hear them but can't see
            f32 uAgit   = sees ? 0.0f : (0.6f * sawRecently + (hears ? 0.4f : 0.0f));
            // Chase: player is visible and within chase range
            f32 uChase  = (sees && dist < CHASE_RANGE_MAX) ? (0.4f + 0.6f * smoothstep01(CHASE_RANGE_MAX, 0.0f, dist)) : 0.0f;
            f32 hurtFac = (1.0f - hpFrac); hurtFac *= hurtFac;
            f32 uScared = hurtFac + ((sees && hpFrac < SCARED_HP_FRAC) ? 0.3f : 0.0f);

            uIdle   = clamp01(uIdle);
            uAgit   = clamp01(uAgit);
            uChase  = clamp01(uChase);
            uScared = clamp01(uScared);

            u32 current = state[i];
            f32 uCur = (current == AI_STATE_IDLE)     ? uIdle  :
                       (current == AI_STATE_AGITATED) ? uAgit  :
                       (current == AI_STATE_CHASE)    ? uChase : uScared;

            f32 bestU = uCur; u32 best = current;
            if (uIdle   > bestU + HYSTERESIS) { bestU = uIdle;   best = AI_STATE_IDLE; }
            if (uAgit   > bestU + HYSTERESIS) { bestU = uAgit;   best = AI_STATE_AGITATED; }
            if (uChase  > bestU + HYSTERESIS) { bestU = uChase;  best = AI_STATE_CHASE; }
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

            // --- find nearby zombies for flocking (exclude self by distance > 0.05)
            Vec3 neighbours[16];
            u32  nNeighbours = 0;
            f32  sepR2 = SEPARATION_RADIUS * SEPARATION_RADIUS;
            for (u32 n = 0; n < allCount && nNeighbours < 16; n++)
            {
                f32 dx = allPos[n].x - ePos.x, dz = allPos[n].z - ePos.z;
                f32 d2 = dx*dx + dz*dz;
                if (d2 > 0.01f && d2 < sepR2)   // 0.01 = ~0.1 m, excludes exact self
                    neighbours[nNeighbours++] = allPos[n];
            }

            // --- steering stack per state
            SteerSample samples[8];
            u32 nSamples = 0;
            f32 maxSpeed = MAX_SPEED_IDLE;

            // Priority 0: obstacle avoidance
            samples[nSamples++] = (SteerSample){ steerAvoidObstacles(ePos, eVel, OBSTACLE_PROBE), 1.0f, 0 };

            // Priority 0: zombie-zombie separation (weight varies by state)
            if (nNeighbours > 0)
            {
                f32 sepW = (state[i] == AI_STATE_CHASE)    ? SEP_WEIGHT_CHASE    :
                           (state[i] == AI_STATE_AGITATED) ? SEP_WEIGHT_AGITATED :
                           (state[i] == AI_STATE_SCARED)   ? SEP_WEIGHT_SCARED   :
                                                             SEP_WEIGHT_IDLE;
                samples[nSamples++] = (SteerSample){
                    steerSeparate(ePos, neighbours, nNeighbours, SEPARATION_RADIUS), sepW, 0 };
            }

            switch (state[i])
            {
                case AI_STATE_IDLE:
                {
                    maxSpeed = MAX_SPEED_IDLE;

                    // Tick wander timer; re-sample target when it expires
                    wTimer[i] -= dt;
                    if (wTimer[i] <= 0.0f)
                    {
                        f32 angle = ((f32)(rand() % 3600) / 3600.0f) * 6.28318530f;
                        f32 r = WANDER_MIN_DIST +
                                ((f32)(rand() % 1000) / 1000.0f) * (WANDER_MAX_DIST - WANDER_MIN_DIST);
                        wX[i] = posX[i] + cosf(angle) * r;
                        wZ[i] = posZ[i] + sinf(angle) * r;
                        wTimer[i] = WANDER_TRAVEL_BUDGET;
                    }

                    // Steer toward wander target; steerArrive naturally decelerates
                    // into a dwell when close, then the timer forces a new sample.
                    f32 tdx = wX[i] - posX[i], tdz = wZ[i] - posZ[i];
                    b8 arrived = (tdx*tdx + tdz*tdz) < (WANDER_ARRIVE_R * WANDER_ARRIVE_R);
                    if (arrived && wTimer[i] > WANDER_DWELL_TIME)
                        wTimer[i] = WANDER_DWELL_TIME; // clamp down to dwell budget on arrival

                    if (!arrived)
                    {
                        Vec3 wTarget = { wX[i], ePos.y, wZ[i] };
                        samples[nSamples++] = (SteerSample){
                            steerArrive(ePos, eVel, wTarget, WANDER_ARRIVE_R * 2.0f, maxSpeed),
                            1.0f, 2 };
                    }
                    break;
                }
                case AI_STATE_AGITATED:
                {
                    maxSpeed = MAX_SPEED_AGITATED;
                    Vec3 lastSeen = { lsX[i], lsY[i], lsZ[i] };
                    samples[nSamples++] = (SteerSample){
                        steerArrive(ePos, eVel, lastSeen, 2.0f, maxSpeed), 1.0f, 1 };
                    break;
                }
                case AI_STATE_CHASE:
                {
                    maxSpeed = MAX_SPEED_CHASE;
                    samples[nSamples++] = (SteerSample){
                        steerPursue(ePos, eVel, pPos, pVel, maxSpeed), 1.0f, 1 };
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

            // Steering returns (desired_velocity - current_velocity). Apply it
            // as a velocity delta with a kinematic gain so the zombie accelerates
            // smoothly without the near-zero result of mass integration.
            Vec3 hv = { velX[i] + force.x * dt * STEER_RESPONSE,
                        0.0f,
                        velZ[i] + force.z * dt * STEER_RESPONSE };
            f32 hm = v3Mag(hv);
            if (hm > maxSpeed) hv = v3Scale(hv, maxSpeed / hm);

            velX[i] = hv.x;
            velZ[i] = hv.z;  // velY left alone so gravity applies

            // --- yaw to face movement direction (chase: always face the player)
            Vec3 faceDir = (state[i] == AI_STATE_CHASE && dist > 0.1f) ? dirToP : hv;
            f32 fd = faceDir.x * faceDir.x + faceDir.z * faceDir.z;
            if (fd > 1e-4f)
            {
                yawF[i] = atan2f(-faceDir.x, -faceDir.z);
                rot[i] = quatFromAxisAngle(v3Up, yawF[i] + ZOMBIE_YAW_OFFSET);
            }
        }
    }
}
