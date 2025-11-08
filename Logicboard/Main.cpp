#include <GL/glew.h> // MUST be included before GLFW
#include <GLFW/glfw3.h>
#include "Rendering/Renderer.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Chess.hpp"
#include <string>
#include "Rendering/imgui/imgui.h"
#include "Rendering/imgui/imgui_impl_glfw.h"
#include "Rendering/imgui/imgui_impl_opengl3.h"

int main() {
    // --- GLFW init ---
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 800, "Logicboard", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // --- GLEW init ---
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- ImGui setup ---
    IMGUI_CHECKVERSION();
    ImGuiContext* ctx = ImGui::CreateContext();
    if (!ctx) {
        std::cerr << "ImGui context creation failed!\n";
        return -1;
    }
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "ImGui GLFW init failed!\n";
        return -1;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "ImGui OpenGL3 init failed!\n";
        return -1;
    }
	glfwSwapInterval(1); // Enable vsync
    Renderer::SetupImGuiStyle();
    Chess::Board chessBoard;

    // --- Load shader ---
    Renderer::ShaderRenderer renderer(
        "Rendering/Shaders/vertex.glsl",
        "Rendering/Shaders/fragment.glsl",
        window
    );

    const int GRID_SIZE = 8;
    float tileSize = 2.0f / GRID_SIZE;

    // --- Setup board once ---
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            bool isWhite = (x + y) % 2 == 0;
            float xpos = -1.0f + x * tileSize;
            float ypos = -1.0f + y * tileSize;
            Vec3 color = isWhite
                ? Vec3(235.0f / 255.0f, 236.0f / 255.0f, 208.0f / 255.0f)
                : Vec3(115.0f / 255.0f, 149.0f / 255.0f, 82.0f / 255.0f);

            renderer.setupColoredRect(xpos, ypos, tileSize, tileSize, color);

            Chess::Piece* piece = chessBoard.getPiece(x, y);
            if (piece->getType() == Chess::PieceType::EMPTY)
                continue;

            std::string texturePath = "Rendering/Textures/";
            texturePath += (piece->getColor() == Chess::PieceColor::WHITE ? "w_" : "b_");

            switch (piece->getType()) {
            case Chess::PieceType::PAWN:   texturePath += "Pawn.png";   break;
            case Chess::PieceType::ROOK:   texturePath += "Rook.png";   break;
            case Chess::PieceType::KNIGHT: texturePath += "Knight.png"; break;
            case Chess::PieceType::BISHOP: texturePath += "Bishop.png"; break;
            case Chess::PieceType::QUEEN:  texturePath += "Queen.png";  break;
            case Chess::PieceType::KING:   texturePath += "King.png";   break;
            default: continue;
            }

            renderer.setupQuad(xpos, ypos, tileSize, tileSize, texturePath.c_str());
        }
    }
    // --- Main loop ---
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    while (!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- ImGui frame ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Logicboard");
        ImGui::Text("Logicboard Chess Engine");

        if (ImGui::Button("Reset Board")) {
            chessBoard.resetBoard();
        }
        if (ImGui::Button("Move")) {
            chessBoard.makeMove({ 1, 0 }, { 4, 4 });
        }
        ImGui::End();

        // --- Shader updates ---
        renderer.setUniform1f("uTime", (float)glfwGetTime());
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
        glm::mat4 model = glm::mat4(1.0f);
		renderer.clearVertexObjects();
        for(int y = 0; y < GRID_SIZE; y++) {
            for(int x = 0; x < GRID_SIZE; x++) {
                bool isWhite = (x + y) % 2 == 0;
                float xpos = -1.0f + x * tileSize;
                float ypos = -1.0f + y * tileSize;

                Chess::Piece* piece = chessBoard.getPiece(x, y);
                if (piece->getType() == Chess::PieceType::EMPTY)
                    continue;

                std::string texturePath = "Rendering/Textures/";
                texturePath += (piece->getColor() == Chess::PieceColor::WHITE ? "w_" : "b_");

                switch (piece->getType()) {
                case Chess::PieceType::PAWN:   texturePath += "Pawn.png";   break;
                case Chess::PieceType::ROOK:   texturePath += "Rook.png";   break;
                case Chess::PieceType::KNIGHT: texturePath += "Knight.png"; break;
                case Chess::PieceType::BISHOP: texturePath += "Bishop.png"; break;
                case Chess::PieceType::QUEEN:  texturePath += "Queen.png";  break;
                case Chess::PieceType::KING:   texturePath += "King.png";   break;
                default: continue;
                }

                renderer.setupQuad(xpos, ypos, tileSize, tileSize, texturePath.c_str());
            }
		}
		std::cout << "Pieces: " << renderer.vertexObjs.size() << std::endl;
        renderer.setUniformMat4("model", model);
        renderer.setUniformMat4("projection", projection);
        renderer.renderConstants();
        renderer.render();

        // --- Render ImGui ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Cleanup ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}
