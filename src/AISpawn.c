#include "AISpawn.h"
#include "Enemy.h"
#include <string.h>

static i32 findFieldIndex(const StructLayout *layout, const c8 *name)
{
    if (!layout) return -1;
    for (u32 i = 0; i < layout->count; i++)
    {
        if (layout->fields[i].name && strcmp(layout->fields[i].name, name) == 0)
            return (i32)i;
    }
    return -1;
}

// Shared position reader.  If out_positions != NULL, fills the array instead of spawning.
// Returns number of matching entities processed.
static u32 processTaggedEntities(Archetype *arch, const c8 *matchTag,
                                  Vec3 *out_positions, u32 maxOut, u32 *outCount)
{
    if (!arch || !arch->layout) return 0;

    i32 tagIdx = findFieldIndex(arch->layout, "tag");
    if (tagIdx < 0) return 0;

    i32 pxIdx  = findFieldIndex(arch->layout, "positionX");
    i32 pyIdx  = findFieldIndex(arch->layout, "positionY");
    i32 pzIdx  = findFieldIndex(arch->layout, "positionZ");
    i32 posIdx = findFieldIndex(arch->layout, "position");

    u32 tagStride = arch->layout->fields[tagIdx].size;
    u32 matched   = 0;

    for (u32 ch = 0; ch < arch->activeChunkCount; ch++)
    {
        void **fields = getArchetypeFields(arch, ch);
        if (!fields) continue;
        u32 count = arch->arena[ch].count;

        c8   *tagBase = (c8   *)fields[tagIdx];
        Vec3 *posVec  = (posIdx >= 0) ? (Vec3 *)fields[posIdx]  : NULL;
        f32  *px      = (pxIdx  >= 0) ? (f32  *)fields[pxIdx]   : NULL;
        f32  *py      = (pyIdx  >= 0) ? (f32  *)fields[pyIdx]   : NULL;
        f32  *pz      = (pzIdx  >= 0) ? (f32  *)fields[pzIdx]   : NULL;

        for (u32 i = 0; i < count; i++)
        {
            const c8 *tag = &tagBase[i * tagStride];
            if (strncmp(tag, matchTag, tagStride) != 0) continue;

            Vec3 p = v3Zero;
            if (posVec)              p = posVec[i];
            else if (px && py && pz) p = (Vec3){ px[i], py[i], pz[i] };
            else continue;

            if (out_positions)
            {
                if (*outCount < maxOut)
                    out_positions[(*outCount)++] = p;
            }
            else
            {
                enemySpawnAt(p);
            }
            matched++;
        }
    }
    return matched;
}

static SceneRuntime *getScene(void)
{
    if (!sceneRuntime)         { WARN("aiSpawn: sceneRuntime is NULL");    return NULL; }
    if (!sceneRuntime->loaded) { WARN("aiSpawn: scene not yet loaded");    return NULL; }
    return sceneRuntime;
}

u32 aiSpawnFromScene(const c8 *matchTag)
{
    if (!matchTag) { WARN("aiSpawnFromScene: null matchTag"); return 0; }
    SceneRuntime *sr = getScene();
    if (!sr) return 0;

    u32 total = 0;
    for (u32 a = 0; a < sr->data.archetypeCount && a < MAX_SCENE_ARCHETYPES; a++)
        total += processTaggedEntities(&sr->data.archetypes[a], matchTag, NULL, 0, NULL);

    INFO("aiSpawnFromScene: spawned=%u tag='%s'", total, matchTag);
    return total;
}

u32 aiReadSpawnPoints(const c8 *matchTag, Vec3 *out_positions, u32 maxCount)
{
    if (!matchTag || !out_positions || maxCount == 0) return 0;
    SceneRuntime *sr = getScene();
    if (!sr) return 0;

    u32 found = 0;
    DEBUG("aiReadSpawnPoints: archetypeCount=%u", sr->data.archetypeCount);
    for (u32 a = 0; a < sr->data.archetypeCount && a < MAX_SCENE_ARCHETYPES; a++)
    {
        Archetype *arch = &sr->data.archetypes[a];
        DEBUG("  arch[%u]: activeChunks=%u layout=%p", a, arch->activeChunkCount, (void *)arch->layout);
        if (arch->layout)
        {
            i32 ti = findFieldIndex(arch->layout, "tag");
            DEBUG("  arch[%u]: tagFieldIdx=%d entityCount=%u", a, ti, archetypeEntityCount(arch));
        }
        processTaggedEntities(arch, matchTag, out_positions, maxCount, &found);
    }

    INFO("aiReadSpawnPoints: found=%u tag='%s'", found, matchTag);
    return found;
}
