#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scan the currently loaded scene for SceneEntity rows with tag == matchTag
// and spawn an enemy at each one's position.  Returns spawn count.
DSAPI u32 aiSpawnFromScene(const c8 *matchTag);

// Read up to maxCount world positions from scene entities tagged matchTag into
// out_positions.  Does NOT spawn enemies.  Returns count found.
DSAPI u32 aiReadSpawnPoints(const c8 *matchTag, Vec3 *out_positions, u32 maxCount);

#ifdef __cplusplus
}
#endif
