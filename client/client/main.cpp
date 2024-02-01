#include <SFML/Graphics.hpp>
#include <stdio.h>
#include <iostream>

int main(int argc, char* argv[])
{
    short port = -1;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0) {
            std::cout << "Found --port" << std::endl;
            if (i + 1 < argc) {
                port = (short)std::stoi(argv[i + 1]);
                break;
            }
            else {
                errno = 22;
                perror("\033[1;31m[-] Invalid --port usage.\n[-] Usage: client --port <port>\033[0m\n");
                abort();
            }
        }
    }

    sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}