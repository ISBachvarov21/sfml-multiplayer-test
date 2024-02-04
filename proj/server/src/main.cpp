#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <stdio.h>
#include <iostream>

static sf::UdpSocket socket;

static void recvClients() {
    char* buffer = new char[4098];
    memset(buffer, 0, 4098);

    sf::IpAddress connAddr;
    size_t received;
    unsigned short port;
    
    while (true) {
        socket.receive(buffer, sizeof(buffer), received, connAddr, port);
        std::cout << "\033[1;34m[*] Received message: " << buffer << "\033[0m\n";
    }
}

void bindSock() {
    socket.bind(55001, sf::IpAddress::getLocalAddress());

    recvClients();
}

int main(int argc, char* argv[])
{
    bindSock();

    return 0;
}