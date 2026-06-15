// GameScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "Framework.h"
#include "AssetManager.hpp"
using namespace std;

class GameScene {
public:
    GameScene(sf::RenderWindow& window, sf::Font& font)
        : m_window(window)
        , m_font(font)
        , m_phase(Phase::Preparing)
        , m_countdownTimer(0.0)
        , m_finished(false)
        , m_comboScaleTimer(0.0)
        , m_comboScale(1.0)
        , m_trackJudgmentTimer(4, 0.0)
        , m_trackJudgmentText(4, "")
        , m_trackJudgmentColor(4, sf::Color::White) {
        setupUI();
    }

    void loadChart(const Chart& chart, const string& musicPath = "") {
        m_game.loadChart(chart, musicPath);
    }

    void start() {
        // 进入倒计时阶段，不立刻开始音�?
        m_phase = Phase::Preparing;
        m_countdownTimer = 0.0;
        m_finished = false;
        fill(m_trackJudgmentTimer.begin(), m_trackJudgmentTimer.end(), 0.0);
    }

    void handleEvent(const sf::Event& event) {
        // 倒计时期间不响应游戏按键
        if (m_phase != Phase::Playing) return;

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
        // 倒计时阶段：累积时间，到点启动音�?
        if (m_phase == Phase::Preparing) {
            m_countdownTimer += deltaTime;
            if (m_countdownTimer >= COUNTDOWN_DURATION) {
                m_game.startGame();  // 播放音乐，进�? Playing 状�?
                m_phase = Phase::Playing;
            }
            // 倒计时期间不运行游戏逻辑
        } else if (m_phase == Phase::Playing && m_game.isPlaying()) {
            // 先捕获按键产生的判定（handleEvent 中已设置），再运�? game update
            processJudgmentDisplay();
            m_game.update(m_game.getMusicTime());
            // 再捕�? auto-miss �? update 内产生的判定
            processJudgmentDisplay();
        }

        if (m_game.getState() == GameState::Result && !m_finished) {
            m_finished = true;
            m_phase = Phase::Finished;
        }

        // 更新每个轨道的判定显示计时器
        for (int i = 0; i < 4; i++) {
            if (m_trackJudgmentTimer[i] > 0.0) {
                m_trackJudgmentTimer[i] -= deltaTime;
            }
        }

        if (m_comboScaleTimer > 0.0) {
            m_comboScaleTimer -= deltaTime;
            m_comboScale = 1.0 + (m_comboScaleTimer * 2.0);
            if (m_comboScale < 1.0) m_comboScale = 1.0;
        }
    }

    // 检测并显示最新的判定
    void processJudgmentDisplay() {
        Judgment latest = m_game.getLatestJudgment();
        double latestTime = m_game.getLatestJudgmentTime();
        int track = m_game.getLatestJudgmentTrack();

        if (latest == Judgment::None || track < 0 || track >= 4) return;

        if (latestTime == m_lastDisplayedJudgmentTime) return;
        m_lastDisplayedJudgmentTime = latestTime;

        switch (latest) {
            case Judgment::Perfect:
                m_trackJudgmentText[track] = "PERFECT!";
                m_trackJudgmentColor[track] = sf::Color(255, 215, 0);
                m_trackJudgmentTimer[track] = 0.5;
                m_comboScaleTimer = 0.2;
                break;
            case Judgment::Great:
                m_trackJudgmentText[track] = "GREAT!";
                m_trackJudgmentColor[track] = sf::Color(200, 200, 100);
                m_trackJudgmentTimer[track] = 0.4;
                m_comboScaleTimer = 0.15;
                break;
            case Judgment::Good:
                m_trackJudgmentText[track] = "GOOD";
                m_trackJudgmentColor[track] = sf::Color(150, 150, 200);
                m_trackJudgmentTimer[track] = 0.3;
                break;
            case Judgment::Miss:
                m_trackJudgmentText[track] = "MISS";
                m_trackJudgmentColor[track] = sf::Color(255, 80, 80);
                m_trackJudgmentTimer[track] = 0.3;
                break;
            default:
                break;
        }
    }

