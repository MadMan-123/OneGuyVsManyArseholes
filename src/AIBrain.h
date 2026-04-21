#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// Per-frame brain tick. Runs perception -> utility scoring ->
// FSM transition -> WTPS locomotion -> fire logic for every alive enemy.
DSAPI void aiBrainTick(Archetype *arch, f32 dt);

#ifdef __cplusplus
}
#endif
