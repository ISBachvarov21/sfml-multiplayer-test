#include <SFML/Network.hpp>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include "../../json/json.hpp"
#include <thread>
#include <mutex>

#define MAX_CLIENTS 2

using json = nlohmann::json;

static sf::UdpSocket clientSocket;
static sf::UdpSocket gameSocket;
static json gameState;
static json clients;
static uint_least8_t connectedClients = 0;
static std::mutex mut;

void recvClients() {
    char buffer[4098];

    sf::IpAddress connAddr;
    size_t received;
    unsigned short port;
    std::string data;
    
    while (true) {
        memset(buffer, 0, 4098);
        clientSocket.receive(buffer, 4098, received, connAddr, port);
        data = std::string(buffer);
        //std::cout << "\033[1;34m[*] Received message: " << data << "\033[0m\n";

        json response;
        json dataJson;

        try {
            dataJson = json::parse(data);
        }
        catch (json::exception& e) {
            std::cout << "\033[1;31m[-] Error while parsing:\n" << e.what() << "\033[0m\n";
        }

        std::string type;
        try {
            type = dataJson["type"];
        }
        catch (json::exception& e) {
            std::cout << "\033[1;31m[-] Error while getting type:\n" << e.what() << "\033[0m\n";
            abort();
        }
        if (type == "connect") {
            if (connectedClients < MAX_CLIENTS) {
                response["type"] = "status";
                response["data"] = "connected";
                std::string username;
                
                try {
                    username = dataJson["username"];
                }
                catch (json::exception& e) {
                    std::cout << "\033[1;31m[-] Error while parsing username:\n" << e.what() << "\033[0m\n";
                }

                clients[username]["ip"] = connAddr.toString();
                clients[username]["port"] = port;
                
                mut.lock();
                gameState["players"][username]["posY"] = 0.25f;

                bool hasLeft = false;
                if (gameState["players"].size() == 0) {
                    gameState["players"][username]["alignment"] = "left";
                }
                else {
                    for (auto& [key, value] : gameState["players"].items()) {
                        if (value["alignment"] == "left") {
                            hasLeft = true;
                        }
                    }
                    if (hasLeft) {
                        gameState["players"][username]["alignment"] = "right";
                    }
                    else {
                        gameState["players"][username]["alignment"] = "left";
                    }
                }
                mut.unlock();
                
                connectedClients++;
                for (int i = 0; i < 5; ++i) {
                    if (clientSocket.send(response.dump().data(), response.dump().size(), connAddr, port) == sf::Socket::Done) {
                        break;
                    }
                }
            }
            else {
                response["type"] = "error";
                response["data"] = "too many clients";
                
                for (int i = 0; i < 5; ++i) {
                    if (clientSocket.send(response.dump().data(), response.dump().size(), connAddr, port) == sf::Socket::Done) {
                        break;
                    }
                }
            }
        }
        else if (type == "leave") {
            std::string username;

            try {
                username = dataJson["username"];
            }
            catch (json::exception& e) {
                std::cout << "\033[1;31m[-] Error while parsing username:\n" << e.what() << "\033[0m\n";
            }
            
            if (clients.contains(username)) {
                clients.erase(username);

                mut.lock();
                gameState["players"].erase(username);
                mut.unlock();

                connectedClients--;
            }
        }
        else if (type == "move") {
            float y;
            std::string username;

            try {
                username = dataJson["username"];
                y = dataJson["data"];
            }
            catch (json::exception& e) {
                std::cout << "\033[1;31m[-] Error while parsing move data:\n" << e.what() << "\033[0m\n";
            }

            mut.lock();
            gameState["players"][username]["posY"] = y;
            mut.unlock();
            //for (auto& [key, value] : clients.items()) {
            //    std::string ip = value["ip"];
            //    unsigned short port = value["port"];
            //    mut.lock();
            //    gameSocket.send(gameState.dump(4).data(), gameState.dump(4).size(), sf::IpAddress(ip), port);
            //    mut.unlock();
            //}
        }
    }
}

void bindSock() {
    std::cout << "\033[1;34m[*] Local address: " << sf::IpAddress::getLocalAddress() << "\033[0m\n";
    clientSocket.bind(55001, sf::IpAddress::getLocalAddress());
    gameSocket.bind(55002, sf::IpAddress::getLocalAddress());
}

int main(int argc, char* argv[])
{
    bindSock();

    bool shouldRun = true;
    
    sf::Vector2f simulationSize;
    simulationSize.x = 1280;
    simulationSize.y = 1080;

    sf::Vector2f ballPos;
    ballPos.x = 640;
    ballPos.y = 360;
    
    sf::Vector2i ballDir;
    ballDir.x = 1;
    ballDir.y = 1;

    sf::Vector2f player1Pos;
    player1Pos.x = 0;
    player1Pos.y = 180;
    
    sf::Vector2f player2Pos;
    player2Pos.x = 1260;
    player2Pos.y = 180;

    gameState["ball"]["posX"] = 0.5;
    gameState["ball"]["posY"] = 0.5;
    
    std::jthread receiveThread(recvClients);

    sf::Clock clock;
    float dt;
    while (shouldRun) {
        if (clock.getElapsedTime().asSeconds() < 0.016) {
            continue;
        }

        if (connectedClients < 2) {
            clock.restart();
            continue;
        }

        dt = clock.getElapsedTime().asSeconds();


        ballPos.x += (float)ballDir.x * 400.f * dt;
        ballPos.y += (float)ballDir.y * 400.f * dt;

        mut.lock();
        for (auto& [key, value] : gameState["players"].items()) {
            float posY = value["posY"];
            if (value["alignment"] == "left") {
                player1Pos.y = posY * simulationSize.y;
            }
            else {
                player2Pos.y = posY * simulationSize.y;
            }
        }
        mut.unlock();

        if (ballPos.x > 0 && ballPos.x <= player1Pos.x + 20 && ballPos.y + 16 >= player1Pos.y && ballPos.y <= player1Pos.y + 260 && ballDir.x == -1) {
            ballDir.x *= -1;
        }
        else if (ballPos.x + 8 < 1280 && ballPos.x + 8 >= player2Pos.x && ballPos.y + 16 >= player2Pos.y && ballPos.y <= player2Pos.y + 260 && ballDir.x == 1) {
            ballDir.x *= -1;
        }
        if ((ballPos.x <= 0 && ballDir.x == -1) || (ballPos.x + 8 >= simulationSize.x && ballDir.x == 1)) {
            ballPos = sf::Vector2f(640 - 16, 360 - 16);
            ballDir *= -1;
        }
        if ((ballPos.y <= 0 && ballDir.y == -1) || (ballPos.y + 16 >= simulationSize.y && ballDir.y == 1)) {
            ballDir.y *= -1;
        }

        mut.lock();
        gameState["ball"]["posX"] = ballPos.x / simulationSize.x;
        gameState["ball"]["posY"] = ballPos.y / simulationSize.y;
        mut.unlock();

        for (auto& [key, value] : clients.items()) {
            std::string ip = value["ip"];
            unsigned short port = value["port"];
            mut.lock();
            gameSocket.send(gameState.dump(4).data(), gameState.dump(4).size(), sf::IpAddress(ip), port);
            mut.unlock();
        }

        clock.restart();
    }

    clientSocket.unbind();
    gameSocket.unbind();

    return 0;
}