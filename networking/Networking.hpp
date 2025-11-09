#pragma once
#include "asio/asio.hpp"
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <cstring>
#include <chrono>
std::string discoverServer(asio::io_context& ioContext, unsigned short port, int timeoutMs = 1000);
namespace Networking {
    class UDPDiscoveryServer {
    public:
        UDPDiscoveryServer(asio::io_context& ioContext, unsigned short port)
            : socket_(ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)) {
            startReceive();
        }

    private:
        void startReceive();

        asio::ip::udp::socket socket_;
        asio::ip::udp::endpoint remoteEndpoint_;
        std::array<char, 1024> recvBuffer_;
    };
	class NetworkManager {
	public:
		NetworkManager();
		~NetworkManager();
		void startServer(unsigned short port);
		void startClient(const std::string& host, unsigned short port);
        template<typename T>
        void sendData(const T& obj) {
            if (!socket_ || !socket_->is_open()) {
                std::cerr << "[Send Error] Socket not open.\n";
                return;
            }

            const char* rawData = reinterpret_cast<const char*>(&obj);
            std::vector<char> data(rawData, rawData + sizeof(T));

            try {
                uint32_t size = static_cast<uint32_t>(data.size());
                asio::write(*socket_, asio::buffer(&size, sizeof(size)));
                asio::write(*socket_, asio::buffer(data.data(), data.size()));
            }
            catch (const std::exception& e) {
                std::cerr << "[Send Error] " << e.what() << "\n";
            }
        }

        template<typename T>
        T receiveData() {
            if (!socket_ || !socket_->is_open()) {
                throw std::runtime_error("Socket not open");
            }

            uint32_t size = 0;
            asio::read(*socket_, asio::buffer(&size, sizeof(size)));

            std::vector<char> buffer(size);
            asio::read(*socket_, asio::buffer(buffer.data(), buffer.size()));

            if (size != sizeof(T)) {
                throw std::runtime_error("Received data size mismatch");
            }

            T obj;
            std::memcpy(&obj, buffer.data(), sizeof(T));
            return obj;
        }
	private:
		asio::io_context ioContext_;
		std::unique_ptr<asio::ip::tcp::socket> socket_;
		std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
	};
} // namespace Networking
