#pragma once

// =============================================================================
// WAVE SYSTEM
// =============================================================================

// Seconds between each individual enemy spawn
#define WAVE_SPAWN_INTERVAL      1.5f

// Starting enemy cap (before any kills)
#define WAVE_INITIAL_CAP         2u

// Tier thresholds — kills required to enter each scaling tier
#define WAVE_TIER1_KILLS         10u
#define WAVE_TIER2_KILLS         40u

// Enemy cap at the start of each tier
#define WAVE_TIER1_CAP           5u
#define WAVE_TIER2_CAP           17u

// Kills per step in each tier
#define WAVE_TIER0_STEP_SIZE     3u    // tier 0: +WAVE_TIER0_STEP_ADD enemies every N kills
#define WAVE_TIER1_STEP_SIZE     5u    // tier 1: +WAVE_TIER1_STEP_ADD enemies every N kills
#define WAVE_TIER2_STEP_SIZE     10u   // tier 2: +WAVE_TIER2_STEP_ADD enemies every N kills

// Enemies added per step in each tier
#define WAVE_TIER0_STEP_ADD      1u
#define WAVE_TIER1_STEP_ADD      2u
#define WAVE_TIER2_STEP_ADD      5u

// Hard cap on total live enemies regardless of kills
#define WAVE_MAX_CAP             64u

// =============================================================================
// HEALTH
// =============================================================================

#define PLAYER_START_HP          100.0f
#define ENEMY_START_HP           50.0f

// =============================================================================
// DAMAGE
// =============================================================================

#define BULLET_DAMAGE            25.0f

#define ENEMY_MELEE_DAMAGE       10.0f
#define ENEMY_MELEE_CD           1.0f   // seconds between enemy melee hits

// =============================================================================
// GUN POSITIONING & ADS
// =============================================================================

// Hip-fire gun offset (relative to eye, in camera space)
#define GUN_HIP_X  0.30f
#define GUN_HIP_Y -0.20f
#define GUN_HIP_Z -0.50f

// ADS gun offset (relative to eye, in camera space)
#define GUN_ADS_X  0.00f
#define GUN_ADS_Y -0.10f
#define GUN_ADS_Z -0.60f

// Lerp speeds for ADS transitions
#define ADS_LERP_SPEED  10.0f
#define FOV_LERP_SPEED   8.0f

// =============================================================================
// WEAPONS
// =============================================================================

// Pistol
#define PISTOL_FIRE_RATE         0.15f  // seconds between shots
#define PISTOL_CLIP_SIZE         9.0f
#define PISTOL_RELOAD_TIME       1.8f
#define PISTOL_BULLET_SPEED      50.0f
#define PISTOL_SPREAD_MAX        0.01f
#define PISTOL_RECOIL            0.03f
#define PISTOL_RECOIL_FIRST      0.035f

// AK-47
#define AK_FIRE_RATE             0.1f   // seconds between shots
#define AK_CLIP_SIZE             30.0f
#define AK_RELOAD_TIME           3.1f
#define AK_BULLET_SPEED          70.0f
#define AK_SPREAD_MAX            0.06f
#define AK_SPREAD_RATE           0.012f
#define AK_SPREAD_DECAY          0.08f
#define AK_RECOIL                0.05f
#define AK_RECOIL_FIRST          0.10f

// SUOMI
#define SUOMI_FIRE_RATE             0.075f   // seconds between shots
#define SUOMI_CLIP_SIZE             55.0f
#define SUOMI_RELOAD_TIME           4.1f
#define SUOMI_BULLET_SPEED          60.0f
#define SUOMI_SPREAD_MAX            0.10f
#define SUOMI_SPREAD_RATE           0.015f
#define SUOMI_SPREAD_DECAY          0.09f
#define SUOMI_RECOIL                0.06f
#define SUOMI_RECOIL_FIRST          0.06f
