#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    Vec3 force;     // behaviour output (force or desired-velocity delta)
    f32  weight;    // 0..1 blend within a priority tier
    u8   priority;  // lower = higher priority; tiers processed in order
} SteerSample;

DSAPI Vec3 steerSeek    (Vec3 pos, Vec3 vel, Vec3 target, f32 maxSpeed);
DSAPI Vec3 steerFlee    (Vec3 pos, Vec3 vel, Vec3 threat, f32 maxSpeed);
DSAPI Vec3 steerPursue  (Vec3 pos, Vec3 vel, Vec3 tgtPos, Vec3 tgtVel, f32 maxSpeed);
DSAPI Vec3 steerEvade   (Vec3 pos, Vec3 vel, Vec3 tgtPos, Vec3 tgtVel, f32 maxSpeed);
DSAPI Vec3 steerWander  (Vec3 vel, f32 *wanderAngle, f32 dt, f32 maxSpeed);
DSAPI Vec3 steerArrive  (Vec3 pos, Vec3 vel, Vec3 target, f32 slowRadius, f32 maxSpeed);
DSAPI Vec3 steerSeparate(Vec3 pos, const Vec3 *neighbours, u32 count, f32 radius);
DSAPI Vec3 steerAvoidObstacles(Vec3 pos, Vec3 vel, f32 probeLen);

// Weighted Truncated Priority Steering combiner.
//   - Groups samples by priority (ascending).
//   - Within a tier: force += weight * sample.force.
//   - If accumulated |force| >= maxForce after a tier, truncate and stop.
DSAPI Vec3 wtpsCombine(const SteerSample *samples, u32 count, f32 maxForce);

#ifdef __cplusplus
}
#endif
