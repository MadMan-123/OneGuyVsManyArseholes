#version 430

in vec2 tc;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D albedoTexture;
uniform float roughness;
uniform vec3  colour;
uniform float emissive;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

void main()
{
    gPosition = FragPos;
    gNormal   = normalize(Normal);

    vec3 diffuse = texture(albedoTexture, tc).rgb;
    vec3 albedo = diffuse;
    if (!(colour.r == 1.0 && colour.g == 1.0 && colour.b == 1.0))
        albedo = diffuse * colour;

    vec3 litAlbedo = albedo + (albedo * max(emissive, 0.0));
    gAlbedoSpec = vec4(litAlbedo, 1.0 - roughness);
}
