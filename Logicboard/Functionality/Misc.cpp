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



bool AllowPortThroughFirewall(int port, const std::wstring& ruleName) {
    wchar_t command[256];
    swprintf_s(command,
        L"netsh advfirewall firewall add rule name=\"%s\" dir=in action=allow protocol=TCP localport=%d",
        ruleName.c_str(), port);
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas"; // Run as admin
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = (std::wstring(L"/c ") + command).c_str();
    sei.nShow = SW_HIDE; // or SW_SHOW if you want to see it
    sei.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;

    if (!ShellExecuteExW(&sei)) {
        std::wcerr << L"Failed to request admin privileges or execute command.\n";
        return false;
    }

	Sleep(3000); // Wait a bit for the command to take effect
    DWORD exitCode = 1;
    GetExitCodeProcess(sei.hProcess, &exitCode);
    CloseHandle(sei.hProcess);
    if (exitCode == 0) {
        std::wcout << L"Firewall rule added successfully.\n";
        return true;
    }
    else {
        std::wcerr << L"Failed to add firewall rule (exit code " << exitCode << L").\n";
        return false;
    }
}

std::string Networking::discoverServer(asio::io_context& ioContext, unsigned short port, int timeoutMs) {
    asio::ip::udp::socket socket(ioContext);
    socket.open(asio::ip::udp::v4());

    // Enable broadcast
    socket.set_option(asio::socket_base::broadcast(true));

    asio::ip::udp::endpoint broadcastEndpoint(asio::ip::address_v4::broadcast(), port);

    // Send discovery message
    std::string discoverMsg = "DISCOVER_CHESS_SERVER";
    socket.send_to(asio::buffer(discoverMsg), broadcastEndpoint);

    // Wait for reply
    socket.non_blocking(true);
    auto start = std::chrono::steady_clock::now();
    asio::ip::udp::endpoint serverEndpoint;

    std::array<char, 1024> recvBuffer;
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(timeoutMs)) {
        std::error_code ec;
        std::size_t len = socket.receive_from(asio::buffer(recvBuffer), serverEndpoint, 0, ec);
        if (!ec && len > 0) {
            std::string reply(recvBuffer.data(), len);
            if (reply == "CHESS_SERVER_HERE") {
                return serverEndpoint.address().to_string();
            }
        }
    }

    return ""; // not found
}

void Networking::UDPDiscoveryServer::startReceive() {
    socket_.async_receive_from(
        asio::buffer(recvBuffer_), remoteEndpoint_,
        [this](std::error_code ec, std::size_t bytesRecv) {
            if (!ec) {
                std::string msg(recvBuffer_.data(), bytesRecv);
                if (msg == "DISCOVER_CHESS_SERVER") {
                    std::string reply = "CHESS_SERVER_HERE";
                    socket_.send_to(asio::buffer(reply), remoteEndpoint_);
                }
            }
            startReceive();
        });
}