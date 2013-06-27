#version 400 core

// Tessellation Evaluation Shader (Domain Shader in HLSL)

uniform mat4 g_mWorldViewProjection;

layout(triangles, equal_spacing, cw) in;

void main()
{
    // Baricentric interpolation
    vec3 finalPos = (gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position).xyz;
    gl_Position = g_mWorldViewProjection * vec4(finalPos, 1.0);
}
