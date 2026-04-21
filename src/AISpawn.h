#pragma once
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scan the currently loaded scene for SceneEntity rows with tag == matchTag
// and spawn an enemy at each one's position.  Returns spawn count.
DSAPI u32 aiSpawnFromScene(const c8 *matchTag);

#ifdef __cplusplus
}
#endif
