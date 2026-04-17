#include "game.h"
#include "Player.h"
#include "Bullet.h"
#include <stdio.h>
#include <string.h>

// Game state.
static c8       g_projectDir[512] = {0};
static Camera  *g_cam = NULL;
static b8        g_standaloneMode = false;

// Standalone runtime state.
static u32      g_defaultShader = 0;
static u32      g_gbufferShader = 0;
static u32      g_lightingShader = 0;

static SceneData g_scene = {0};
static b8        g_sceneLoaded = false;

static Archetype g_playerArch = {0};
static Archetype g_bulletArch = {0};
static b8        g_playerCreated = false;
static b8        g_bulletCreated = false;
static b8        g_playerPhysRegistered = false;
static b8        g_bulletPhysRegistered = false;

static FieldInfo g_floorFields[] = {
    { "PositionX",       sizeof(f32),  FIELD_TEMP_HOT  },
    { "PositionY",       sizeof(f32),  FIELD_TEMP_HOT  },
    { "PositionZ",       sizeof(f32),  FIELD_TEMP_HOT  },
    { "Rotation",        sizeof(Vec4), FIELD_TEMP_HOT  },
    { "Scale",           sizeof(Vec3), FIELD_TEMP_HOT  },
    { "PhysicsBodyType", sizeof(u32),  FIELD_TEMP_HOT  },
    { "Mass",            sizeof(f32),  FIELD_TEMP_HOT  },
    { "InvMass",         sizeof(f32),  FIELD_TEMP_HOT  },
    { "Restitution",     sizeof(f32),  FIELD_TEMP_HOT  },
    { "ColliderHalfX",   sizeof(f32),  FIELD_TEMP_HOT  },
    { "ColliderHalfY",   sizeof(f32),  FIELD_TEMP_HOT  },
    { "ColliderHalfZ",   sizeof(f32),  FIELD_TEMP_HOT  },
    { "ModelID",         sizeof(u32),  FIELD_TEMP_COLD },
};
static StructLayout g_floorLayout  = { "Floor", g_floorFields, sizeof(g_floorFields) / sizeof(FieldInfo) };
static Archetype    g_floorArch    = {0};
static b8           g_floorCreated = false;

// Scene field pointers.
static Vec3 *g_positions  = NULL;
static Vec4 *g_rotations  = NULL;
static Vec3 *g_scales     = NULL;
static b8   *g_isActive   = NULL;
static u32  *g_modelIDs   = NULL;
static u32  *g_matIDs     = NULL;
static b8   *g_sceneCameraFlags = NULL;
static u32   g_entityCount = 0;

// Helpers.

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

// Scene loading.

