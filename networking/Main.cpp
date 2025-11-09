#include "Networking.hpp"
#include <iostream>
#include <thread>
#include <atomic>

struct Board {
    int x, y;
};

int main() {
    Networking::NetworkManager net;
    Board board;
    char choice;

    std::cout << "Host (h) or Join (j) a game? ";
    std::cin >> choice;

    if (choice == 'h') {
        net.startServer(4275);
    }
    else {
        net.startClient("localhost", 4275);
    }

    std::atomic<bool> running(true);

    // Thread for receiving data
    std::thread receiver([&]() {
        while (running) {
            try {
                Board data = net.receiveData<Board>();
                std::cout << "\n[Received board position]: " << data.x << " " << data.y << "\n> ";
                std::cout.flush();
            }
            catch (...) {}
        }
        });

    // Main thread for input and sending
    while (running) {
        std::cout << "> Enter board position (x y): ";
        if (!(std::cin >> board.x >> board.y)) {
            running = false; // exit if input fails
            break;
        }
        net.sendData(board);
    }

    running = false;
    receiver.join();
}
