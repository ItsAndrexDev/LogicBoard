#version 330 core

layout (location = 0) in vec3 aPos;      // matches Vertex.position
layout (location = 1) in vec3 aColor;    // matches Vertex.color
layout (location = 2) in vec2 aTexCoord; // matches Vertex.texCoord

out vec3 vColor;
out vec2 vTexCoord;

uniform mat4 model;
uniform mat4 projection;

void main()
{
    vColor = aColor;
    vTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y); // Flip the y-coordinate
    gl_Position = projection * model * vec4(aPos, 1.0);
}
