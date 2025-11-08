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
#include <optional>
#include <algorithm>
const int GRID_SIZE = 8;
float tileSize = 2.0f / GRID_SIZE;

Chess::Board chessBoard;

std::vector<Renderer::VertexObject> tileVertexObjects;
std::vector<Renderer::VertexObject> pieceVertexObjects;
std::optional<Chess::Position> draggedFromPos;
std::optional<Chess::Position> draggedToPos;
bool isDragging = false;

Chess::Position screenToWorld(double mouseX, double mouseY, int windowWidth, int windowHeight) {
    // 1. Normalize Mouse Coordinates (0.0 to 1.0)
    // GLFW mouse Y is top-down (0 at top), so we invert for OpenGL's bottom-up (-1 at bottom)
    float xNormalized = (float)mouseX / (float)windowWidth;
    float yNormalized = 1.0f - (float)mouseY / (float)windowHeight;

    // 2. Convert to Normalized Device Coordinates (NDC: -1 to 1)
    float xNDC = xNormalized * 2.0f - 1.0f;
    float yNDC = yNormalized * 2.0f - 1.0f;

    // 3. Convert to World Coordinates based on your ortho projection
    float aspect = (float)windowWidth / (float)windowHeight;

    // Your projection: ortho(-aspect, aspect, -1.0f, 1.0f, ...)
    // X World: x_NDC * aspect
    // Y World: y_NDC * 1.0f
    float xWorld = xNDC * aspect;
    float yWorld = yNDC;

    // 4. Check if the mouse is within the fixed board area [-1.0, 1.0] in world space
    if (xWorld < -1.0f || xWorld > 1.0f || yWorld < -1.0f || yWorld > 1.0f) {
        // Return an invalid position if the cursor is outside the board area
        return { -1, -1 };
    }

    // 5. Map the [-1.0, 1.0] world range to [0, GRID_SIZE] tile indices
    // (xWorld + 1.0f) shifts the range from [-1.0, 1.0] to [0.0, 2.0]
    // Dividing by tileSize (0.25) maps it to [0.0, 8.0]
    int col = static_cast<int>((xWorld + 1.0f) / tileSize);
    int row = static_cast<int>((yWorld + 1.0f) / tileSize);

    // 6. Clamp to ensure 0-7 range
    col = std::clamp(col, 0, GRID_SIZE - 1);
    row = std::clamp(row, 0, GRID_SIZE - 1);

    // Assuming Chess::Position is a struct/class with public x and y members
    return { col, row };
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        Chess::Position worldPos = screenToWorld(xpos, ypos, width, height);


        // Check if the position is valid AND there is a piece
        if (worldPos.x >= 0 && worldPos.y >= 0 &&
            chessBoard.getPiece(worldPos.x, worldPos.y)->getType() != Chess::PieceType::EMPTY)
        {
			std::cout << "Mouse pressed at: " << worldPos.x << ", " << worldPos.y << "\n";
			draggedFromPos = worldPos;
			isDragging = true;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		std::cout << "Mouse released\n";
		isDragging = false;
        if (draggedFromPos.has_value()) 
            if (draggedToPos.has_value()) 
				chessBoard.makeMove(*draggedFromPos, *draggedToPos);
            else {
                draggedFromPos.reset();
                draggedToPos.reset();
            }
        
        else {
            draggedFromPos.reset();
            draggedToPos.reset();
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!draggedFromPos.has_value())
        return;
    if(!isDragging)
		return;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    Chess::Position worldPos = screenToWorld(xpos, ypos, width, height);
    if (worldPos.x >= 0 && worldPos.y >= 0) {
        draggedToPos = worldPos;
    }
    
}



int main() {
    // --- GLFW init ---
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Logicboard", monitor, nullptr);
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

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

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
    

    // --- Load shader ---
    Renderer::ShaderRenderer renderer(
        "Rendering/Shaders/vertex.glsl",
        "Rendering/Shaders/fragment.glsl",
        window
    );

    

    // --- Setup board once ---
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            bool isWhite = (x + y) % 2 == 0;
            float xpos = -1.0f + x * tileSize;
            float ypos = -1.0f + y * tileSize;
            
            Vec3 color = isWhite
                ? Vec3(235.0f / 255.0f, 236.0f / 255.0f, 208.0f / 255.0f)
                : Vec3(115.0f / 255.0f, 149.0f / 255.0f, 82.0f / 255.0f);

            tileVertexObjects.push_back(renderer.setupColoredRect(xpos, ypos, tileSize, tileSize, color));

            Chess::Piece* piece = chessBoard.getPiece(x, y);
            if (piece->getType() == Chess::PieceType::EMPTY)
                continue;

            std::string texturePath = "Rendering/Assets/";
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

            pieceVertexObjects.push_back(renderer.setupQuad(xpos, ypos, tileSize, tileSize, texturePath.c_str()));
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

        
        // Make the window always appear at the top-left corner
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

        // Make it full height and fixed width (e.g., 250 pixels)
        ImGui::SetNextWindowSize(ImVec2(250, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

        // Begin the window with flags to lock it in place
        ImGui::Begin("Left Panel", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar); // optional: remove title bar

        // Your panel content
        ImGui::Text(chessBoard.currentTurn == Chess::PieceColor::WHITE ?
            "White's Turn!" : "Black's Turn!");
        ImGui::End();


        // --- Shader updates ---
        renderer.setUniform1f("uTime", (float)glfwGetTime());
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
        glm::mat4 model = glm::mat4(1.0f);
		pieceVertexObjects.clear();
        for(int y = 0; y < GRID_SIZE; y++) {
            for(int x = 0; x < GRID_SIZE; x++) {
                bool isWhite = (x + y) % 2 == 0;
                float xpos = -1.0f + x * tileSize;
                float ypos = -1.0f + y * tileSize;

                Chess::Piece* piece = chessBoard.getPiece(x, y);
                if (piece->getType() == Chess::PieceType::EMPTY)
                    continue;

                std::string texturePath = "Rendering/Assets/";
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

                pieceVertexObjects.push_back(renderer.setupQuad(xpos, ypos, tileSize, tileSize, texturePath.c_str()));
            }
		}
        renderer.setUniformMat4("model", model);
        renderer.setUniformMat4("projection", projection);
        renderer.render(tileVertexObjects);
		renderer.render(pieceVertexObjects);

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
