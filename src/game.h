#pragma once
#include <druid.h>

#ifdef GAME_EXPORT
  #ifdef _WIN32
    #define GAME_API __declspec(dllexport)
  #else
    #define GAME_API __attribute__((visibility("default")))
  #endif
#else
  #ifdef _WIN32
    #define GAME_API __declspec(dllimport)
  #else
    #define GAME_API
  #endif
#endif

// Game plugin API.
typedef void (*PluginInitFn)(const c8 *projectDir);
typedef void (*PluginUpdateFn)(f32 dt);
typedef void (*PluginRenderFn)(f32 dt);
typedef void (*PluginDestroyFn)(void);

typedef struct GamePlugin {
    PluginInitFn    init;
    PluginUpdateFn  update;
    PluginRenderFn  render;
    PluginDestroyFn destroy;
} GamePlugin;

#ifdef __cplusplus
extern "C" {
#endif

GAME_API void druidGetPlugin(GamePlugin *out);


extern Archetype g_playerArch;
extern Archetype g_bulletArch;
extern Archetype g_gunArch;

#ifdef __cplusplus
}
#endif
