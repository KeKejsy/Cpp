// GameScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "GameFramework.h"
#include "AssetManager.hpp"

class GameScene {
public:
    GameScene(sf::RenderWindow& window, sf::Font& font)
        : m_window(window)
        , m_font(font)
        , m_finished(false)
        , m_comboScaleTimer(0.0f)
        , m_comboScale(1.0f)
        , m_trackJudgmentTimer(4, 0.0f)
        , m_trackJudgmentText(4, "")
        , m_trackJudgmentColor(4, sf::Color::White) {
        setupUI();
    }

    void loadChart(const Chart& chart, const std::string& musicPath = "") {
        m_game.loadChart(chart, musicPath);
    }

    void start() {
        m_game.startGame();
        m_finished = false;
        std::fill(m_trackJudgmentTimer.begin(), m_trackJudgmentTimer.end(), 0.0f);
    }

    void handleEvent(const sf::Event& event) {
        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            switch (keyPressed->code) {
                case sf::Keyboard::Key::D: m_game.handleKeyPress(0); break;
                case sf::Keyboard::Key::F: m_game.handleKeyPress(1); break;
                case sf::Keyboard::Key::J: m_game.handleKeyPress(2); break;
                case sf::Keyboard::Key::K: m_game.handleKeyPress(3); break;
                default: break;
            }
        }
        
