#version 330 core
in vec2 vTexCoord;
in vec3 vColor;
out vec4 FragColor;

uniform sampler2D tex;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 texColor = texture(tex, vTexCoord);
        FragColor = texColor * vec4(vColor, 1.0);
    } else {
        FragColor = vec4(vColor, 1.0);
    }
}
