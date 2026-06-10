// GameMain.cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include "MenuScene.hpp"
#include "GameScene.hpp"
#include "ResultScene.hpp"
#include "BeatDetector.h"

// Win32 文件选择对话框
#include <windows.h>
#include <commdlg.h>

// 打开文件选择对话框，返回用户选择的文件路径，取消则返回空字符串
std::string openFileDialog() {
    OPENFILENAMEA ofn;
    char filename[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));
    ZeroMemory(filename, MAX_PATH);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Audio Files\0*.mp3;*.ogg;*.wav;*.flac\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select Music File";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

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
                        // 弹出文件选择对话框
                        std::string musicPath = openFileDialog();

                        if (!musicPath.empty()) {
                            // 用户选择了文件，自动生成谱面
                            std::cout << "=== Auto Chart Generator ===" << std::endl;
                            std::cout << "Loading: " << musicPath << std::endl;

                            BeatDetector detector;
                            Chart generatedChart = detector.generate(musicPath);

                            if (!generatedChart.notes.empty()) {
                                game.loadChart(generatedChart, musicPath);
                                std::cout << "Chart generated with "
                                          << generatedChart.notes.size() << " notes." << std::endl;
                            } else {
                                // 检测失败，使用测试谱面
                                std::cout << "No beats detected, using test chart." << std::endl;
                                game.loadChart(testChart, "");
                            }
                        } else {
                            // 用户取消了文件选择，使用测试谱面
                            std::cout << "No file selected, using test chart." << std::endl;
                            game.loadChart(testChart, "");
                        }

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