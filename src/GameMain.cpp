// GameMain.cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include "MenuScene.hpp"
#include "GameScene.hpp"
#include "ResultScene.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Rhythm Master");
    window.setFramerateLimit(60);
    
    sf::Font& font = AssetManager::getInstance().getFont();
    
    MenuScene menu(window);
    GameScene game(window, font);
    ResultScene result(window, font);
    
    Chart testChart;
    testChart.songName = "Test Song";
    testChart.bpm = 120.0;
    testChart.duration = 30.0;
    
    for (int i = 0; i < 60; i++) {
        Note note;
        note.time = 2.0 + i * 0.5;
        note.track = i % 4;
        note.type = 0;
        note.hit = false;
        testChart.notes.push_back(note);
    }
    
    game.loadChart(testChart, "");
    
    enum class AppState {
        Menu,
        Game,
        Result
    };
    
    AppState state = AppState::Menu;
    sf::Clock clock;
    
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            switch (state) {
                case AppState::Menu: {
                    MenuResult resultAction = menu.handleEvent(*event);
                    if (resultAction == MenuResult::StartGame) {
                        game.start();
                        state = AppState::Game;
                    } else if (resultAction == MenuResult::Exit) {
                        window.close();
                    }
                    break;
                }
                case AppState::Game:
                    game.handleEvent(*event);
                    break;
                case AppState::Result: {
                    ResultAction action = result.handleEvent(*event);
                    if (action == ResultAction::Restart) {
                        game.start();
                        state = AppState::Game;
                    } else if (action == ResultAction::BackToMenu) {
                        state = AppState::Menu;
                    }
                    break;
                }
            }
        }
        
        float deltaTime = clock.restart().asSeconds();
        
        window.clear();
        
        switch (state) {
            case AppState::Menu:
                menu.update(deltaTime);
                menu.draw();
                break;
            case AppState::Game:
                game.update(deltaTime);
                game.draw();
                if (game.isFinished()) {
                    result.setStats(game.getStats());
                    state = AppState::Result;
                }
                break;
            case AppState::Result:
                result.update(deltaTime);
                result.draw();
                break;
        }
        
        window.display();
    }
    
    return 0;
}