static b8 loadGameScene(const c8 *name)
{
    c8 path[512];
    snprintf(path, sizeof(path), "%s/scenes/%s", g_projectDir, name);

    SceneData sd = loadScene(path);
    if (sd.archetypeCount == 0 || !sd.archetypes)
    {
        ERROR("Failed to load scene: %s", path);
        return false;
    }

    g_scene = sd;
    g_sceneLoaded = true;

    void **fields = getArchetypeFields(&g_scene.archetypes[0], 0);
    StructLayout *layout = g_scene.archetypes[0].layout;
    i32 posIdx    = findFieldIndex(layout, "position");
    i32 rotIdx    = findFieldIndex(layout, "rotation");
    i32 scaleIdx  = findFieldIndex(layout, "scale");
    i32 activeIdx = findFieldIndex(layout, "isActive");
    i32 modelIdx  = findFieldIndex(layout, "modelID");
    i32 matIdx    = findFieldIndex(layout, "materialID");
    i32 camIdx    = findFieldIndex(layout, "isSceneCamera");
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

    if (sd.materialCount > 0 && sd.materials)
    {
        u32 count = sd.materialCount;
        if (count > resources->materialCount) count = resources->materialCount;
        memcpy(resources->materialBuffer, sd.materials, sizeof(Material) * count);
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
        size_t rd = fread(sceneName, 1, sizeof(sceneName) - 1, cfg);
        fclose(cfg);
        sceneName[rd] = '\0';
        for (size_t i = 0; sceneName[i]; i++)
        {
            if (sceneName[i] == '\r' || sceneName[i] == '\n')
            { sceneName[i] = '\0'; break; }
        }
        if (sceneName[0]) return loadGameScene(sceneName);
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
                const c8 *fn = files[i];
                const c8 *slash = strrchr(fn, '/');
                if (!slash) slash = strrchr(fn, '\\');
                b8 ok = loadGameScene(slash ? slash + 1 : fn);
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

// Archetype setup.



static void setupPlayer(void)
{
    g_playerArch.flags = 0;
    FLAG_SET(g_playerArch.flags, ARCH_SINGLE);
    FLAG_SET(g_playerArch.flags, ARCH_PHYSICS_BODY);

    if (!createArchetype(&Player_layout, 1, &g_playerArch))
    { ERROR("Failed to create Player archetype"); return; }
    g_playerCreated = true;

    u64 entity = 0;
    createEntityInArchetype(&g_playerArch, &entity);

    void **fields = getArchetypeFields(&g_playerArch, 0);
    if (fields)
    {
        Vec4 iRot = {0, 0, 0, 1};
        Vec3 pScl = {0.8f, 1.8f, 0.8f};
        ((f32 *)fields[PF_POS_X])[0]      = 0.0f;
        ((f32 *)fields[PF_POS_Y])[0]      = 5.0f;
        ((f32 *)fields[PF_POS_Z])[0]      = 0.0f;
        ((Vec4 *)fields[PF_ROT])[0]        = iRot;
        ((Vec3 *)fields[PF_SCALE])[0]      = pScl;

        ((u32 *)fields[PF_BODY_TYPE])[0]   = PHYS_BODY_DYNAMIC;
        ((f32 *)fields[PF_MASS])[0]        = 80.0f;
        ((f32 *)fields[PF_RESTITUTION])[0] = 0.0f;
        ((f32 *)fields[PF_DAMPING])[0]     = 0.1f;
        ((f32 *)fields[PF_HALF_X])[0]      = 0.4f;
        ((f32 *)fields[PF_HALF_Y])[0]      = 0.9f;
        ((f32 *)fields[PF_HALF_Z])[0]      = 0.4f;
    }
    playerInit();
}

static void setupBullets(void)
{
    g_bulletArch.flags = 0;
    FLAG_SET(g_bulletArch.flags, ARCH_BUFFERED);
    FLAG_SET(g_bulletArch.flags, ARCH_PHYSICS_BODY);

    if (!createArchetype(&Bullet_layout, 256, &g_bulletArch))
    { ERROR("Failed to create Bullet archetype"); return; }
    g_bulletCreated = true;

    bulletInit(&g_bulletArch);
}

// Standalone rendering path.

static void standaloneRenderGeometry(u32 shader)
{
    glUseProgram(shader);

    u32 prev = renderer ? renderer->defaultShader : 0;
    if (renderer) renderer->defaultShader = shader;

    if (g_sceneLoaded)
    {
        for (u32 id = 0; id < g_entityCount; id++)
        {
            if (!g_isActive[id]) continue;
            u32 modelID = g_modelIDs[id];
            if (modelID >= resources->modelUsed) continue;

            Model *model = &resources->modelBuffer[modelID];
            Transform t = {g_positions[id], g_rotations[id], g_scales[id]};
            updateShaderModel(shader, t);

            for (u32 m = 0; m < model->meshCount; m++)
            {
                u32 mi = model->meshIndices[m];
                if (mi >= resources->meshUsed) continue;
                u32 matI = model->materialIndices[m];
                if (g_matIDs && g_matIDs[id] != (u32)-1) matI = g_matIDs[id];
                if (matI < resources->materialUsed)
                {
                    MaterialUniforms unis = getMaterialUniforms(shader);
                    updateMaterial(&resources->materialBuffer[matI], &unis);
                }
                drawMesh(&resources->meshBuffer[mi]);
            }
        }
    }

    if (g_playerCreated) rendererDefaultArchetypeRender(&g_playerArch, renderer);
    
    // Bullets are rendered manually here.
    if (g_bulletCreated && renderer && resources)
    {
        for (u32 _ch = 0; _ch < g_bulletArch.activeChunkCount; _ch++)
        {
            void **fields = getArchetypeFields(&g_bulletArch, _ch);
            if (!fields) continue;
            u32 count = g_bulletArch.arena[_ch].count;

            b8   *alive   = (b8  *)fields[BF_ALIVE];
            f32  *posX    = (f32 *)fields[BF_POS_X];
            f32  *posY    = (f32 *)fields[BF_POS_Y];
            f32  *posZ    = (f32 *)fields[BF_POS_Z];
            Vec4 *rot     = (Vec4 *)fields[BF_ROT];
            Vec3 *scl     = (Vec3 *)fields[BF_SCALE];
            u32  *modelID = (u32 *)fields[BF_MODEL_ID];

            for (u32 i = 0; i < count; i++)
            {
                if (!alive[i]) continue;
                u32 mID = modelID[i];
                if (mID >= resources->modelUsed) continue;

                Model *model = &resources->modelBuffer[mID];
                Transform t = {{posX[i], posY[i], posZ[i]}, rot[i], scl[i]};
                updateShaderModel(shader, t);

                for (u32 m = 0; m < model->meshCount; m++)
                {
                    u32 mi = model->meshIndices[m];
                    if (mi >= resources->meshUsed) continue;
                    drawMesh(&resources->meshBuffer[mi]);
                }
            }
        }
    }

    if (renderer) renderer->defaultShader = prev;
}

// Plugin callbacks.

static void gameInit(const c8 *projectDir)
{
    strncpy(g_projectDir, projectDir, sizeof(g_projectDir) - 1);
    g_standaloneMode = (strcmp(projectDir, ".") == 0);

    if (renderer)
        g_cam = (Camera *)bufferGet(&renderer->cameras, renderer->activeCamera);

    if (g_standaloneMode)
    {
        u32 idx = 0;
        findInMap(&resources->shaderIDs, "default", &idx);
        g_defaultShader = resources->shaderHandles[idx];

        loadStartupScene();

        g_gbufferShader  = createGraphicsProgram("./res/gbuffer.vert", "./res/gbuffer.frag");
        g_lightingShader = createGraphicsProgram("./res/deferred_lighting.vert", "./res/deferred_lighting.frag");

        if (renderer && g_gbufferShader && g_lightingShader)
            rendererEnableDeferred(renderer, 1280, 720);
    }

    // Gameplay archetypes are needed in both standalone and editor play mode.
    // Physics world ownership differs by mode:
    // - standalone: game plugin owns physics init/step
    // - editor play: editor owns physics world lifetime
    if (g_standaloneMode)
    {
        Vec3 gravity = {0.0f, -9.81f, 0.0f};
        physInit(gravity, 1.0f / 60.0f);
    }

    setupPlayer();
    setupBullets();

    g_playerPhysRegistered = false;
    g_bulletPhysRegistered = false;

    if (physicsWorld)
    {
        if (g_playerCreated) physRegisterArchetype(physicsWorld, &g_playerArch);
        if (g_bulletCreated) physRegisterArchetype(physicsWorld, &g_bulletArch);
        g_playerPhysRegistered = g_playerCreated;
        g_bulletPhysRegistered = g_bulletCreated;
    }

    setMouseCaptured(true);
}

static void gameUpdate(f32 dt)
{
    // In editor play mode the editor may initialize/reset physics after plugin init,
    // so lazily (re)register gameplay archetypes when the world is available.
    if (physicsWorld)
    {
        if (g_playerCreated && !g_playerPhysRegistered)
        {
            physRegisterArchetype(physicsWorld, &g_playerArch);
            g_playerPhysRegistered = true;
        }
        if (g_bulletCreated && !g_bulletPhysRegistered)
        {
            physRegisterArchetype(physicsWorld, &g_bulletArch);
            g_bulletPhysRegistered = true;
        }
    }

    if (g_playerCreated) playerUpdate(&g_playerArch, dt);
    if (g_bulletCreated) bulletUpdate(&g_bulletArch, dt);
    if (g_standaloneMode && physicsWorld)
        physWorldStep(physicsWorld, dt);
}

static void gameRender(f32 dt)
{
    (void)dt;
    if (!g_standaloneMode)
    {
        // In editor play mode, draw gameplay-owned archetypes here.
        if (renderer)
        {
            if (g_playerCreated) rendererDefaultArchetypeRender(&g_playerArch, renderer);
            if (g_bulletCreated) bulletRender(&g_bulletArch, renderer);
        }
        return;
    }

    if (renderer && renderer->useDeferredRendering && g_gbufferShader && g_lightingShader)
    {
        rendererBeginDeferredPass(renderer);
        standaloneRenderGeometry(g_gbufferShader);
        rendererEndDeferredPass(renderer);
        rendererLightingPass(renderer, g_lightingShader);
    }
    else
    {
        standaloneRenderGeometry(g_defaultShader);
    }
}

static void gameDestroy(void)
{
    setMouseCaptured(false);

    if (g_playerCreated || g_bulletCreated || physicsWorld || g_standaloneMode)
    {
        if (g_playerCreated) { playerDestroy(); destroyArchetype(&g_playerArch); }
        if (g_bulletCreated) { bulletDestroy(); destroyArchetype(&g_bulletArch); }
        physShutdown();
        g_playerPhysRegistered = false;
        g_bulletPhysRegistered = false;

        if (g_sceneLoaded && g_scene.archetypes)
        {
            for (u32 i = 0; i < g_scene.archetypeCount; i++)
                destroyArchetype(&g_scene.archetypes[i]);
            free(g_scene.archetypes);
            g_scene.archetypes = NULL;
        }
        if (g_scene.materials) { free(g_scene.materials); g_scene.materials = NULL; }
        g_sceneLoaded = false;
    }
}

void druidGetPlugin(GamePlugin *out)
{
    out->init    = gameInit;
    out->update  = gameUpdate;
    out->render  = gameRender;
    out->destroy = gameDestroy;
}
