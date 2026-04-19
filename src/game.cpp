#include "game.h"
#include "Player.h"
#include "Bullet.h"

static Archetype g_playerArch = {0};
static Archetype g_bulletArch = {0};
static b8        g_playerCreated = false;
static b8        g_bulletCreated = false;

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

static void gameInit(const c8 *projectDir)
{
    runtimeCreate(projectDir, runtimeDefaultConfig());

    setupPlayer();
    setupBullets();
    runtimeRegisterArchetype(runtime, &g_playerArch);
    runtimeRegisterArchetype(runtime, &g_bulletArch);

    setMouseCaptured(true);
}

static void gameUpdate(f32 dt)
{
    if (g_playerCreated) playerUpdate(&g_playerArch, dt);
    if (g_bulletCreated) bulletUpdate(&g_bulletArch, dt);
    runtimeUpdate(runtime, dt);
}

static void gameRender(f32 dt)
{
    runtimeBeginScenePass(runtime, dt);
    if (g_playerCreated) rendererDefaultArchetypeRender(&g_playerArch, renderer);
    if (g_bulletCreated) rendererDefaultArchetypeRender(&g_bulletArch, renderer);
    runtimeEndScenePass(runtime);
}

static void gameDestroy(void)
{
    setMouseCaptured(false);
    if (g_playerCreated) { playerDestroy(); destroyArchetype(&g_playerArch); g_playerCreated = false; }
    if (g_bulletCreated) { bulletDestroy(); destroyArchetype(&g_bulletArch); g_bulletCreated = false; }
    runtimeDestroy(runtime);
}

void druidGetPlugin(GamePlugin *out)
{
    out->init    = gameInit;
    out->update  = gameUpdate;
    out->render  = gameRender;
    out->destroy = gameDestroy;
}
