#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <stdio.h>
#include <iostream>

static sf::UdpSocket socket;

void sendServer(const char* message) {
    socket.send(message, sizeof(message), sf::IpAddress::getLocalAddress(), 55001);
}

void bindSock(uint_fast16_t port) {
    socket.bind(port, sf::IpAddress::getLocalAddress());

    sendServer("connect");
}

int main(int argc, char* argv[])
{
    uint_fast16_t port = -1;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0) {
            std::cout << "Found --port" << std::endl;
            if (i + 1 < argc) {
                port = (uint_fast16_t)std::stoi(argv[i + 1]);
                break;
            }
            else {
                errno = 22;
                perror("\033[1;31m[-] Invalid --port usage.\n[-] Usage: client --port <port>\033[0m\n");
                abort();
            }
        }
    }

    bindSock(port);

    return 0;
}