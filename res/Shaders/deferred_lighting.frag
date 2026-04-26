#version 430

in vec2 TexCoord;

// GBuffer textures (bound by rendererLightingPass)
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform samplerCube envMap;

layout (std140, binding = 0) uniform CoreShaderData
{
    vec3  camPos;
    float time;
    mat4  view;
    mat4  projection;
};

struct Light
{
    float posX, posY, posZ;
    float range;
    float colorR, colorG, colorB;
    float intensity;
    float dirX, dirY, dirZ;
    float innerCone;
    float outerCone;
    uint  type;
    float _pad0, _pad1;
};

layout (std430, binding = 3) buffer LightBuffer
{
    uint  lightCount;
    uint  _pad[3];
    Light lights[];
};

out vec4 FragColour;

vec3 calcLight(Light l, vec3 fragPos, vec3 N, vec3 V, vec3 albedo, float specPower)
{
    vec3 lightPos = vec3(l.posX, l.posY, l.posZ);
    vec3 lightCol = vec3(l.colorR, l.colorG, l.colorB) * l.intensity;
    vec3 lightDir = vec3(l.dirX, l.dirY, l.dirZ);

    vec3 L;
    float atten = 1.0;

    if (l.type == 1u) // directional
    {
        L = normalize(-lightDir);
    }
    else // point or spot
    {
        vec3 toLight = lightPos - fragPos;
        float dist = length(toLight);
        L = toLight / max(dist, 0.0001);

        float falloff = clamp(1.0 - (dist / max(l.range, 0.001)), 0.0, 1.0);
        atten = falloff * falloff;

        if (l.type == 2u) // spot
        {
            float theta = dot(L, normalize(-lightDir));
            float epsilon = l.innerCone - l.outerCone;
            float spotAtten = clamp((theta - l.outerCone) / max(epsilon, 0.0001), 0.0, 1.0);
            atten *= spotAtten;
        }
    }

    // diffuse
    float diff = max(dot(N, L), 0.0);

    // specular (Blinn-Phong)
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), specPower);

    return (diff * albedo + spec * vec3(0.3)) * lightCol * atten;
}

void main()
{
    vec3 fragPos = texture(gPosition, TexCoord).rgb;
    vec3 normal  = texture(gNormal, TexCoord).rgb;
    vec4 albedoSpec = texture(gAlbedoSpec, TexCoord);
    vec3 albedo  = albedoSpec.rgb;
    float specular = albedoSpec.a;

    // discard empty pixels (no geometry was written here)
    if (length(normal) < 0.01)
        discard;

    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - fragPos);
    float specPower = mix(8.0, 256.0, specular);

    // ambient
    vec3 result = albedo * 0.08;

    // accumulate all lights
    for (uint i = 0u; i < lightCount; i++)
    {
        result += calcLight(lights[i], fragPos, N, V, albedo, specPower);
    }

    // environment reflection
    if (specular > 0.1)
    {
        vec3 R = reflect(-V, N);
        vec3 envColor = texture(envMap, R).rgb;
        result += envColor * specular * 0.2;
    }

    // fallback: no lights = unlit
    if (lightCount == 0u)
        result = albedo;

    FragColour = vec4(result, 1.0);
}
