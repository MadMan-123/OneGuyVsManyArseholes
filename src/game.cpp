#include "game.h"
#include <stdio.h>
#include <string.h>

// ---- game state ----
static c8       g_projectDir[512] = {0};
static Camera  *g_cam = NULL;  // pointer to renderer's active camera
static u32      g_defaultShader = 0;
static f32      g_time = 0.0f;

// scene data loaded from .drsc files
static SceneData g_scene = {0};
static b8        g_sceneLoaded = false;
static b8        g_standaloneMode = false;

// entity field pointers (extracted from the loaded scene archetype)
static Vec3 *g_positions  = NULL;
static Vec4 *g_rotations  = NULL;
static Vec3 *g_scales     = NULL;
static b8   *g_isActive   = NULL;
static u32  *g_modelIDs   = NULL;
static u32  *g_shaderH    = NULL;
static u32  *g_matIDs     = NULL;
static b8   *g_sceneCameraFlags = NULL;
static u32   g_entityCount = 0;

static i32 findFieldIndex(const StructLayout *layout, const c8 *name)
{
    if (!layout || !layout->fields || !name) return -1;
    for (u32 i = 0; i < layout->count; i++)
    {
        if (strcmp(layout->fields[i].name, name) == 0)
            return (i32)i;
    }
    return -1;
}

// helper: build a full path from the project dir
static void scenePath(c8 *out, u32 sz, const c8 *name)
{
    snprintf(out, sz, "%s/scenes/%s", g_projectDir, name);
}

// Load a .drsc scene file and bind entity field pointers.
// Returns true on success.
static b8 loadGameScene(const c8 *name)
{
    c8 path[512];
    scenePath(path, sizeof(path), name);

    SceneData sd = loadScene(path);
    if (sd.archetypeCount == 0 || !sd.archetypes)
    {
        ERROR("Failed to load scene: %s", path);
        return false;
    }

    g_scene = sd;
    g_sceneLoaded = true;

    // grab SOA field pointers from the first archetype
    void **fields = getArchetypeFields(&g_scene.archetypes[0], 0);
    StructLayout *layout = g_scene.archetypes[0].layout;
    i32 posIdx = findFieldIndex(layout, "position");
    i32 rotIdx = findFieldIndex(layout, "rotation");
    i32 scaleIdx = findFieldIndex(layout, "scale");
    i32 activeIdx = findFieldIndex(layout, "isActive");
    i32 modelIdx = findFieldIndex(layout, "modelID");
    i32 shaderIdx = findFieldIndex(layout, "shaderHandle");
    i32 matIdx = findFieldIndex(layout, "materialID");
    i32 camIdx = findFieldIndex(layout, "isSceneCamera");
    if (posIdx < 0 || rotIdx < 0 || scaleIdx < 0 || activeIdx < 0 || modelIdx < 0)
    {
        ERROR("Scene is missing required fields");
        return false;
    }
    g_positions  = (Vec3 *)fields[posIdx];
    g_rotations  = (Vec4 *)fields[rotIdx];
    g_scales     = (Vec3 *)fields[scaleIdx];
    g_isActive   = (b8 *)  fields[activeIdx];
    g_modelIDs   = (u32 *) fields[modelIdx];
    g_shaderH    = (shaderIdx >= 0) ? (u32 *)fields[shaderIdx] : NULL;
    g_matIDs     = (matIdx >= 0) ? (u32 *)fields[matIdx] : NULL;
    g_sceneCameraFlags = (camIdx >= 0) ? (b8 *)fields[camIdx] : NULL;
    g_entityCount = g_scene.archetypes[0].arena[0].count;

    if (g_cam && g_sceneCameraFlags)
    {
        for (u32 i = 0; i < g_entityCount; i++)
        {
            if (g_isActive[i] && g_sceneCameraFlags[i])
            {
                g_cam->pos = g_positions[i];
                g_cam->orientation = g_rotations[i];
                break;
            }
        }
    }

    // apply loaded materials to the resource manager
    if (sd.materialCount > 0 && sd.materials)
    {
        u32 count = sd.materialCount;
        if (count > resources->materialCount)
            count = resources->materialCount;
        memcpy(resources->materialBuffer, sd.materials,
               sizeof(Material) * count);
        resources->materialUsed = count;
    }

    INFO("Loaded scene '%s' with %u entities", name, g_entityCount);
    return true;
}