        if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
            switch (keyReleased->code) {
                case sf::Keyboard::Key::D: m_game.handleKeyRelease(0); break;
                case sf::Keyboard::Key::F: m_game.handleKeyRelease(1); break;
                case sf::Keyboard::Key::J: m_game.handleKeyRelease(2); break;
                case sf::Keyboard::Key::K: m_game.handleKeyRelease(3); break;
                default: break;
            }
        }
    }

    void update(float deltaTime) {
        if (m_game.isPlaying()) {
            m_game.update(m_game.getMusicTime());
        } else if (m_game.getState() == GameState::Result && !m_finished) {
            m_finished = true;
        }

        // 更新每个轨道的判定显示计时器
        for (int i = 0; i < 4; i++) {
            if (m_trackJudgmentTimer[i] > 0.0f) {
                m_trackJudgmentTimer[i] -= deltaTime;
            }
        }

        if (m_comboScaleTimer > 0.0f) {
            m_comboScaleTimer -= deltaTime;
            m_comboScale = 1.0f + (m_comboScaleTimer * 2.0f);
            if (m_comboScale < 1.0f) m_comboScale = 1.0f;
        }

        // 检测新的判定，设置对应轨道的显示
        Judgment latest = m_game.getLatestJudgment();
        double latestTime = m_game.getLatestJudgmentTime();
        int track = m_game.getLatestJudgmentTrack();

        if (latest != Judgment::None && track >= 0 && track < 4) {
            // 只处理新的判定（时间戳变化）
            static double s_lastJudgmentTime = -1.0;
            if (latestTime != s_lastJudgmentTime) {
                s_lastJudgmentTime = latestTime;
                sf::Color trackColor = m_tracks[track].color;

                switch (latest) {
                    case Judgment::Perfect:
                        m_trackJudgmentText[track] = "PERFECT!";
                        m_trackJudgmentColor[track] = sf::Color(
                            std::min(255, trackColor.r + 150),
                            std::min(255, trackColor.g + 150),
                            std::min(255, trackColor.b + 150));
                        m_trackJudgmentTimer[track] = 0.5f;
                        m_comboScaleTimer = 0.2f;
                        break;
                    case Judgment::Great:
                        m_trackJudgmentText[track] = "GREAT!";
                        m_trackJudgmentColor[track] = sf::Color(
                            std::min(255, trackColor.r + 80),
                            std::min(255, trackColor.g + 80),
                            std::min(255, trackColor.b + 80));
                        m_trackJudgmentTimer[track] = 0.4f;
                        m_comboScaleTimer = 0.15f;
                        break;
                    case Judgment::Good:
                        m_trackJudgmentText[track] = "GOOD";
                        m_trackJudgmentColor[track] = trackColor;
                        m_trackJudgmentTimer[track] = 0.3f;
                        break;
                    case Judgment::Miss:
                        m_trackJudgmentText[track] = "MISS";
                        m_trackJudgmentColor[track] = sf::Color(
                            std::max(0, trackColor.r - 50),
                            std::max(0, trackColor.g - 50),
                            std::max(0, trackColor.b - 50));
                        m_trackJudgmentTimer[track] = 0.3f;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    void draw() {
        for (size_t i = 0; i < m_tracks.size(); i++) {
            sf::RectangleShape track(sf::Vector2f(m_tracks[i].width, 400.0f));
            track.setPosition(sf::Vector2f(m_tracks[i].x, 100.0f));
            track.setFillColor(sf::Color(50, 50, 50));
            m_window.draw(track);
        }
        
        sf::RectangleShape judgmentLine(sf::Vector2f(4.0f * m_trackWidth + 30.0f, 5.0f));
        judgmentLine.setPosition(sf::Vector2f(m_trackStartX - 5.0f, m_trackY));
        judgmentLine.setFillColor(sf::Color::White);
        m_window.draw(judgmentLine);
        
        if (m_game.isPlaying()) {
            drawNotes();
        }

        drawUI();
        drawJudgment();
    }

    bool isPlaying() const { return m_game.isPlaying(); }
    const GameStats& getStats() const { return m_game.getStats(); }
    bool isFinished() const { return m_finished; }
    void resetFinished() { m_finished = false; }

private:
    struct TrackConfig {
        float x;
        float width;
        sf::Color color;
        
        TrackConfig() : x(0.0f), width(0.0f), color(sf::Color::White) {}
    };

    void setupUI() {
        m_trackWidth = 150.0f;
        m_trackStartX = 100.0f;
        m_trackY = 500.0f;
        
        sf::Color trackColors[] = {
            sf::Color::Red,
            sf::Color::Green,
            sf::Color::Blue,
            sf::Color::Yellow
        };
        
        for (int i = 0; i < 4; i++) {
            TrackConfig track;
            track.x = m_trackStartX + i * (m_trackWidth + 10.0f);
            track.width = m_trackWidth;
            track.color = trackColors[i];
            m_tracks.push_back(track);
        }
    }

    void drawNotes() {
        double currentTime = m_game.getMusicTime();
        const auto& notes = m_game.getNotes();

        for (const auto& note : notes) {
            // tap 音符：被击中后 0.3s 消失
            if (note.type != 1 && note.hit && currentTime - note.hitTime > 0.3) continue;

            double timeDiff = note.time - currentTime;
            // tap 音符超出范围跳过；hold 音符由渲染条件自行裁剪
            if (note.type != 1 && (timeDiff < -0.5f || timeDiff > 3.0f)) continue;
            // hold 音符：只在完全超出可见范围时跳过
            if (note.type == 1) {
                double endTimeDiff = (note.time + note.duration) - currentTime;
                if (timeDiff > 3.0f || endTimeDiff < -0.3f) continue;
            }

            float noteX = m_trackStartX + note.track * (m_trackWidth + 10.0f) + 10.0f;
            float noteWidth = m_trackWidth - 20.0f;
            sf::Color trackColor = m_tracks[note.track].color;

            if (note.type == 1 && note.duration > 0.0) {
                // hold 音符：渲染为长条矩形
                float noteHeadY = m_trackY - static_cast<float>(timeDiff * 200.0);
                double endTimeDiff = (note.time + note.duration) - currentTime;
                float noteTailY = m_trackY - static_cast<float>(endTimeDiff * 200.0);

                // 裁剪：头部超出底部时截断，尾部超出顶部时截断
                float drawTop = noteTailY;
                float drawBottom = noteHeadY;
                if (drawTop < 50.0f) drawTop = 50.0f;
                if (drawBottom > m_trackY + 50.0f) drawBottom = m_trackY + 50.0f;

                float holdHeight = drawBottom - drawTop;
                if (holdHeight < 1.0f) holdHeight = 1.0f;

                // 只在可见范围内绘制（尾部还没超出底部）
                if (noteTailY < m_trackY + 50.0f && drawBottom > drawTop) {
                    int noteIndex = static_cast<int>(&note - &notes[0]);
                    bool isBeingHeld = m_game.isNoteBeingHeld(noteIndex);

                    std::uint8_t barAlpha = isBeingHeld ? 200 : 100;
                    std::uint8_t headAlpha = isBeingHeld ? 255 : 180;

                    // 绘制长条（裁剪后的可见部分）
                    sf::RectangleShape holdBar(sf::Vector2f(noteWidth, holdHeight));
                    holdBar.setPosition(sf::Vector2f(noteX, drawTop));
                    holdBar.setFillColor(sf::Color(trackColor.r, trackColor.g, trackColor.b, barAlpha));
                    m_window.draw(holdBar);

                    // 绘制 hold 音符头部（在可见范围内时）
                    if (noteHeadY >= 50.0f && noteHeadY <= m_trackY + 50.0f) {
                        sf::RectangleShape noteShape(sf::Vector2f(noteWidth, 30.0f));
                        noteShape.setPosition(sf::Vector2f(noteX, noteHeadY - 15.0f));
                        noteShape.setFillColor(sf::Color(trackColor.r, trackColor.g, trackColor.b, headAlpha));
                        m_window.draw(noteShape);
                    }
                }
            } else {
                // tap 音符：渲染为矩形（原有逻辑）
                float noteY = m_trackY - static_cast<float>(timeDiff * 200.0);
                if (noteY < 50.0f || noteY > m_trackY + 50.0f) continue;

                sf::RectangleShape noteShape(sf::Vector2f(noteWidth, 30.0f));
                noteShape.setPosition(sf::Vector2f(noteX, noteY));
                noteShape.setFillColor(trackColor);
                m_window.draw(noteShape);
            }
        }
    }

    void drawUI() {
        const auto& stats = m_game.getStats();
        std::stringstream ss;
        ss << "Score: " << stats.totalScore << "\n";
        ss << "Combo: " << stats.combo << "\n";
        ss << "Perfect: " << stats.perfectCount << "\n";
        ss << "Great: " << stats.greatCount << "\n";
        ss << "Good: " << stats.goodCount << "\n";
        ss << "Miss: " << stats.missCount;
        
        sf::Text scoreText(m_font, ss.str(), 20);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(sf::Vector2f(600.0f, 50.0f));
        m_window.draw(scoreText);
        
        if (stats.combo >= 5) {
            sf::Text comboText(m_font, std::to_string(stats.combo) + " COMBO!", 36);
            comboText.setFillColor(sf::Color(255, 200, 100));
            
            sf::FloatRect bounds = comboText.getLocalBounds();
            comboText.setOrigin(sf::Vector2f(bounds.size.x / 2.0f, bounds.size.y / 2.0f));
            comboText.setPosition(sf::Vector2f(400.0f, 150.0f));
            comboText.setScale(sf::Vector2f(m_comboScale, m_comboScale));
            
            m_window.draw(comboText);
        }
        
        sf::Text keyHint(m_font, "D    F    J    K", 18);
        keyHint.setFillColor(sf::Color::Yellow);
        keyHint.setPosition(sf::Vector2f(m_trackStartX + 20.0f, m_trackY + 30.0f));
        m_window.draw(keyHint);
    }

    void drawJudgment() {
        for (int i = 0; i < 4; i++) {
            if (m_trackJudgmentTimer[i] > 0.0f && !m_trackJudgmentText[i].empty()) {
                sf::Text judgmentText(m_font, m_trackJudgmentText[i], 32);
                judgmentText.setFillColor(m_trackJudgmentColor[i]);

                sf::FloatRect bounds = judgmentText.getLocalBounds();
                judgmentText.setOrigin(sf::Vector2f(bounds.size.x / 2.0f, bounds.size.y / 2.0f));

                // 每个轨道判定显示在对应轨道的判定线上方
                float trackCenterX = m_tracks[i].x + m_tracks[i].width / 2.0f;
                judgmentText.setPosition(sf::Vector2f(trackCenterX, m_trackY - 60.0f));

                // 向上飘动效果
                float progress = 1.0f - (m_trackJudgmentTimer[i] / 0.5f);
                float offsetY = progress * 30.0f;
                judgmentText.move(sf::Vector2f(0.0f, -offsetY));

                m_window.draw(judgmentText);
            }
        }
    }

    sf::RenderWindow& m_window;
    sf::Font& m_font;
    GameFramework m_game;

    std::vector<TrackConfig> m_tracks;
    float m_trackStartX;
    float m_trackWidth;
    float m_trackY;
    bool m_finished;

    float m_comboScaleTimer;
    float m_comboScale;

    // 每个轨道独立的判定显示
    std::vector<float> m_trackJudgmentTimer;
    std::vector<std::string> m_trackJudgmentText;
    std::vector<sf::Color> m_trackJudgmentColor;
};