    void draw() {
        for (size_t i = 0; i < m_tracks.size(); i++) {
            sf::RectangleShape track(sf::Vector2f(m_tracks[i].width, m_trackHeight));
            track.setPosition(sf::Vector2f(m_tracks[i].x, m_trackTop));
            track.setFillColor(sf::Color(50, 50, 50));
            m_window.draw(track);
        }

        sf::RectangleShape judgmentLine(sf::Vector2f(4.0 * m_trackWidth + 3.0 * m_trackGap + 10.0, 5.0));
        judgmentLine.setPosition(sf::Vector2f(m_trackStartX - 5.0, m_trackY));
        judgmentLine.setFillColor(sf::Color::White);
        m_window.draw(judgmentLine);

        // 倒计时动�?
        if (m_phase == Phase::Preparing) {
            drawCountdown();
        }

        if (m_phase == Phase::Playing && m_game.isPlaying()) {
            drawNotes();
        }

        drawUI();
        drawJudgment();
    }

    bool isPlaying() const { return m_game.isPlaying(); }
    const GameStats& getStats() const { return m_game.getStats(); }
    bool isFinished() const { return m_finished && m_phase == Phase::Finished; }
    void resetFinished() { m_finished = false; }

private:
    enum class Phase {
        Preparing,   // 3-2-1-GO 倒计�?
        Playing,     // 正常游戏
        Finished     // 结束
    };
    static constexpr float COUNTDOWN_DURATION = 3.0;

    struct TrackConfig {
        float x;
        float width;
        sf::Color color;
        
        TrackConfig() : x(0.0), width(0.0), color(sf::Color::White) {}
    };

    void setupUI() {
        m_trackWidth = 190.0;
        m_trackGap = 15.0;
        m_trackStartX = 237.5;
        // 判定线下移到底部，最大化轨道长度
        m_trackY = 640.0;
        m_trackTop = 10.0;
        m_trackHeight = m_trackY - m_trackTop;
        m_noteSpeed = 150.0;
        // 可见时间: 630/150 = 4.2秒（比之�?2.7秒多56%�?
        m_visibleAhead = 4.5;

        sf::Color trackColors[] = {
            sf::Color::Red,
            sf::Color::Green,
            sf::Color::Blue,
            sf::Color::Yellow
        };

        for (int i = 0; i < 4; i++) {
            TrackConfig track;
            track.x = m_trackStartX + i * (m_trackWidth + m_trackGap);
            track.width = m_trackWidth;
            track.color = trackColors[i];
            m_tracks.push_back(track);
        }
    }

