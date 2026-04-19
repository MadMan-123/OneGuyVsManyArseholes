#include "game.h"

#ifndef DRUID_APP_TITLE
#define DRUID_APP_TITLE "Game"
#endif

static GamePlugin    plugin = {0};
static Application  *g_app  = NULL;

static void _init(void)        { plugin.init("."); }
static void _update(f32 dt)    { plugin.update(dt); }
static void _render(f32 dt)    { plugin.render(dt); }
static void _destroy(void)     { plugin.destroy(); }

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    druidGetPlugin(&plugin);
    g_app = createApplication(DRUID_APP_TITLE, _init, _update, _render, _destroy);
    g_app->width  = 1280;
    g_app->height = 720;
    run(g_app);
    return 0;
}
