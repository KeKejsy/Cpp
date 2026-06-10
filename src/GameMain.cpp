// GameMain.cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include "MenuScene.hpp"
#include "GameScene.hpp"
#include "ResultScene.hpp"
#include "BeatDetector.h"
#include <future>
#include "LoadingScene.hpp"
#include "ConfirmScene.hpp"

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
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Rhythm Master");
    window.setFramerateLimit(60);
    
    sf::Font& font = AssetManager::getInstance().getFont();
    
    MenuScene menu(window);
    GameScene game(window, font);
    ResultScene result(window, font);
    LoadingScene loading(window, font);
    ConfirmScene confirm(window, font);
    
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
        Loading,
        Confirm,
        Game,
        Result
    };
    
    AppState state = AppState::Menu;
    sf::Clock clock;
    std::future<Chart> detectionFuture;
    std::string pendingMusicPath;
    Difficulty currentDifficulty = Difficulty::Normal;
    
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            switch (state) {
                case AppState::Menu: {
                    MenuResult resultAction = menu.handleEvent(*event);
                    if (resultAction == MenuResult::StartGame) {
                        std::string musicPath = openFileDialog();

                        if (!musicPath.empty()) {
                            // 异步启动节拍检测，显示加载动画
                            std::cout << "=== Auto Chart Generator ===" << std::endl;
                            std::cout << "Loading: " << musicPath << std::endl;
                            pendingMusicPath = musicPath;
                            currentDifficulty = Difficulty::Normal;
                            detectionFuture = std::async(std::launch::async, [musicPath]() {
                                BeatDetector detector(Difficulty::Normal);
                                return detector.generate(musicPath);
                            });
                            state = AppState::Loading;
                        } else {
                            // 用户取消，使用测试谱面
                            std::cout << "No file selected, using test chart." << std::endl;
                            game.loadChart(testChart, "");
                            game.start();
                            state = AppState::Game;
                        }
                    } else if (resultAction == MenuResult::Exit) {
                        window.close();
                    }
                    break;
                }
                case AppState::Loading: {
                    LoadingResult loadResult = loading.handleEvent(*event);
                    if (loadResult == LoadingResult::Cancelled) {
                        state = AppState::Menu;
                    }
                    break;
                }
                case AppState::Confirm: {
                    ConfirmResult confirmResult = confirm.handleEvent(*event);
                    if (confirmResult == ConfirmResult::StartGame) {
                        Difficulty selectedDifficulty = confirm.getDifficulty();
                        if (selectedDifficulty != currentDifficulty) {
                            // 难度变更，用新难度重新生成谱面（同步，FFT 很快）
                            std::cout << "[GameMain] Difficulty changed, regenerating..." << std::endl;
                            currentDifficulty = selectedDifficulty;
                            BeatDetector detector(currentDifficulty);
                            Chart regenerated = detector.generate(pendingMusicPath);
                            if (!regenerated.notes.empty()) {
                                confirm.setChart(regenerated, pendingMusicPath);
                                game.loadChart(regenerated, pendingMusicPath);
                            } else {
                                // 降级：用已有谱面
                                game.loadChart(confirm.getChart(), pendingMusicPath);
                            }
                        } else {
                            game.loadChart(confirm.getChart(), pendingMusicPath);
                        }
                        game.start();
                        state = AppState::Game;
                    } else if (confirmResult == ConfirmResult::Back) {
                        state = AppState::Menu;
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
            case AppState::Loading:
                loading.update(deltaTime);
                loading.draw();
                // 检查异步检测是否完成
                if (detectionFuture.valid()) {
                    auto status = detectionFuture.wait_for(std::chrono::milliseconds(0));
                    if (status == std::future_status::ready) {
                        Chart generatedChart;
                        try {
                            generatedChart = detectionFuture.get();
                        } catch (const std::exception& e) {
                            std::cerr << "Beat detection failed: " << e.what() << std::endl;
                        }

                        if (!generatedChart.notes.empty()) {
                            std::cout << "Chart generated with "
                                      << generatedChart.notes.size() << " notes." << std::endl;
                        } else {
                            std::cout << "No beats detected, using test chart." << std::endl;
                            generatedChart = testChart;
                        }

                        confirm.setChart(generatedChart, pendingMusicPath);
                        state = AppState::Confirm;
                    }
                }
                break;
            case AppState::Confirm:
                confirm.update(deltaTime);
                confirm.draw();
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