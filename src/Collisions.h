#pragma once
#include <druid.h>
#include "Health.h"
#include "GameConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

DSAPI extern HealthManager g_healthManager;
DSAPI extern DamageQueue   g_damageQueue;

DSAPI void collisionsInit(void);

// Callbacks set on archetypes
DSAPI void onBulletCollideEnter(ContactInfo *info);
DSAPI void onEnemyCollideEnter(ContactInfo *info);

#ifdef __cplusplus
}
#endif
