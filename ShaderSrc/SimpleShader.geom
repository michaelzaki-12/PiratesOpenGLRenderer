#version 460 core
layout (triangles) in;
layout (triangle_strip, max_vertices=36) out;

uniform mat4 shadowMatrices[6];

out vec4 FragPos; // FragPos from GS (output per emitvertex)
uniform int lightindex;

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        int lldx = lightindex;
        gl_Layer = face + (6 * lldx); // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle vertex
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }    
        EndPrimitive();
    }
}
