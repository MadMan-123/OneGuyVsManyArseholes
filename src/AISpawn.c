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

static u32 spawnFromArchetype(Archetype *arch, const c8 *matchTag)
{
    if (!arch || !arch->layout) return 0;

    i32 tagIdx  = findFieldIndex(arch->layout, "tag");
    if (tagIdx < 0) return 0;

    i32 pxIdx   = findFieldIndex(arch->layout, "positionX");
    i32 pyIdx   = findFieldIndex(arch->layout, "positionY");
    i32 pzIdx   = findFieldIndex(arch->layout, "positionZ");
    i32 posIdx  = findFieldIndex(arch->layout, "position");
    i32 nameIdx = findFieldIndex(arch->layout, "name");

    u32 tagStride  = arch->layout->fields[tagIdx].size;
    u32 nameStride = (nameIdx >= 0) ? arch->layout->fields[nameIdx].size : 0;

    INFO("spawnFromArchetype: layout='%s' chunks=%u tagStride=%u posIdx=%d pxIdx=%d",
         arch->layout->name ? arch->layout->name : "(null)",
         arch->activeChunkCount, tagStride, posIdx, pxIdx);

    u32 spawned = 0;
    for (u32 ch = 0; ch < arch->activeChunkCount; ch++)
    {
        void **fields = getArchetypeFields(arch, ch);
        if (!fields) continue;
        u32 count = arch->arena[ch].count;

        c8  *tagBase  = (c8 *)fields[tagIdx];
        c8  *nameBase = (nameIdx >= 0) ? (c8 *)fields[nameIdx] : NULL;
        Vec3 *posVec  = (posIdx >= 0) ? (Vec3 *)fields[posIdx] : NULL;
        f32  *px = (pxIdx >= 0) ? (f32 *)fields[pxIdx] : NULL;
        f32  *py = (pyIdx >= 0) ? (f32 *)fields[pyIdx] : NULL;
        f32  *pz = (pzIdx >= 0) ? (f32 *)fields[pzIdx] : NULL;

        for (u32 i = 0; i < count; i++)
        {
            const c8 *tag = &tagBase[i * tagStride];
            const c8 *nm  = nameBase ? &nameBase[i * nameStride] : "";
            INFO("  ent[%u] name='%s' tag='%s'", i, nm, tag);

            if (strncmp(tag, matchTag, tagStride) != 0) continue;

            Vec3 p = v3Zero;
            if (posVec)              p = posVec[i];
            else if (px && py && pz) p = (Vec3){ px[i], py[i], pz[i] };
            else continue;

            INFO("  -> MATCH: spawning enemy at (%.2f, %.2f, %.2f)", p.x, p.y, p.z);
            if (enemySpawnAt(p)) spawned++;
        }
    }
    return spawned;
}

u32 aiSpawnFromScene(const c8 *matchTag)
{
    if (!matchTag) { WARN("aiSpawnFromScene: null matchTag"); return 0; }

    SceneRuntime *sr = (runtime ? runtime->scene : NULL);
    if (!sr) { WARN("aiSpawnFromScene: runtime->scene is NULL"); return 0; }
    if (!sr->loaded) { WARN("aiSpawnFromScene: scene not yet loaded"); return 0; }

    INFO("aiSpawnFromScene: runtime->scene has %u archetypes, %u entities",
         sr->data.archetypeCount, sr->entityCount);

    u32 total = 0;
    for (u32 a = 0; a < sr->data.archetypeCount && a < MAX_SCENE_ARCHETYPES; a++)
    {
        INFO("  scene archetype[%u] name='%s'", a, sr->data.archetypeNames[a]);
        total += spawnFromArchetype(&sr->data.archetypes[a], matchTag);
    }

    INFO("aiSpawnFromScene: total spawned=%u tag='%s'", total, matchTag);
    return total;
}