    void drawNotes() {
        double currentTime = m_game.getMusicTime();
        const auto& notes = m_game.getNotes();

        for (const auto& note : notes) {
            float noteX = m_trackStartX + note.track * (m_trackWidth + m_trackGap) + 10.0;
            float noteWidth = m_trackWidth - 20.0;
            
            // ������������������ɫ���������ֹ��
            sf::Color noteColor;
            if (note.type == 1) {
                noteColor = sf::Color(100, 180, 255);  // ����ɫ
            } else {
                noteColor = sf::Color(255, 220, 100);  // ӫ��ɫ
            }

            // ---- tap ���� ----
            if (note.type != 1) {
                // �ѱ����У��������ж���λ�ã�������ʧ
                if (note.hit) {
                    double elapsed = currentTime - note.hitTime;
                    if (elapsed > 0.25) continue;

                    float alpha = 1.0 - elapsed / 0.25;
                    sf::Color fadeColor(noteColor.r, noteColor.g, noteColor.b,
                                        static_cast<unsigned char>(180 * alpha));
                    sf::RectangleShape noteShape(sf::Vector2f(noteWidth, 30.0));
                    noteShape.setPosition(sf::Vector2f(noteX, m_trackY));
                    noteShape.setFillColor(fadeColor);
                    m_window.draw(noteShape);
                    continue;
                }

                // δ���У���ʱ������λ��
                double timeDiff = note.time - currentTime;
                if (timeDiff < -0.6 || timeDiff > m_visibleAhead) continue;

                float noteY = m_trackY - timeDiff * m_noteSpeed;
                if (noteY < m_trackTop || noteY > m_trackY + 60.0) continue;

                sf::RectangleShape noteShape(sf::Vector2f(noteWidth, 10.0));
                noteShape.setPosition(sf::Vector2f(noteX, noteY));
                noteShape.setFillColor(noteColor);  // ʹ������ɫ
                m_window.draw(noteShape);
                continue;
            }

            // ---- hold ���� ----
            if (note.type == 1 && note.duration > 0.0) {
                double timeDiff = note.time - currentTime;
                double endTimeDiff = (note.time + note.duration) - currentTime;

                // �ѱ�����
                if (note.hit) {
                    double elapsed = currentTime - note.hitTime;
                    if (elapsed > 0.3) continue;

                    float alpha = 1.0 - static_cast<float>(elapsed / 0.3);
                    sf::Color fadeColor(noteColor.r, noteColor.g, noteColor.b,
                                        static_cast<unsigned char>(150 * alpha));
                    sf::RectangleShape noteShape(sf::Vector2f(noteWidth, 30.0));
                    noteShape.setPosition(sf::Vector2f(noteX, m_trackY - 15.0));
                    noteShape.setFillColor(fadeColor);
                    m_window.draw(noteShape);
                    continue;
                }

                if (timeDiff > m_visibleAhead || endTimeDiff < -0.5) continue;

                float noteHeadY = m_trackY - timeDiff * m_noteSpeed;
                float noteTailY = m_trackY - endTimeDiff * m_noteSpeed;

                float drawTop = noteTailY;
                float drawBottom = noteHeadY;
                float clipBottom = m_trackY + 50.0;
                if (drawTop < m_trackTop) drawTop = m_trackTop;
                if (drawBottom > clipBottom) drawBottom = clipBottom;

                float holdHeight = drawBottom - drawTop;
                if (holdHeight < 1.0) holdHeight = 1.0;

                if (noteTailY < clipBottom && drawBottom > drawTop) {
                    int noteIndex = static_cast<int>(&note - &notes[0]);
                    bool isBeingHeld = m_game.isNoteBeingHeld(noteIndex);

                    unsigned char barAlpha = isBeingHeld ? 200 : 100;
                    unsigned char headAlpha = isBeingHeld ? 255 : 180;

                    // hold ����ʹ�õ���ɫ
                    sf::RectangleShape holdBar(sf::Vector2f(noteWidth, holdHeight));
                    holdBar.setPosition(sf::Vector2f(noteX, drawTop));
                    holdBar.setFillColor(sf::Color(noteColor.r, noteColor.g, noteColor.b, barAlpha));
                    m_window.draw(holdBar);

                    // hold ͷ��ʹ�õ���ɫ
                    if (noteHeadY >= m_trackTop && noteHeadY <= clipBottom) {
                        sf::RectangleShape noteShape(sf::Vector2f(noteWidth, 10.0));
                        noteShape.setPosition(sf::Vector2f(noteX, noteHeadY - 15.0));
                        noteShape.setFillColor(sf::Color(noteColor.r, noteColor.g, noteColor.b, headAlpha));
                        m_window.draw(noteShape);
                    }
                }
            }
        }
    }

    void drawCountdown() {
        sf::Vector2u windowSize = m_window.getSize();
        float centerX = windowSize.x / 2.0;
        float centerY = m_trackY - 150.0;

        string countdownText;
        sf::Color countdownColor;
        float textScale = 1.0;

        if (m_countdownTimer < 0.9) {
            countdownText = "3";
            countdownColor = sf::Color::White;
        } else if (m_countdownTimer < 1.6) {
            countdownText = "2";
            countdownColor = sf::Color::White;
        } else if (m_countdownTimer < 2.3) {
            countdownText = "1";
            countdownColor = sf::Color(255, 255, 100);
        } else {
            countdownText = "GO!";
            countdownColor = sf::Color(100, 255, 100);
            textScale = 1.3;
        }

        sf::Text text(m_font, countdownText, 120);
        text.setStyle(sf::Text::Bold);

        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(bounds.size.x / 2.0, bounds.size.y / 2.0));
        text.setPosition(sf::Vector2f(centerX, centerY));
        text.setScale(sf::Vector2f(textScale, textScale));

