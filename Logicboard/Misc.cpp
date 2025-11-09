#include "Misc.hpp"

std::string retrievePath(Chess::Piece* piece) {
    std::string texturePath = "Rendering/Assets/";
    texturePath += (piece->getColor() == Chess::PieceColor::WHITE ? "w" : "b");
    switch (piece->getType()) {
    case Chess::PieceType::PAWN:   texturePath += "p.png";   break;
    case Chess::PieceType::ROOK:   texturePath += "r.png";   break;
    case Chess::PieceType::KNIGHT: texturePath += "n.png"; break;
    case Chess::PieceType::BISHOP: texturePath += "b.png"; break;
    case Chess::PieceType::QUEEN:  texturePath += "q.png";  break;
    case Chess::PieceType::KING:   texturePath += "k.png";   break;
    }
    return texturePath;
}


Chess::Position screenToWorld(double mouseX, double mouseY, int windowWidth, int windowHeight, float tileSize, int GRID_SIZE) {
    float xNormalized = (float)mouseX / (float)windowWidth;
    float yNormalized = 1.0f - (float)mouseY / (float)windowHeight;

    float xNDC = xNormalized * 2.0f - 1.0f;
    float yNDC = yNormalized * 2.0f - 1.0f;

    float aspect = (float)windowWidth / (float)windowHeight;

    float xWorld = xNDC * aspect;
    float yWorld = yNDC;

    if (xWorld < -1.0f || xWorld > 1.0f || yWorld < -1.0f || yWorld > 1.0f)
        return { -1, -1 };

    int col = static_cast<int>((xWorld + 1.0f) / tileSize);
    int row = static_cast<int>((yWorld + 1.0f) / tileSize);

    col = std::clamp(col, 0, GRID_SIZE - 1);
    row = std::clamp(row, 0, GRID_SIZE - 1);

    return { col, row };
}

namespace Networking {

    using asio::ip::tcp;

    NetworkManager::NetworkManager()
        : socket_(std::make_unique<tcp::socket>(ioContext_)) {
    }

    NetworkManager::~NetworkManager() {
        if (socket_ && socket_->is_open()) {
            asio::error_code ec;
            socket_->close(ec);
        }
    }

    void NetworkManager::startServer(unsigned short port) {
        try {
            acceptor_ = std::make_unique<tcp::acceptor>(ioContext_, tcp::endpoint(tcp::v4(), port));
            std::cout << "[Server] Waiting for client on port " << port << "...\n";
            acceptor_->accept(*socket_);
            std::cout << "[Server] Client connected!\n";
        }
        catch (std::exception& e) {
            std::cerr << "[Server Error] " << e.what() << "\n";
        }
    }

    void NetworkManager::startClient(const std::string& host, unsigned short port) {
        try {
            tcp::resolver resolver(ioContext_);
            auto endpoints = resolver.resolve(host, std::to_string(port));
            asio::connect(*socket_, endpoints);
            std::cout << "[Client] Connected to server " << host << ":" << port << "\n";
        }
        catch (std::exception& e) {
            std::cerr << "[Client Error] " << e.what() << "\n";
        }
    }


} // namespace Networking