#include "Renderer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui/imgui.h"

GLuint Renderer::compileShader(const char* path, GLenum type) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open shader file: " << path << std::endl;
        return 0;
    }
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const char* src = source.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION (" << path << ")\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint Renderer::createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    GLuint vertex = compileShader(vertexPath, GL_VERTEX_SHADER);
    GLuint fragment = compileShader(fragmentPath, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM_LINKING\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

Renderer::ShaderRenderer::ShaderRenderer(const char* vertexPath, const char* fragmentPath, GLFWwindow* window)
    : window(window)
{
    shaderProgram = createShaderProgram(vertexPath, fragmentPath);
    glUseProgram(shaderProgram);

    // enable blending (also enabled in main, but safe to have here)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // set resolution uniform if shader uses it
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    GLint locRes = glGetUniformLocation(shaderProgram, "uResolution");
    if (locRes != -1) {
        glUniform2f(locRes, (float)width, (float)height);
    }

    // ensure the sampler2D "tex" uses texture unit 0
    GLint locTex = glGetUniformLocation(shaderProgram, "tex");
    if (locTex != -1) {
        glUniform1i(locTex, 0);
    }
}

Renderer::ShaderRenderer::~ShaderRenderer() {
    // Delete per-frame/scene vertex objects
    clearVertexObjects();

    // Delete constant objects too (they're used for background/board)
    for (const auto& obj : constantObjs) {
        if (obj.VAO) glDeleteVertexArrays(1, &obj.VAO);
        if (obj.VBO) glDeleteBuffers(1, &obj.VBO);
        if (obj.EBO) glDeleteBuffers(1, &obj.EBO);
        if (obj.textured && obj.textureID) glDeleteTextures(1, &obj.textureID);
    }
    constantObjs.clear();

    // If you have a textureCache, delete those textures now:
    for (auto& kv : textureCache) {
        if (kv.second) glDeleteTextures(1, &kv.second);
    }
    textureCache.clear();

    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}

void Renderer::ShaderRenderer::setupTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    Vertex vertices[] = { v1, v2, v3 };

    GLuint VAO = 0, VBO = 0;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    vertexObjs.push_back({ VAO, VBO, 0, GL_TRIANGLES, 3, false, 0 });

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    // color (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    // texcoord (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::ShaderRenderer::setupQuad(float x, float y, float width, float height, const char* texturePath) {
    float left = x, right = x + width;
    float bottom = y, top = y + height;

    Vertex vertices[] = {
        {{left,  bottom, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // BL
        {{right, bottom, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // BR
        {{right, top,    0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // TR
        {{left,  top,    0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}  // TL
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint VAO = 0, VBO = 0, EBO = 0;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    GLuint texture = 0;
    bool textured = false;

    if (texturePath) {
        texture = loadTexture(texturePath);
        if (texture != 0) textured = true;
        else std::cerr << "Renderer::setupQuad: texture load failed: " << texturePath << std::endl;
    }

    vertexObjs.push_back({ VAO, VBO, EBO, GL_TRIANGLES, 6, textured, texture });

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    // color (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    // texcoord (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::ShaderRenderer::clearVertexObjects() {
    // Delete GPU resources for vertexObjs
    for (const auto& obj : vertexObjs) {
        if (obj.VAO) glDeleteVertexArrays(1, &obj.VAO);
        if (obj.VBO) glDeleteBuffers(1, &obj.VBO);
        if (obj.EBO) glDeleteBuffers(1, &obj.EBO);
        if (obj.textured && obj.textureID) {
            // If you are using a texture cache, do NOT delete textures here;
            // delete cached textures once in destructor. If you don't cache,
            // deleting here is fine. We'll assume you have textureCache:
            // glDeleteTextures(1, &obj.textureID);
        }
    }
    vertexObjs.clear();
}

void Renderer::ShaderRenderer::renderConstants() {
    glUseProgram(shaderProgram);
    for (const auto& obj : constantObjs) {
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), obj.textured ? 1 : 0);
        if (obj.textured) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0); // texture unit 0
        }
        glBindVertexArray(obj.VAO);
        if (obj.EBO)
            glDrawElements(obj.type, obj.vertexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(obj.type, 0, obj.vertexCount);
        glBindVertexArray(0);
    }
}


void Renderer::ShaderRenderer::render() {
    glUseProgram(shaderProgram);
    for (const auto& obj : vertexObjs) {
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), obj.textured ? 1 : 0);

        if (obj.textured) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0); // texture unit 0
        }

        glBindVertexArray(obj.VAO);
        if (obj.EBO)
            glDrawElements(obj.type, obj.vertexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(obj.type, 0, obj.vertexCount);
        glBindVertexArray(0);
    }

}

void Renderer::ShaderRenderer::setupColoredRect(float x, float y, float width, float height, const Vec3& color) {
    // Define four vertices for the rectangle
    Vertex vertices[] = {
        {{x,           y,            0.0f}, color, {0.0f, 0.0f}}, // bottom-left
        {{x + width,   y,            0.0f}, color, {1.0f, 0.0f}}, // bottom-right
        {{x + width,   y + height,   0.0f}, color, {1.0f, 1.0f}}, // top-right
        {{x,           y + height,   0.0f}, color, {0.0f, 1.0f}}  // top-left
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    constantObjs.push_back({ VAO, VBO, EBO, GL_TRIANGLES, 6, false, 0 });

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}


GLuint Renderer::ShaderRenderer::loadTexture(const char* path) {
    if (textureCache.contains(path))
        return textureCache[path];

    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (!data) { std::cerr << "Failed to load texture: " << path << std::endl; return 0; }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    textureCache[path] = texture;
    return texture;

}

// --- Uniform utilities ---
// Note: we ensure the program is active before setting uniforms and check location.
void Renderer::ShaderRenderer::setUniform1f(const std::string& name, float value) {
    glUseProgram(shaderProgram);
    GLint loc = glGetUniformLocation(shaderProgram, name.c_str());
    if (loc == -1) {
        // optional: log once
        // std::cerr << "Uniform '" << name << "' not found.\n";
        return;
    }
    glUniform1f(loc, value);
}

void Renderer::ShaderRenderer::setUniform2f(const std::string& name, float v1, float v2) {
    glUseProgram(shaderProgram);
    GLint loc = glGetUniformLocation(shaderProgram, name.c_str());
    if (loc == -1) return;
    glUniform2f(loc, v1, v2);
}

void Renderer::ShaderRenderer::setUniformMat4(const std::string& name, const glm::mat4& mat) {
    glUseProgram(shaderProgram);
    GLint loc = glGetUniformLocation(shaderProgram, name.c_str());
    if (loc == -1) {
        // std::cerr << "Uniform '" << name << "' not found.\n";
        return;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}


 void Renderer::SetupImGuiStyle()
{
    // Comfortable Dark Cyan style by SouthCraftX from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 1.0f;
    style.WindowPadding = ImVec2(20.0f, 20.0f);
    style.WindowRounding = 11.5f;
    style.WindowBorderSize = 0.0f;
    style.WindowMinSize = ImVec2(20.0f, 20.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ChildRounding = 20.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 17.39999961853027f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(20.0f, 3.400000095367432f);
    style.FrameRounding = 11.89999961853027f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.899999618530273f, 13.39999961853027f);
    style.ItemInnerSpacing = ImVec2(7.099999904632568f, 1.799999952316284f);
    style.CellPadding = ImVec2(12.10000038146973f, 9.199999809265137f);
    style.IndentSpacing = 0.0f;
    style.ColumnsMinSpacing = 8.699999809265137f;
    style.ScrollbarSize = 11.60000038146973f;
    style.ScrollbarRounding = 15.89999961853027f;
    style.GrabMinSize = 3.700000047683716f;
    style.GrabRounding = 20.0f;
    style.TabRounding = 9.800000190734863f;
    style.TabBorderSize = 0.0f;
    style.TabCloseButtonMinWidthSelected = 0.0f;
    style.TabCloseButtonMinWidthUnselected = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09411764889955521f, 0.1019607856869698f, 0.1176470592617989f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1137254908680916f, 0.125490203499794f, 0.1529411822557449f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0313725508749485f, 0.9490196108818054f, 0.843137264251709f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.0313725508749485f, 0.9490196108818054f, 0.843137264251709f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.6000000238418579f, 0.9647058844566345f, 0.0313725508749485f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1803921610116959f, 0.1882352977991104f, 0.196078434586525f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1529411822557449f, 0.1529411822557449f, 0.1529411822557449f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.1411764770746231f, 0.1647058874368668f, 0.2078431397676468f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.105882354080677f, 0.105882354080677f, 0.105882354080677f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.1294117718935013f, 0.1490196138620377f, 0.1921568661928177f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0313725508749485f, 0.9490196108818054f, 0.843137264251709f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.125490203499794f, 0.2745098173618317f, 0.572549045085907f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0313725508749485f, 0.9490196108818054f, 0.843137264251709f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.9372549057006836f, 0.9372549057006836f, 0.9372549057006836f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2666666805744171f, 0.2901960909366608f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
}