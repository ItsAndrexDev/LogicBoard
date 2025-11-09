#include "Networking.hpp"
#include <iostream>

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