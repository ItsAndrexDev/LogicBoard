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


std::unique_ptr<Renderer::ShaderRenderer> renderer;



Chess::Board chessBoard;

std::vector<Renderer::VertexObject> tileVertexObjects;
std::vector<std::unique_ptr<Chess::Piece>> takenPieces;
Renderer::VertexObject draggedPieceVertexObject;
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
			chessBoard.getPiece(worldPos.x, worldPos.y)->getType() != Chess::PieceType::EMPTY &&
            chessBoard.currentTurn == chessBoard.getPiece(worldPos.x, worldPos.y)->getColor())
        {
			draggedFromPos = worldPos;
			draggedToPos = worldPos;
			isDragging = true;
			chessBoard.getPiece(worldPos.x, worldPos.y)->isVisible = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        isDragging = false;
        if (draggedFromPos.has_value())
            if (draggedToPos.has_value()) {
				if (draggedFromPos->x == draggedToPos->x && draggedFromPos->y == draggedToPos->y) return;
                chessBoard.getPiece(draggedFromPos->x, draggedFromPos->y)->isVisible = true;
                chessBoard.makeMove(*draggedFromPos, *draggedToPos, takenPieces);
                draggedPieceVertexObject = renderer->setupQuad(0, 0, 0, 0);

            }
        draggedFromPos.reset();
        draggedToPos.reset();
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
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                Chess::Piece* piece = chessBoard.getPiece(x, y);
                if (piece->getType() == Chess::PieceType::EMPTY)
                    continue;
                if (x == draggedFromPos->x && y == draggedFromPos->y) {
                    std::string texturePath = "Rendering/Assets/";
                    texturePath += (piece->getColor() == Chess::PieceColor::WHITE ? "w" : "b");
                    switch (piece->getType()) {
                    case Chess::PieceType::PAWN:   texturePath += "p.png";   break;
                    case Chess::PieceType::ROOK:   texturePath += "r.png";   break;
                    case Chess::PieceType::KNIGHT: texturePath += "n.png"; break;
                    case Chess::PieceType::BISHOP: texturePath += "b.png"; break;
                    case Chess::PieceType::QUEEN:  texturePath += "q.png";  break;
                    case Chess::PieceType::KING:   texturePath += "k.png";   break;
                    default: continue;
                    }
                    float aspect = static_cast<float>(width) / static_cast<float>(height);

                    float xposWorld = ((xpos / width) * 2.0f - 1.0f) * aspect - tileSize / 2.0f;
                    float yposWorld = 1.0f - (ypos / height) * 2.0f - tileSize / 2.0f;
                    
                    draggedPieceVertexObject = renderer->setupQuad(
                        xposWorld, yposWorld, tileSize, tileSize, texturePath.c_str()
                    );
                }
            }
        }
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
    

    renderer = std::make_unique<Renderer::ShaderRenderer>(
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

            tileVertexObjects.push_back(renderer->setupColoredRect(xpos, ypos, tileSize, tileSize, color));

            Chess::Piece* piece = chessBoard.getPiece(x, y);
            if (piece->getType() == Chess::PieceType::EMPTY)
                continue;
            if (!piece->isVisible)
                continue;
            std::string texturePath = "Rendering/Assets/";
            texturePath += (piece->getColor() == Chess::PieceColor::WHITE ? "w" : "b");

            switch (piece->getType()) {
            case Chess::PieceType::PAWN:   texturePath += "p.png";   break;
            case Chess::PieceType::ROOK:   texturePath += "r.png";   break;
            case Chess::PieceType::KNIGHT: texturePath += "n.png"; break;
            case Chess::PieceType::BISHOP: texturePath += "b.png"; break;
            case Chess::PieceType::QUEEN:  texturePath += "q.png";  break;
            case Chess::PieceType::KING:   texturePath += "k.png";   break;
            default: continue;
            }

            piece->vertexObject = renderer->setupQuad(xpos, ypos, tileSize, tileSize, texturePath.c_str());
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
        if(ImGui::Button("Reset Board")) {
            chessBoard.resetBoard();
			takenPieces.clear();
		}
        ImGui::End();


        // --- Shader updates ---
        renderer->setUniform1f("uTime", (float)glfwGetTime());
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
        glm::mat4 model = glm::mat4(1.0f);
        renderer->setUniformMat4("model", model);
        renderer->setUniformMat4("projection", projection);
        renderer->render(tileVertexObjects);


        for(int y = 0; y < GRID_SIZE; y++) {
            for(int x = 0; x < GRID_SIZE; x++) {
                bool isWhite = (x + y) % 2 == 0;
                float xpos = -1.0f + x * tileSize;
                float ypos = -1.0f + y * tileSize;

                Chess::Piece* piece = chessBoard.getPiece(x, y);
                if (piece->getType() == Chess::PieceType::EMPTY)
                    continue;
				if (!piece->isVisible)
					continue;
                std::string texturePath = "Rendering/Assets/";
                texturePath += (piece->getColor() == Chess::PieceColor::WHITE ? "w" : "b");

                switch (piece->getType()) {
                case Chess::PieceType::PAWN:   texturePath += "p.png";   break;
                case Chess::PieceType::ROOK:   texturePath += "r.png";   break;
                case Chess::PieceType::KNIGHT: texturePath += "n.png"; break;
                case Chess::PieceType::BISHOP: texturePath += "b.png"; break;
                case Chess::PieceType::QUEEN:  texturePath += "q.png";  break;
                case Chess::PieceType::KING:   texturePath += "k.png";   break;
                default: continue;
                }

                piece->vertexObject = renderer->setupQuad(xpos, ypos, tileSize, tileSize, texturePath.c_str());
				renderer->render({ piece->vertexObject });
            }
		}
        
        // --- Render taken pieces (right side) ---
        float pieceDisplaySize = tileSize * 0.5f;
        float margin = tileSize * 0.8f;
        float startX = aspect - pieceDisplaySize - margin + 0.2f; // rightmost column
        float whiteStartY = 1.0f - margin - pieceDisplaySize + 0.2f;  // start near top
        float blackStartY = -1.0f + margin + pieceDisplaySize - 0.2f; // start near bottom
        int maxRows = 5; // pieces per column

        int whiteIndex = 0;
        int blackIndex = 0;

        for (const auto& p : takenPieces) {
            std::string texturePath = "Rendering/Assets/";
            texturePath += (p->getColor() == Chess::PieceColor::WHITE ? "w" : "b");

            switch (p->getType()) {
            case Chess::PieceType::PAWN:   texturePath += "p.png";   break;
            case Chess::PieceType::ROOK:   texturePath += "r.png";   break;
            case Chess::PieceType::KNIGHT: texturePath += "n.png"; break;
            case Chess::PieceType::BISHOP: texturePath += "b.png"; break;
            case Chess::PieceType::QUEEN:  texturePath += "q.png";  break;
            case Chess::PieceType::KING:   texturePath += "k.png";   break;
            default: continue;
            }

            float x, y;

            if (p->getColor() == Chess::PieceColor::WHITE) {
                // column and row logic for white pieces
                int col = whiteIndex / maxRows;
                int row = whiteIndex % maxRows;

                x = startX - col * pieceDisplaySize * 1.2f;
                y = whiteStartY - row * pieceDisplaySize * 1.1f;
                whiteIndex++;
            }
            else {
                // column and row logic for black pieces
                int col = blackIndex / maxRows;
                int row = blackIndex % maxRows;

                x = startX - col * pieceDisplaySize * 1.2f;
                y = blackStartY + row * pieceDisplaySize * 1.1f;
                blackIndex++;
            }

            auto vobj = renderer->setupQuad(x, y, pieceDisplaySize, pieceDisplaySize, texturePath.c_str());
            renderer->render({ vobj });
        }

        if(isDragging)
			renderer->render({ draggedPieceVertexObject });

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
