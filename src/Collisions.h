#pragma once
#include <druid.h>
#include "Health.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ENEMY_MELEE_DAMAGE   10.0f
#define ENEMY_MELEE_CD       1.0f   // seconds between melee hits
#define BULLET_DAMAGE        25.0f

DSAPI extern HealthManager g_healthManager;
DSAPI extern DamageQueue   g_damageQueue;

DSAPI void collisionsInit(void);

// Callbacks set on archetypes
DSAPI void onBulletCollideEnter(ContactInfo *info);
DSAPI void onEnemyCollideEnter(ContactInfo *info);

#ifdef __cplusplus
}
#endif
