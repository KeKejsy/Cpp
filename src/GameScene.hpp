// GameScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <sstream>
#include <cmath>
#include <cstdint>
#include "GameFramework.h"
#include "AssetManager.hpp"

class GameScene {
public:
    GameScene(sf::RenderWindow& window, sf::Font& font)
        : m_window(window)
        , m_font(font)
        , m_finished(false)
        , m_judgmentDisplayTimer(0.0f)
        , m_comboScaleTimer(0.0f)
        , m_comboScale(1.0f) {
        setupUI();
    }

    void loadChart(const Chart& chart, const std::string& musicPath = "") {
        m_game.loadChart(chart, musicPath);
    }

    void start() {
        m_game.startGame();
        m_finished = false;
        m_judgmentDisplayTimer = 0.0f;
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
        
        if (m_judgmentDisplayTimer > 0.0f) {
            m_judgmentDisplayTimer -= deltaTime;
        }
        
        if (m_comboScaleTimer > 0.0f) {
            m_comboScaleTimer -= deltaTime;
            m_comboScale = 1.0f + (m_comboScaleTimer * 2.0f);
            if (m_comboScale < 1.0f) m_comboScale = 1.0f;
        }
        
        Judgment latest = m_game.getLatestJudgment();
        if (latest != Judgment::None) {
            switch (latest) {
                case Judgment::Perfect:
                    m_currentJudgmentText = "PERFECT!";
                    m_currentJudgmentColor = sf::Color::Yellow;
                    m_judgmentDisplayTimer = 0.5f;
                    m_comboScaleTimer = 0.2f;
                    break;
                case Judgment::Great:
                    m_currentJudgmentText = "GREAT!";
                    m_currentJudgmentColor = sf::Color::Green;
                    m_judgmentDisplayTimer = 0.4f;
                    m_comboScaleTimer = 0.15f;
                    break;
                case Judgment::Good:
                    m_currentJudgmentText = "GOOD";
                    m_currentJudgmentColor = sf::Color::Blue;
                    m_judgmentDisplayTimer = 0.3f;
                    break;
                case Judgment::Miss:
                    m_currentJudgmentText = "MISS";
                    m_currentJudgmentColor = sf::Color::Red;
                    m_judgmentDisplayTimer = 0.3f;
                    break;
                default:
                    break;
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
            if (note.hit) continue;
            
            double timeDiff = note.time - currentTime;
            if (timeDiff < -0.5f || timeDiff > 3.0f) continue;
            
            float noteY = m_trackY - static_cast<float>(timeDiff * 200.0);
            if (noteY < 50.0f || noteY > m_trackY + 50.0f) continue;
            
            sf::RectangleShape noteShape(sf::Vector2f(m_trackWidth - 20.0f, 30.0f));
            noteShape.setPosition(sf::Vector2f(
                m_trackStartX + note.track * (m_trackWidth + 10.0f) + 10.0f,
                noteY
            ));
            noteShape.setFillColor(m_tracks[note.track].color);
            m_window.draw(noteShape);
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
        if (m_judgmentDisplayTimer > 0.0f && !m_currentJudgmentText.empty()) {
            sf::Text judgmentText(m_font, m_currentJudgmentText, 40);
            judgmentText.setFillColor(m_currentJudgmentColor);
            
            sf::FloatRect bounds = judgmentText.getLocalBounds();
            judgmentText.setOrigin(sf::Vector2f(bounds.size.x / 2.0f, bounds.size.y / 2.0f));
            judgmentText.setPosition(sf::Vector2f(400.0f, m_trackY - 80.0f));
            
            float offsetY = (0.5f - m_judgmentDisplayTimer) * 50.0f;
            judgmentText.move(sf::Vector2f(0.0f, offsetY));
            
            m_window.draw(judgmentText);
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
    
    float m_judgmentDisplayTimer;
    std::string m_currentJudgmentText;
    sf::Color m_currentJudgmentColor;
    
    float m_comboScaleTimer;
    float m_comboScale;
};