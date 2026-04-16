#pragma once
// Generated scaffold; gameplay edits are allowed.
#define DRUID_SYSTEM_EXPORT
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

extern DSAPI FieldInfo  Player_fields[];
extern DSAPI StructLayout Player_layout;

DSAPI void playerInit(void);
DSAPI void playerUpdate(Archetype *arch, f32 dt);
DSAPI void playerRender(Archetype *arch, Renderer *r);
DSAPI void playerDestroy(void);
DSAPI void druidGetECSSystem_Player(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif
