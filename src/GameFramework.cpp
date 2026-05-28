#include "GameFramework.h"
#include <cmath>

GameFramework::GameFramework()
    : m_state(GameState::Menu)
    , m_latestJudgment(Judgment::None)
    , m_latestJudgmentTime(0.0)
    , m_keyPressed(4, false)
    , m_testTime(-1.0) {
    resetStats();
}

GameFramework::~GameFramework() = default;

void GameFramework::loadChart(const Chart& chart, const std::string& musicPath) {
    m_chart = chart;
    if (!musicPath.empty()) {
        m_music.openFromFile(musicPath);
    }
}

void GameFramework::startGame() {
    resetStats();
    m_state = GameState::Playing;
    m_music.play();
}

void GameFramework::update(double currentTime) {
    if (m_state != GameState::Playing) {
        return;
    }

    // 检查是否有音乐文件加载
    bool hasMusic = m_music.getDuration().asSeconds() > 0;
    
    // 如果有音乐但未播放，检查是否播放完毕
    if (hasMusic && m_music.getStatus() != sf::SoundSource::Status::Playing) {
        endGame();
        return;
    }

    for (auto& note : m_chart.notes) {
        if (!note.hit && currentTime - note.time > 0.15) {
            note.hit = true;
            updateStats(0);
        }
    }
}

void GameFramework::handleKeyPress(int track) {
    if (m_state != GameState::Playing || m_keyPressed[track]) {
        return;
    }

    m_keyPressed[track] = true;
    double currentTime = getMusicTime();

    Note* closestNote = nullptr;
    double closestTime = 1000.0;

    for (auto& note : m_chart.notes) {
        if (note.hit || note.track != track) {
            continue;
        }

        double diff = std::abs(currentTime - note.time);
        if (diff < closestTime) {
            closestTime = diff;
            closestNote = &note;
        }
    }

    if (closestNote) {
        int judgment = judgeNote(closestNote->time, currentTime);
        if (judgment >= 0) {
            closestNote->hit = true;
            updateStats(judgment);
            m_latestJudgmentTime = currentTime;
        }
    }
}

void GameFramework::handleKeyRelease(int track) {
    m_keyPressed[track] = false;
}

void GameFramework::endGame() {
    m_music.stop();
    m_state = GameState::Result;
}

int GameFramework::judgeNote(double noteTime, double currentTime) {
    double diff = std::abs(currentTime - noteTime);

    // 浮点精度容差（1e-9），避免 0.10 被判为 > 0.10 的情况
    const double eps = 1e-9;

    if (diff <= 0.05 + eps) {
        m_latestJudgment = Judgment::Perfect;
        return 3;
    } else if (diff <= 0.10 + eps) {
        m_latestJudgment = Judgment::Great;
        return 2;
    } else if (diff <= 0.15 + eps) {
        m_latestJudgment = Judgment::Good;
        return 1;
    }

    return -1;
}

void GameFramework::updateStats(int judgment) {
    switch (judgment) {
        case 3:
            m_stats.perfectCount++;
            m_stats.totalScore += 300;
            m_stats.combo++;
            break;
        case 2:
            m_stats.greatCount++;
            m_stats.totalScore += 200;
            m_stats.combo++;
            break;
        case 1:
            m_stats.goodCount++;
            m_stats.totalScore += 100;
            m_stats.combo++;
            break;
        case 0:
            m_stats.missCount++;
            m_stats.combo = 0;
            m_latestJudgment = Judgment::Miss;
            break;
    }

    if (m_stats.combo > m_stats.maxCombo) {
        m_stats.maxCombo = m_stats.combo;
    }
}

void GameFramework::resetStats() {
    m_stats.perfectCount = 0;
    m_stats.greatCount = 0;
    m_stats.goodCount = 0;
    m_stats.missCount = 0;
    m_stats.totalScore = 0;
    m_stats.combo = 0;
    m_stats.maxCombo = 0;
    m_latestJudgment = Judgment::None;
    m_latestJudgmentTime = 0.0;
}

double GameFramework::getMusicTime() const {
    if (m_testTime >= 0.0) {
        return m_testTime;
    }
    return m_music.getPlayingOffset().asSeconds();
}

void GameFramework::setTestTime(double time) {
    m_testTime = time;
}