static b8 loadStartupScene(void)
{
    c8 cfgPath[512];
    snprintf(cfgPath, sizeof(cfgPath), "%s/startup_scene.txt", g_projectDir);

    FILE *cfg = fopen(cfgPath, "rb");
    if (cfg)
    {
        c8 sceneName[256] = {0};
        size_t read = fread(sceneName, 1, sizeof(sceneName) - 1, cfg);
        fclose(cfg);
        sceneName[read] = '\0';
        for (size_t i = 0; sceneName[i]; i++)
        {
            if (sceneName[i] == '\r' || sceneName[i] == '\n')
            {
                sceneName[i] = '\0';
                break;
            }
        }
        if (sceneName[0])
            return loadGameScene(sceneName);
    }

    c8 scenesDir[512];
    snprintf(scenesDir, sizeof(scenesDir), "%s/scenes", g_projectDir);
    u32 fileCount = 0;
    c8 **files = listFilesInDirectory(scenesDir, &fileCount);
    if (files)
    {
        for (u32 i = 0; i < fileCount; i++)
        {
            u32 len = (u32)strlen(files[i]);
            if (len > 5 && strcmp(files[i] + len - 5, ".drsc") == 0)
            {
                const c8 *name = files[i];
                const c8 *slash = strrchr(name, '/');
                if (!slash) slash = strrchr(name, '\\');
                b8 ok = loadGameScene(slash ? slash + 1 : name);
                for (u32 j = 0; j < fileCount; j++) free(files[j]);
                free(files);
                return ok;
            }
        }

        for (u32 i = 0; i < fileCount; i++) free(files[i]);
        free(files);
    }

    return loadGameScene("scene.drsc");
}

// ---- plugin callbacks ----

static void gameInit(const c8 *projectDir)
{
    strncpy(g_projectDir, projectDir, sizeof(g_projectDir) - 1);
    g_standaloneMode = (strcmp(projectDir, ".") == 0);

    // Get the renderer's active camera.
    // In the editor, this is the scene camera at play start.
    // In standalone, the main.cpp creates the renderer + camera first.
    if (renderer)
        g_cam = (Camera *)bufferGet(&renderer->cameras, renderer->activeCamera);

    u32 idx = 0;
    findInMap(&resources->shaderIDs, "default", &idx);
    g_defaultShader = resources->shaderHandles[idx];

    loadStartupScene();
}

static void gameUpdate(f32 dt)
{
    g_time += dt;

    // Example: spin all active entities on the Y axis
    // for (u32 i = 0; i < g_entityCount; i++)
    // {
    //     if (!g_isActive[i]) continue;
    //     g_rotations[i] = quatFromAxisAngle(
    //         (Vec3){0.0f, 1.0f, 0.0f}, g_time);
    // }
}

static void gameRender(f32 dt)
{
    if (!g_sceneLoaded) return;
    if (!g_standaloneMode) return;

    // Camera is managed via the renderer (rendererBeginFrame pushes it to UBO).
    // To move the camera in your game, use:
    //   moveForward(g_cam, speed * dt);
    //   rotateY(g_cam, angle);
    // The editor calls rendererBeginFrame() each frame to upload it.

    glUseProgram(g_defaultShader);

    for (u32 id = 0; id < g_entityCount; id++)
    {
        if (!g_isActive[id]) continue;
        u32 modelID = g_modelIDs[id];
        if (modelID >= resources->modelUsed) continue;

        Model *model = &resources->modelBuffer[modelID];
        Transform t = {g_positions[id], g_rotations[id], g_scales[id]};
        u32 shaderToUse = (g_shaderH && g_shaderH[id] != 0) ? g_shaderH[id] : g_defaultShader;
        glUseProgram(shaderToUse);
        updateShaderModel(shaderToUse, t);

        for (u32 m = 0; m < model->meshCount; m++)
        {
            u32 mi = model->meshIndices[m];
            if (mi >= resources->meshUsed) continue;

            u32 matIdx = model->materialIndices[m];
            if (g_matIDs && g_matIDs[id] != (u32)-1)
                matIdx = g_matIDs[id];
            if (matIdx < resources->materialUsed)
            {
                MaterialUniforms unis = getMaterialUniforms(shaderToUse);
                updateMaterial(&resources->materialBuffer[matIdx], &unis);
            }
            drawMesh(&resources->meshBuffer[mi]);
        }
    }

    // Default forward-pass render for ECS archetypes with no custom render
    for (u32 a = 0; a < g_scene.archetypeCount; a++)
        rendererDefaultArchetypeRender(&g_scene.archetypes[a], renderer);
}

static void gameDestroy(void)
{
    // SceneData archetypes were malloc'd by loadScene – free them
    if (g_sceneLoaded && g_scene.archetypes)
    {
        for (u32 i = 0; i < g_scene.archetypeCount; i++)
            destroyArchetype(&g_scene.archetypes[i]);
        free(g_scene.archetypes);
        g_scene.archetypes = NULL;
    }
    if (g_scene.materials)
    {
        free(g_scene.materials);
        g_scene.materials = NULL;
    }
    g_sceneLoaded = false;
}

void druidGetPlugin(GamePlugin *out)
{
    out->init    = gameInit;
    out->update  = gameUpdate;
    out->render  = gameRender;
    out->destroy = gameDestroy;
}
