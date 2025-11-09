#include <GL/glew.h> // MUST be included before GLFW
#include <GLFW/glfw3.h>
#include "Rendering/Renderer.hpp"
#include "GameFunctions.hpp"
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
Chess::Position draggedFromPos;
Chess::Position draggedToPos;
bool isDragging = false;


inline void drawDragging(int width, int height, double xpos, double ypos) {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            Chess::Piece* piece = chessBoard.getPiece(x, y);
            if (piece->getType() == Chess::PieceType::EMPTY)
                continue;
            if (x == draggedFromPos.x && y == draggedFromPos.y) {

                float aspect = static_cast<float>(width) / static_cast<float>(height);

                float xposWorld = ((xpos / width) * 2.0f - 1.0f) * aspect - tileSize / 2.0f;
                float yposWorld = 1.0f - (ypos / height) * 2.0f - tileSize / 2.0f;


                draggedPieceVertexObject = renderer->setupQuad(
                    xposWorld, yposWorld, tileSize, tileSize, retrievePath(piece).c_str()
                );
            }
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if(chessBoard.gameState == Chess::GameState::PAUSED)
		return;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        Chess::Position worldPos = screenToWorld(xpos, ypos, width, height, tileSize, GRID_SIZE);


        // Check if the position is valid AND there is a piece
        if (worldPos.x >= 0 && worldPos.y >= 0 &&
			chessBoard.getPiece(worldPos.x, worldPos.y)->getType() != Chess::PieceType::EMPTY &&
            chessBoard.currentTurn == chessBoard.getPiece(worldPos.x, worldPos.y)->getColor() && !isDragging)
        {
			std::cout << "Grabbing\n";
			draggedFromPos = worldPos;
            draggedToPos = worldPos;
			isDragging = true;
			chessBoard.getPiece(worldPos.x, worldPos.y)->isVisible = false;
			drawDragging(width, height, xpos, ypos);
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        if (draggedFromPos == draggedToPos)
            return;
	    if (draggedFromPos.x == draggedToPos.x && draggedFromPos.y == draggedToPos.y)
            return;
        isDragging = false;
        std::cout << "Releasing\n";
        chessBoard.getPiece(draggedFromPos.x, draggedFromPos.y)->isVisible = true;
        chessBoard.makeMove(draggedFromPos, draggedToPos, takenPieces);
        draggedPieceVertexObject = renderer->setupQuad(0, 0, 0, 0);
        draggedFromPos = Chess::Position();
		draggedToPos = Chess::Position();

    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (draggedFromPos.x == -1 && draggedFromPos.y == -1)
        return;
    if(!isDragging)
		return;

    if (chessBoard.gameState == Chess::GameState::PAUSED) {
        chessBoard.getPiece(draggedFromPos.x, draggedFromPos.y)->isVisible = true;
        draggedPieceVertexObject = renderer->setupQuad(0, 0, 0, 0);
        draggedFromPos = Chess::Position();
        draggedToPos = Chess::Position();
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    Chess::Position worldPos = screenToWorld(xpos, ypos, width, height, tileSize, GRID_SIZE);
    if (worldPos.x >= 0 && worldPos.y >= 0) {
        drawDragging(width,height,xpos,ypos);
        std::cout << "Dragging\n";
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
            bool isWhite = (x + y) % 2 == 1;
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

            piece->vertexObject = renderer->setupQuad(xpos, ypos, tileSize, tileSize, retrievePath(piece).c_str());
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

        ImVec2 menuSize = ImVec2(width / 3, width / 2);
        ImVec2 center = ImVec2(width, height);
        // Calculate centered position
        ImVec2 pos = ImVec2(
            (center.x - menuSize.x) * 0.5f,
            (center.y - menuSize.y) * 0.5f
        );

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings;

        // Set window position and size before creating it


        
        ImGui::Text(chessBoard.currentTurn == Chess::PieceColor::WHITE ?
            "White's Turn!" : "Black's Turn!");
        if (ImGui::Button("Reset Board")) {
            chessBoard.resetBoard();
            takenPieces.clear();
        }

        ImGui::Separator();
        ImGui::Text("GameState:");
        ImGui::Text(chessBoard.gameState == Chess::GameState::ONGOING ? "Ongoing" :
            chessBoard.gameState == Chess::GameState::CHECK ? "Check" :
            chessBoard.gameState == Chess::GameState::CHECKMATE ? "Checkmate" :
            chessBoard.gameState == Chess::GameState::STALEMATE ? "Stalemate" : "Unknown");

        if (chessBoard.gameState == Chess::GameState::PAUSED) {

            ImGui::PushFont(ImGui::GetFont()); // use larger font if you have one loaded

            Renderer::TextCentered(" Chess Engine "); // you'll need to write your own centering helper
            ImGui::Separator();


            if (ImGui::Button("Start Local Game", ImVec2(200, 50)))
            {
                chessBoard.gameState = Chess::GameState::ONGOING;
            }
            if (ImGui::Button("Host Online Game", ImVec2(200, 50)))
            {

            }
            if (ImGui::Button("Join Online Game", ImVec2(200, 50))) {

            }

            ImGui::PopFont();

        }
        else {
            if(ImGui::Button("Restart Game", ImVec2(200, 50))) {
                chessBoard.resetBoard();
				chessBoard.gameState = Chess::GameState::ONGOING;
				takenPieces.clear();
			}
        }


        if (ImGui::Button("Quit", ImVec2(200, 50))) {
            return 0;
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


        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) {
                float xpos = -1.0f + x * tileSize;
                float ypos = -1.0f + y * tileSize;

                Chess::Piece* piece = chessBoard.getPiece(x, y);
                if (piece->getType() == Chess::PieceType::EMPTY)
                    continue;
                if (!piece->isVisible)
                    continue;

                piece->vertexObject = renderer->setupQuad(xpos, ypos, tileSize, tileSize, retrievePath(piece).c_str());
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

            auto vobj = renderer->setupQuad(x, y, pieceDisplaySize, pieceDisplaySize, retrievePath(p.get()).c_str());
            renderer->render({ vobj });
        }

        if (isDragging)
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