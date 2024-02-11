#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <stdio.h>
#include <iostream>
#include <string>
#include "../../json/json.hpp"

#define BUFFERSIZE 12000

using json = nlohmann::json;

static sf::UdpSocket socket;
static std::string addr = "";
static std::string username = "";

void sendServer(const char* message, int retries = 0) {
    //std::cout << "\033[1;34m[*]Sending message:\n" << message << "\033[0m\n";
    for (int i = 0; i < retries + 1; ++i) {
        auto res = socket.send(message, std::strlen(message), sf::IpAddress(addr), 55001);
        if (res == sf::Socket::Status::Done) {
            break;
        }
    }
}

bool connectToServer(const char* message) {
    sendServer(message, 5);

    //std::cout << "\033[1;34m[*] Sent connect message\033[0m\n";
    char buffer[4098];
    memset(buffer, 0, 4098);
    
    size_t received;
    sf::IpAddress respAddr;
    unsigned short port;

    auto res = socket.receive(buffer, 4098, received, respAddr, port);
    if (res == sf::Socket::Done) {
        std::string data(buffer);

        json response = json::parse(data);

        if (response["type"] == "status") {
            std::cout << "\033[1;32m[+] Connected to server successfully\033[0m\n";
            return true;
        }
        else if (response["type"] == "error") {
            std::cout << "\033[1;31m[-] Server already full\033[0m\n";
        }
    }

    return false;
}

void bindSock() {
    socket.bind(sf::Socket::AnyPort, sf::IpAddress::getLocalAddress());
    json data;

    data["type"] = "connect";
    data["username"] = username;
    if (!connectToServer(data.dump(4).data())) {
        std::cout << "\033[1;31m[-] Failed to connect to server\033[0m\n";

        std::exception e("Failed to connect to server");
        throw e;
    }
}

int main(int argc, char* argv[]) {
    std::string addr = "";

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--addr") == 0) {
            if (i + 1 < argc) {
                addr = std::stoi(argv[i + 1]);
            }
            else {
                errno = 22;
                perror("\033[1;31m[-] Invalid --addr usage.\n[-] Usage: client --addr <remoteIPv4Address>");
                abort();
            }
        }
        else if (strcmp(argv[i], "--username") == 0) {
            if (i + 1 < argc) {
                username = argv[i + 1];
            }
            else {
                errno = 22;
                perror("\033[1;31m[-] Invalid --username usage.\n[-] Usage: client --username <username>");
                abort();
            }
        }
    }

    bindSock();
    
    socket.setBlocking(false);
    
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Test");
    window.setFramerateLimit(240);
    
    char* buffer = new char[BUFFERSIZE];
    size_t received = 0;
    sf::IpAddress recvAddr;
    unsigned short recvPort;

    sf::Vector2f ballPos;
    sf::CircleShape ball;
    ball.setFillColor(sf::Color::Red);
    ball.setRadius(8);

    float playerPos = 0.25;
    sf::RectangleShape playerPaddle;
    playerPaddle.setSize(sf::Vector2f(20, 180));
    playerPaddle.setFillColor(sf::Color::White);
    playerPaddle.setPosition(sf::Vector2f(0, 180));
    
    float opponentPos = 0.25;
    sf::RectangleShape opponentPaddle;
    opponentPaddle.setSize(sf::Vector2f(20, 180));
    opponentPaddle.setFillColor(sf::Color::White);
    opponentPaddle.setPosition(sf::Vector2f(1260, 180));

    bool leftAligned = false;

    sf::Clock clock;
    sf::Event ev;
    float dt;
    while (window.isOpen()) {
        memset(buffer, 0, BUFFERSIZE);
        dt = clock.restart().asSeconds();

        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed()) {
                socket.setBlocking(true);

                json leave;

                leave["type"] = "leave";
                leave["username"] = username;

                sendServer(leave.dump(4).data(), 5);

                window.close();
                socket.unbind();
                break;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && window.hasFocus()) {
            playerPos += (-1 * 300.f * dt) / 720;
                    
            json move;

            move["type"] = "move";
            move["data"] = playerPos;
            move["username"] = username;

            sendServer(move.dump(4).data());
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && window.hasFocus()) {
            playerPos += (1 * 300.f * dt) / 720;

            json move;

            move["type"] = "move";
            move["data"] = playerPos;
            move["username"] = username;

            sendServer(move.dump(4).data());
        }

        /*sf::Socket::Status res = sf::Socket::Done;

        std::string lastData;
        while (true) {
            res = socket.receive(buffer, BUFFERSIZE, received, recvAddr, recvPort);
            if (res != sf::Socket::Done) {
                break;
            }
            lastData.assign(buffer);
            memset(buffer, 0, BUFFERSIZE);
        }*/

        auto res = socket.receive(buffer, BUFFERSIZE, received, recvAddr, recvPort);

        if (res == sf::Socket::Done) {
            json serverJson;
            try {
                serverJson = json::parse(buffer);
            }
            catch (json::exception& e) {
                std::cout << e.what();
                abort();
            }
            
            std::string alignment = serverJson["players"][username]["alignment"];
            leftAligned = (alignment == "left");

            ballPos.x = serverJson["ball"]["posX"];
            ballPos.y = serverJson["ball"]["posY"];

            float serverPos = serverJson["players"][username]["posY"];
            if (std::abs(playerPos - serverPos) > 0.5f) {
                playerPos = serverPos;
            }

            for (auto& [key, value] : serverJson["players"].items()) {
                if (key != username) {
                    opponentPos = value["posY"];
                }
            }
        }

        if (leftAligned) {
            ball.setPosition(ballPos.x * 1280, ballPos.y * 720);
        }
        else {
            ball.setPosition((1 - ballPos.x) * 1280, ballPos.y * 720);
        }
        playerPaddle.setPosition(0, playerPos * 720);
        opponentPaddle.setPosition(1260, opponentPos * 720);

        window.clear();
        window.draw(ball);
        window.draw(playerPaddle);
        window.draw(opponentPaddle);
        window.display();
    }
    
    return 0;
}