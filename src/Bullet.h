#pragma once
// Generated scaffold; gameplay edits are allowed.
#include <druid.h>

#ifdef __cplusplus
extern "C" {
#endif

extern DSAPI FieldInfo  Bullet_fields[];
extern DSAPI StructLayout Bullet_layout;

DSAPI void bulletInit(Archetype *arch);
DSAPI void bulletUpdate(Archetype *arch, f32 dt);
DSAPI void bulletRender(Archetype *arch, Renderer *r);
DSAPI void bulletDestroy(void);

DSAPI void bulletSpawn(Vec3 position, Vec3 direction, f32 speed);
DSAPI Archetype *bulletGetArchetype(void);
DSAPI void druidGetECSSystem_Bullet(ECSSystemPlugin *out);

#ifdef __cplusplus
}
#endif
