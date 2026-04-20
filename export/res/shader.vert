#version 430

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

layout (std140, binding = 0) uniform CoreShaderData
{
    vec3  camPos;
    float time;
    mat4  view;
    mat4  projection;
};

uniform mat4 model;

out vec2 tc;
out vec3 Normal;
out vec3 FragPos;
out mat3 TBN;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    FragPos = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * normal);

    vec3 N = Normal;
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    tc = texCoord;
    gl_Position = projection * view * worldPos;
}
