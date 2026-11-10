#version 460 core
layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

uniform vec3 viewPos;

out VS_OUT {
    vec2 texCoords;
    vec3 N;
    vec3 T;
    vec3 B;
    mat3 TBN;
} vs_out;
out vec3 FragPos;
out vec2 TexCoords;
layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

uniform mat4 model;
/*
struct Light {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 color;
};
*/
void main(){
    FragPos = vec3(view * model * vec4(aPos, 1.0));
    TexCoords = aTexCoord;
    vs_out.T = normalize(vec3(view * model * vec4(aTangent, 0.0)));
    vs_out.N = normalize(vec3(view * model * vec4(aNormal, 0.0)));
    // re-orthogonalize T with respect to N
    vs_out.T = normalize(vs_out.T - dot(vs_out.T, vs_out.N) * vs_out.N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vs_out.B = cross(vs_out.N, vs_out.T);

    vs_out.TBN = mat3(vs_out.T, vs_out.B, vs_out.N);

    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}       