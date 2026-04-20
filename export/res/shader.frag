#version 430

in vec2 tc;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D albedoTexture;
uniform sampler2D metallicTexture;
uniform sampler2D roughnessTexture;
uniform sampler2D normalTexture;
uniform float roughness;
uniform float metallic;
uniform float transparency;
uniform vec3  colour;

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

vec3 calcLight(Light l, vec3 N, vec3 V, vec3 albedo)
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
        vec3 toLight = lightPos - FragPos;
        float dist = length(toLight);
        L = toLight / max(dist, 0.0001);

        // distance attenuation
        float falloff = clamp(1.0 - (dist / max(l.range, 0.001)), 0.0, 1.0);
        atten = falloff * falloff;

        // spot cone attenuation
        if (l.type == 2u)
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
    float shininess = mix(8.0, 256.0, 1.0 - roughness);
    float spec = pow(max(dot(N, H), 0.0), shininess) * (1.0 - roughness);

    return (diff * albedo + spec * vec3(0.3)) * lightCol * atten;
}

void main()
{
    vec3 diffuse = texture(albedoTexture, tc).rgb;
    vec3 albedo = diffuse;
    if (!(colour.r == 1.0 && colour.g == 1.0 && colour.b == 1.0))
        albedo = diffuse * colour;

    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - FragPos);

    // ambient
    vec3 result = albedo * 0.08;

    // accumulate lights
    for (uint i = 0u; i < lightCount; i++)
    {
        result += calcLight(lights[i], N, V, albedo);
    }

    // fallback: if no lights, show unlit so things aren't black
    if (lightCount == 0u)
        result = albedo;

    FragColour = vec4(result, transparency);
}
