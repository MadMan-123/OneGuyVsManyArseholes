#version 420

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

out vec4 FragColour;

void main()
{
    vec3 diffuse = texture(albedoTexture, tc).rgb;

    vec3 finalColor = diffuse;
    if (!(colour.r == 1.0 && colour.g == 1.0 && colour.b == 1.0))
        finalColor = diffuse * colour;

    FragColour = vec4(finalColor, transparency);
}
