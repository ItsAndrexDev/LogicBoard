#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
struct Vec2 {
    float x, y;
    Vec2(float a = 0, float b = 0)
        : x(a), y(b){
    }
};
struct Vec3 {
    union {
        struct { float x, y, z; };
        struct { float r, g, b; };
    };
    Vec3(float a = 0, float b = 0, float c = 0)
        : x(a), y(b), z(c) {
    }
};
struct Vec4 {
    union {
        struct { float x, y, z, w; };
        struct { float r, g, b, a; };
    };
    Vec4(float a = 0, float b = 0, float c = 0, float d = 0)
        : x(a), y(b), z(c), w(d) {
    }
};

struct Vertex {
    Vec3 position;
    Vec3 color;
    Vec2 texCoord;
};

namespace Renderer {
    void SetupImGuiStyle();

    struct VertexObject {
        GLuint VAO, VBO, EBO;
        GLenum type;
        int vertexCount;
        bool textured;
        GLuint textureID;
    };

    GLuint compileShader(const char* path, GLenum type);
    GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath);

    struct ShaderRenderer {
        ShaderRenderer(const char* vertexPath, const char* fragmentPath, GLFWwindow* window);
        ~ShaderRenderer();

        void setupTriangle(const Vertex& v1, const Vertex& v2, const Vertex& v3);
        void setupQuad(float x, float y, float width, float height, const char* texturePath = nullptr);
        void setupColoredRect(float x, float y, float width, float height, const Vec3& color);

        void render();

        // Uniform utilities
        void setUniform1f(const std::string& name, float value);
        void setUniform2f(const std::string& name, float v1, float v2);
        void setUniformMat4(const std::string& name, const glm::mat4& mat);

        GLuint shaderProgram; // kept public since you're accessing it in main
        std::vector<VertexObject> vertexObjs;
    private:
        GLFWwindow* window;

        GLuint loadTexture(const char* path);
    };

}