#include "game.h"
#include <stdio.h>

#ifndef DRUID_APP_TITLE
#define DRUID_APP_TITLE "Game"
#endif

static GamePlugin plugin = {0};
static Application *g_app = NULL;
static Mesh *g_skyboxMesh = NULL;
static u32   g_skyboxTex = 0;
static u32   g_skyboxShader = 0;

static void loadStandaloneSkybox(void)
{
    const c8 *suffixes[6] = {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"};
    c8 paths[6][512];
    const c8 *faces[6];
    for (u32 i = 0; i < 6; i++)
    {
        snprintf(paths[i], sizeof(paths[i]), "./res/Textures/Skybox/%s", suffixes[i]);
        if (!fileExists(paths[i]))
            return;
        faces[i] = paths[i];
    }

    g_skyboxTex = createCubeMapTexture(faces, 6);
    g_skyboxMesh = createSkyboxMesh();
    g_skyboxShader = createGraphicsProgram("./res/Skybox.vert", "./res/Skybox.frag");
}

static void _init(void)
{
    // Create renderer in standalone mode.
    if (!renderer && g_app && g_app->display)
    {
        Renderer *r = createRenderer(g_app->display, 70.0f, 0.1f, 100.0f, 8, 16, 8);
        if (r)
        {
            u32 idx = 0;
            findInMap(&resources->shaderIDs, "default", &idx);
            r->defaultShader = resources->shaderHandles[idx];
            // Create gameplay camera.
            Vec3 camStartPos = {0.0f, 2.0f, 8.0f};
            u32 camSlot = rendererAcquireCamera(r, camStartPos, 70.0f,
                                                16.0f / 9.0f, 0.1f, 100.0f);
            rendererSetActiveCamera(r, camSlot);
        }
    }
    loadStandaloneSkybox();
    plugin.init(".");
}
static void _update(f32 dt)
{
    plugin.update(dt);
    if (renderer) rendererBeginFrame(renderer, dt);
}
static void _render(f32 dt)
{
    if (g_skyboxMesh && g_skyboxTex && g_skyboxShader)
    {
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glUseProgram(g_skyboxShader);
        glBindVertexArray(g_skyboxMesh->vao);
        glBindTexture(GL_TEXTURE_CUBE_MAP, g_skyboxTex);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }
    plugin.render(dt);
}
static void _destroy(void)
{
    plugin.destroy();
    if (g_skyboxMesh) freeMesh(g_skyboxMesh);
    if (g_skyboxTex) freeTexture(g_skyboxTex);
    if (g_skyboxShader) freeShader(g_skyboxShader);
}

int main(int argc, char **argv)
{
    druidGetPlugin(&plugin);
    g_app = createApplication(DRUID_APP_TITLE, _init, _update, _render, _destroy);
    g_app->width  = 1280;
    g_app->height = 720;
    run(g_app);
    return 0;
}
