#pragma once

#include "common.h"
#include <SFML/Audio.hpp>
#include <string>

enum class GameState {
    Menu,
    SongSelect,
    Playing,
    Result
};

enum class Judgment {
    Perfect,
    Great,
    Good,
    Miss,
    None
};

struct GameStats {
    int perfectCount;
    int greatCount;
    int goodCount;
    int missCount;
    int totalScore;
    int combo;
    int maxCombo;
};

class GameFramework {
public:
    GameFramework();
    ~GameFramework();

    void loadChart(const Chart& chart, const std::string& musicPath);
    void startGame();
    void update(double currentTime);
    void handleKeyPress(int track);
    void handleKeyRelease(int track);
    void endGame();

    const Chart& getChart() const { return m_chart; }
    const std::vector<Note>& getNotes() const { return m_chart.notes; }
    const GameStats& getStats() const { return m_stats; }
    GameState getState() const { return m_state; }
    Judgment getLatestJudgment() const { return m_latestJudgment; }
    double getLatestJudgmentTime() const { return m_latestJudgmentTime; }
    bool isPlaying() const { return m_state == GameState::Playing; }
    double getMusicTime() const;

    // 测试用：手动设置当前时间（不依赖音乐播放）
    void setTestTime(double time);

private:
    int judgeNote(double noteTime, double currentTime);
    void updateStats(int judgment);
    void resetStats();

    Chart m_chart;
    sf::Music m_music;
    GameState m_state;
    GameStats m_stats;
    Judgment m_latestJudgment;
    double m_latestJudgmentTime;
    std::vector<bool> m_keyPressed;
    double m_testTime;
};