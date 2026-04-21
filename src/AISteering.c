#include "AISteering.h"
#include <math.h>
#include <stdlib.h>

static Vec3 clampLen(Vec3 v, f32 maxLen)
{
    f32 m = v3Mag(v);
    if (m <= maxLen || m < 1e-5f) return v;
    return v3Scale(v, maxLen / m);
}

static Vec3 horizontal(Vec3 v) { return (Vec3){v.x, 0.0f, v.z}; }

Vec3 steerSeek(Vec3 pos, Vec3 vel, Vec3 target, f32 maxSpeed)
{
    Vec3 desired = horizontal(v3Sub(target, pos));
    f32 m = v3Mag(desired);
    if (m < 1e-4f) return v3Zero;
    desired = v3Scale(desired, maxSpeed / m);
    return v3Sub(desired, horizontal(vel));
}

Vec3 steerFlee(Vec3 pos, Vec3 vel, Vec3 threat, f32 maxSpeed)
{
    Vec3 desired = horizontal(v3Sub(pos, threat));
    f32 m = v3Mag(desired);
    if (m < 1e-4f) return v3Zero;
    desired = v3Scale(desired, maxSpeed / m);
    return v3Sub(desired, horizontal(vel));
}

Vec3 steerPursue(Vec3 pos, Vec3 vel, Vec3 tgtPos, Vec3 tgtVel, f32 maxSpeed)
{
    Vec3 toTgt = v3Sub(tgtPos, pos);
    f32 dist = v3Mag(toTgt);
    f32 tgtSpeed = v3Mag(tgtVel);
    f32 lookAhead = (tgtSpeed > 0.01f) ? (dist / (maxSpeed + tgtSpeed)) : 0.0f;
    Vec3 predicted = v3Add(tgtPos, v3Scale(tgtVel, lookAhead));
    return steerSeek(pos, vel, predicted, maxSpeed);
}

Vec3 steerEvade(Vec3 pos, Vec3 vel, Vec3 tgtPos, Vec3 tgtVel, f32 maxSpeed)
{
    Vec3 toTgt = v3Sub(tgtPos, pos);
    f32 dist = v3Mag(toTgt);
    f32 tgtSpeed = v3Mag(tgtVel);
    f32 lookAhead = (tgtSpeed > 0.01f) ? (dist / (maxSpeed + tgtSpeed)) : 0.0f;
    Vec3 predicted = v3Add(tgtPos, v3Scale(tgtVel, lookAhead));
    return steerFlee(pos, vel, predicted, maxSpeed);
}

Vec3 steerWander(Vec3 vel, f32 *wanderAngle, f32 dt, f32 maxSpeed)
{
    // Random walk on the angle, then produce a forward-facing desired velocity.
    f32 jitter = ((f32)(rand() % 2001 - 1000) / 1000.0f) * 2.0f * dt;
    *wanderAngle += jitter;

    Vec3 hvel = horizontal(vel);
    f32 hm = v3Mag(hvel);
    f32 baseYaw = (hm > 0.01f) ? atan2f(hvel.z, hvel.x) : *wanderAngle;
    f32 yaw = baseYaw + *wanderAngle * 0.3f;

    Vec3 desired = (Vec3){ cosf(yaw) * maxSpeed, 0.0f, sinf(yaw) * maxSpeed };
    return v3Sub(desired, hvel);
}

Vec3 steerArrive(Vec3 pos, Vec3 vel, Vec3 target, f32 slowRadius, f32 maxSpeed)
{
    Vec3 toTgt = horizontal(v3Sub(target, pos));
    f32 dist = v3Mag(toTgt);
    if (dist < 1e-4f) return v3Scale(horizontal(vel), -1.0f);
    f32 speed = (dist < slowRadius) ? (maxSpeed * (dist / slowRadius)) : maxSpeed;
    Vec3 desired = v3Scale(toTgt, speed / dist);
    return v3Sub(desired, horizontal(vel));
}

Vec3 steerSeparate(Vec3 pos, const Vec3 *neighbours, u32 count, f32 radius)
{
    Vec3 force = v3Zero;
    if (!neighbours || count == 0) return force;
    f32 r2 = radius * radius;
    for (u32 n = 0; n < count; n++)
    {
        Vec3 away = horizontal(v3Sub(pos, neighbours[n]));
        f32 d2 = away.x * away.x + away.z * away.z;
        if (d2 > r2 || d2 < 1e-4f) continue;
        f32 d = sqrtf(d2);
        force = v3Add(force, v3Scale(away, (radius - d) / (d * radius)));
    }
    return force;
}

Vec3 steerAvoidObstacles(Vec3 pos, Vec3 vel, f32 probeLen)
{
    if (!physicsWorld) return v3Zero;
    Vec3 hvel = horizontal(vel);
    f32 sp = v3Mag(hvel);
    if (sp < 0.05f) return v3Zero;

    Vec3 fwd = v3Scale(hvel, 1.0f / sp);
    // 30-degree whiskers
    f32 c = 0.8660254f;   // cos30
    f32 s = 0.5f;          // sin30
    Vec3 left  = (Vec3){ c * fwd.x - s * fwd.z, 0.0f, s * fwd.x + c * fwd.z };
    Vec3 right = (Vec3){ c * fwd.x + s * fwd.z, 0.0f, -s * fwd.x + c * fwd.z };

    Vec3 origin = (Vec3){ pos.x, pos.y + 1.0f, pos.z };
    Vec3 dirs[3]   = { fwd, left, right };
    f32  lens[3]   = { probeLen, probeLen * 0.7f, probeLen * 0.7f };
    Vec3 force = v3Zero;

    for (u32 i = 0; i < 3; i++)
    {
        PhysRay ray = { origin, dirs[i], lens[i], 0xFFFFFFFFu };
        PhysRayHit hit = physRaycast(physicsWorld, ray);
        if (!hit.hit) continue;
        f32 penetration = 1.0f - (hit.distance / lens[i]);
        // push away from surface normal, strength ramps as we get closer.
        force = v3Add(force, v3Scale(hit.normal, penetration * 10.0f));
    }
    force.y = 0.0f;
    return force;
}

// insertion sort by priority (stable, small n expected).
static void sortByPriority(SteerSample *s, u32 n)
{
    for (u32 i = 1; i < n; i++)
    {
        SteerSample key = s[i];
        i32 j = (i32)i - 1;
        while (j >= 0 && s[j].priority > key.priority)
        {
            s[j + 1] = s[j];
            j--;
        }
        s[j + 1] = key;
    }
}

Vec3 wtpsCombine(const SteerSample *samplesIn, u32 count, f32 maxForce)
{
    if (!samplesIn || count == 0) return v3Zero;

    SteerSample buf[16];
    if (count > 16) count = 16;
    for (u32 i = 0; i < count; i++) buf[i] = samplesIn[i];
    sortByPriority(buf, count);

    Vec3 total = v3Zero;
    f32 maxSq = maxForce * maxForce;

    u32 i = 0;
    while (i < count)
    {
        u8 tier = buf[i].priority;
        Vec3 tierForce = v3Zero;
        while (i < count && buf[i].priority == tier)
        {
            tierForce = v3Add(tierForce, v3Scale(buf[i].force, buf[i].weight));
            i++;
        }
        total = v3Add(total, tierForce);

        f32 mSq = total.x * total.x + total.y * total.y + total.z * total.z;
        if (mSq >= maxSq)
        {
            f32 m = sqrtf(mSq);
            total = v3Scale(total, maxForce / m);
            break;
        }
    }
    return total;
}
