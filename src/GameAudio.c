#include "GameAudio.h"
#include <stdlib.h>

#define WEAPON_PISTOL 0
#define WEAPON_AK47   1

#define ZOMBIE_IDLE_VARIANTS  3
#define ZOMBIE_AMBIENT_MIN    15.0f
#define ZOMBIE_AMBIENT_MAX    30.0f
#define FOOTSTEP_INTERVAL     0.38f

static f32 s_footstepTimer = 0.0f;
static f32 s_zombieTimer   = 0.0f;

// Filenames — matched to res/audio/
#define SND_PISTOL_SHOT     "universfield-gunshot-352466.mp3"
#define SND_AK_SHOT         "universfield-gunshot-352466.mp3"
#define SND_PISTOL_RELOAD   "freesound_community-1911-reload-6248.mp3"
#define SND_AK_RELOAD       "freesound_community-1911-reload-6248.mp3"
#define SND_RELOAD_DONE     NULL
#define SND_FOOTSTEP        "freesound_community-step-1-81064.mp3"
#define SND_PLAYER_HIT      "universfield-punch-140236.mp3"
#define SND_ENEMY_HIT       "u_68csiaifb5-bulletimpact2-442718.mp3"
#define SND_ZOMBIE_DEATH    "dragon-studio-zombie-dying-sound-357974.mp3"
#define SND_ZOMBIE_ALERT    "dragon-studio-zombie-sfx-450450.mp3"
#define SND_ZOMBIE_IDLE_0   "dragon-studio-zombie-sfx-450450.mp3"
#define SND_ZOMBIE_IDLE_1   "vilches86-zombie-death-15965.mp3"
#define SND_ZOMBIE_IDLE_2   "dragon-studio-zombie-dying-sound-357974.mp3"

static const c8 *s_zombieIdleSounds[ZOMBIE_IDLE_VARIANTS] = {
    SND_ZOMBIE_IDLE_0,
    SND_ZOMBIE_IDLE_1,
    SND_ZOMBIE_IDLE_2,
};

void gameAudioInit(void)
{
    s_footstepTimer = 0.0f;
    s_zombieTimer   = ZOMBIE_AMBIENT_MIN +
        ((f32)(rand() % 1000) / 1000.0f) * (ZOMBIE_AMBIENT_MAX - ZOMBIE_AMBIENT_MIN);
}

void gameAudioUpdate(f32 dt)
{
    if (s_footstepTimer > 0.0f) s_footstepTimer -= dt;

    s_zombieTimer -= dt;
    if (s_zombieTimer <= 0.0f)
    {
        s_zombieTimer = ZOMBIE_AMBIENT_MIN +
            ((f32)(rand() % 1000) / 1000.0f) * (ZOMBIE_AMBIENT_MAX - ZOMBIE_AMBIENT_MIN);
        playSound(s_zombieIdleSounds[rand() % ZOMBIE_IDLE_VARIANTS], 0.12f);
    }
}

void gameAudioPlayShot(u32 weapon)
{
    playSound(weapon == WEAPON_AK47 ? SND_AK_SHOT : SND_PISTOL_SHOT, 0.9f);
}

void gameAudioPlayReloadStart(u32 weapon)
{
    playSound(weapon == WEAPON_AK47 ? SND_AK_RELOAD : SND_PISTOL_RELOAD, 0.8f);
}

void gameAudioPlayReloadDone(u32 weapon)
{
    (void)weapon;
    if (SND_RELOAD_DONE) playSound(SND_RELOAD_DONE, 0.7f);
}

void gameAudioPlayFootstep(void)
{
    if (s_footstepTimer > 0.0f) return;
    s_footstepTimer = FOOTSTEP_INTERVAL;
    playSound(SND_FOOTSTEP, 0.5f);
}

void gameAudioPlayPlayerHit(void)  { playSound(SND_PLAYER_HIT,   1.0f); }
void gameAudioPlayEnemyHit(void)   { playSound(SND_ENEMY_HIT,    0.75f); }
void gameAudioPlayEnemyDeath(void) { playSound(SND_ZOMBIE_DEATH, 0.25f); }
void gameAudioPlayZombieAlert(void){ playSound(SND_ZOMBIE_ALERT, 0.2f);  }
