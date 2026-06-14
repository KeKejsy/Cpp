#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include "Framework.h"
using namespace std;

int main() {
    cout << "=== 节奏大师 - 交互式测试 ===" << endl;
    cout << "按 SPACE 开始游戏" << endl;
    cout << "按 D/F/J/K 对应轨道 0/1/2/3" << endl;
    cout << "按 ESC 退出" << endl;
    cout << endl;

    // 创建窗口
    sf::RenderWindow window(sf::VideoMode({800, 600}), "节奏大师 - 交互式测试");
    window.setFramerateLimit(60);

    // 加载字体
    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
        cerr << "无法加载字体！" << endl;
        return 1;
    }

    // 创建测试谱面
    Chart testChart;
    testChart.songName = "测试歌曲";
    testChart.noteDensity = 120.0;
    testChart.duration = 30.0;

    // 生成音符：每0.5秒一个，循环4个轨道
    for (int i = 0; i < 60; i++) {
        Note note;
        note.time = 2.0 + i * 0.5;
        note.track = i % 4;
        note.type = 0;
        note.hit = false;
        testChart.notes.push_back(note);
    }

    // 创建游戏框架
    Framework game;
    game.loadChart(testChart, "");

    // 轨道颜色
    sf::Color trackColors[] = {
        sf::Color::Red,
        sf::Color::Green,
        sf::Color::Blue,
        sf::Color::Yellow
    };

    // 轨道位置
    float trackWidth = 150.0;
    float trackStartX = 100.0;
    float trackY = 500.0;

    // 主循环
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    if (!game.isPlaying()) {
                        cout << "游戏开始！" << endl;
                        game.startGame();
                    }
                }

                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }

                // 按键判定
                if (keyPressed->code == sf::Keyboard::Key::D) {
                    game.handleKeyPress(0);
                }
                if (keyPressed->code == sf::Keyboard::Key::F) {
                    game.handleKeyPress(1);
                }
                if (keyPressed->code == sf::Keyboard::Key::J) {
                    game.handleKeyPress(2);
                }
                if (keyPressed->code == sf::Keyboard::Key::K) {
                    game.handleKeyPress(3);
                }
            }

            if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
                if (keyReleased->code == sf::Keyboard::Key::D) {
                    game.handleKeyRelease(0);
                }
                if (keyReleased->code == sf::Keyboard::Key::F) {
                    game.handleKeyRelease(1);
                }
                if (keyReleased->code == sf::Keyboard::Key::J) {
                    game.handleKeyRelease(2);
                }
                if (keyReleased->code == sf::Keyboard::Key::K) {
                    game.handleKeyRelease(3);
                }
            }
        }

        // 更新游戏状态
        if (game.isPlaying()) {
            game.update(game.getMusicTime());
        }

        // 渲染
        window.clear(sf::Color::Black);

        // 绘制轨道背景
        for (int i = 0; i < 4; i++) {
            sf::RectangleShape track({trackWidth, 400});
            track.setPosition({trackStartX + i * (trackWidth + 10), 100});
            track.setFillColor(sf::Color(50, 50, 50));
            window.draw(track);
        }

        // 绘制判定线
        sf::RectangleShape judgmentLine({4 * trackWidth + 30, 5});
        judgmentLine.setPosition({trackStartX - 5, trackY});
        judgmentLine.setFillColor(sf::Color::White);
        window.draw(judgmentLine);

        // 绘制音符
        if (game.isPlaying()) {
            double currentTime = game.getMusicTime();
            const auto& notes = game.getNotes();

            for (const auto& note : notes) {
                if (note.hit) continue;

                // 计算音符位置（从上往下落）
                double timeDiff = note.time - currentTime;
                if (timeDiff < -0.5 || timeDiff > 3.0) continue;

                float noteY = trackY - (float)(timeDiff * 200.0);
                if (noteY < 50 || noteY > trackY + 50) continue;

                sf::RectangleShape noteShape({trackWidth - 20, 30});
                noteShape.setPosition({trackStartX + note.track * (trackWidth + 10) + 10, noteY});
                noteShape.setFillColor(trackColors[note.track]);
                window.draw(noteShape);
            }
        }

        // 绘制得分信息
        const auto& stats = game.getStats();
        stringstream ss;
        ss << "Score: " << stats.totalScore << "\n";
        ss << "Combo: " << stats.combo << "\n";
        ss << "Perfect: " << stats.perfectCount << "\n";
        ss << "Great: " << stats.greatCount << "\n";
        ss << "Good: " << stats.goodCount << "\n";
        ss << "Miss: " << stats.missCount;

        sf::Text scoreText(font, ss.str(), 20);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition({600, 50});
        window.draw(scoreText);

        // 绘制状态信息
        if (!game.isPlaying()) {
            sf::Text statusText(font, "Press SPACE to start", 24);
            statusText.setFillColor(sf::Color::Cyan);
            statusText.setPosition({250, 300});
            window.draw(statusText);
        } else {
            sf::Text statusText(font, "Playing...", 24);
            statusText.setFillColor(sf::Color::Cyan);
            statusText.setPosition({350, 30});
            window.draw(statusText);
        }

        // 绘制按键提示
        sf::Text keyHint(font, "D    F    J    K", 18);
        keyHint.setFillColor(sf::Color::Yellow);
        keyHint.setPosition({trackStartX + 20, trackY + 30});
        window.draw(keyHint);

        // 绘制最新判定
        if (game.isPlaying()) {
            Judgment latest = game.getLatestJudgment();
            if (latest != Judgment::None) {
                string judgmentStr;
                sf::Color judgmentColor;

                switch (latest) {
                    case Judgment::Perfect:
                        judgmentStr = "PERFECT";
                        judgmentColor = sf::Color::Yellow;
                        break;
                    case Judgment::Great:
                        judgmentStr = "GREAT";
                        judgmentColor = sf::Color::Green;
                        break;
                    case Judgment::Good:
                        judgmentStr = "GOOD";
                        judgmentColor = sf::Color::Blue;
                        break;
                    case Judgment::Miss:
                        judgmentStr = "MISS";
                        judgmentColor = sf::Color::Red;
                        break;
                    default:
                        break;
                }

                sf::Text judgmentText(font, judgmentStr, 30);
                judgmentText.setPosition({350, trackY - 50});
                judgmentText.setFillColor(judgmentColor);
                window.draw(judgmentText);
            }
        }

        window.display();
    }

    return 0;
}