        // 脉冲透明效果
        float fractional = m_countdownTimer - floor(m_countdownTimer);
        float alpha = 0.6 + 0.4 * sin(fractional * 3.14159);
        countdownColor.a = static_cast<unsigned char>(255 * alpha);
        text.setFillColor(countdownColor);

        m_window.draw(text);
    }

    void drawUI() {
        const auto& stats = m_game.getStats();
        stringstream ss;
        ss << "Score: " << stats.totalScore << "\n";
        ss << "Combo: " << stats.combo << "\n";
        ss << "Perfect: " << stats.perfectCount << "\n";
        ss << "Great: " << stats.greatCount << "\n";
        ss << "Good: " << stats.goodCount << "\n";
        ss << "Miss: " << stats.missCount;
        
        sf::Text scoreText(m_font, ss.str(), 20);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(sf::Vector2f(1060.0, 50.0));
        m_window.draw(scoreText);
        
        if (stats.combo >= 5) {
            sf::Text comboText(m_font, to_string(stats.combo) + " COMBO!", 36);
            comboText.setFillColor(sf::Color(255, 200, 100));
            
            sf::FloatRect bounds = comboText.getLocalBounds();
            comboText.setOrigin(sf::Vector2f(bounds.size.x / 2.0, bounds.size.y / 2.0));
            comboText.setPosition(sf::Vector2f(640.0, 150.0));
            comboText.setScale(sf::Vector2f(m_comboScale, m_comboScale));
            
            m_window.draw(comboText);
        }
        
        sf::Text keyHint(m_font, "D    F    J    K", 18);
        keyHint.setFillColor(sf::Color::Yellow);
        keyHint.setPosition(sf::Vector2f(m_trackStartX + 20.0, m_trackY + 30.0));
        m_window.draw(keyHint);
    }

    void drawJudgment() {
        for (int i = 0; i < 4; i++) {
            if (m_trackJudgmentTimer[i] > 0.0 && !m_trackJudgmentText[i].empty()) {
                sf::Text judgmentText(m_font, m_trackJudgmentText[i], 32);
                judgmentText.setFillColor(m_trackJudgmentColor[i]);

                sf::FloatRect bounds = judgmentText.getLocalBounds();
                judgmentText.setOrigin(sf::Vector2f(bounds.size.x / 2.0, bounds.size.y / 2.0));

                // 每个轨道判定显示在对应轨道的判定线上�?
                float trackCenterX = m_tracks[i].x + m_tracks[i].width / 2.0;
                judgmentText.setPosition(sf::Vector2f(trackCenterX, m_trackY - 60.0));

                // 向上飘动效果
                float progress = 1.0 - (m_trackJudgmentTimer[i] / 0.5);
                float offsetY = progress * 30.0;
                judgmentText.move(sf::Vector2f(0.0, -offsetY));

                m_window.draw(judgmentText);
            }
        }
    }

    sf::RenderWindow& m_window;
    sf::Font& m_font;
    Framework m_game;

    vector<TrackConfig> m_tracks;
    float m_trackStartX;
    float m_trackWidth;
    float m_trackGap;
    float m_trackY;
    float m_trackTop;
    float m_trackHeight;
    float m_noteSpeed;
    float m_visibleAhead;
    Phase m_phase;
    float m_countdownTimer;
    bool m_finished;

    float m_comboScaleTimer;
    float m_comboScale;

    // 每个轨道独立的判定显�?
    vector<float> m_trackJudgmentTimer;
    vector<string> m_trackJudgmentText;
    vector<sf::Color> m_trackJudgmentColor;
    double m_lastDisplayedJudgmentTime = -1.0;
};