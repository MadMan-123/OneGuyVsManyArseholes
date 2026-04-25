#pragma once
#include <druid.h>

// Expected .wav files in the project's audio/ directory:
//   pistol_shot.wav     ak_shot.wav
//   pistol_reload.wav   ak_reload.wav     reload_done.wav
//   footstep.wav        player_hit.wav    enemy_hit.wav
//   zombie_death.wav    zombie_alert.wav
//   zombie_idle_0.wav   zombie_idle_1.wav  zombie_idle_2.wav

#ifdef __cplusplus
extern "C" {
#endif

void gameAudioInit(void);
void gameAudioUpdate(f32 dt);           // ambient zombie ticks — call once per frame

void gameAudioPlayShot(u32 weapon);
void gameAudioPlayReloadStart(u32 weapon);
void gameAudioPlayReloadDone(u32 weapon);
void gameAudioPlayFootstep(void);       // internally throttled
void gameAudioPlayPlayerHit(void);
void gameAudioPlayEnemyHit(void);
void gameAudioPlayEnemyDeath(void);
void gameAudioPlayZombieAlert(void);    // enemy transitions to chase

#ifdef __cplusplus
}
#endif
