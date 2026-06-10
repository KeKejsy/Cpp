#include "GameFramework.h"
#include <cmath>

GameFramework::GameFramework()
    : m_state(GameState::Menu)
    , m_latestJudgment(Judgment::None)
    , m_latestJudgmentTime(0.0)
    , m_keyPressed(4, false)
    , m_testTime(-1.0)
    , m_activeHoldIndex(4, -1)
    , m_holdJudgment(4, 0) {
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

    // 处理活跃的 hold 音符自动完成
    for (int track = 0; track < 4; track++) {
        int holdIdx = m_activeHoldIndex[track];
        if (holdIdx >= 0) {
            Note& note = m_chart.notes[holdIdx];
            double holdEndTime = note.time + note.duration;
            if (currentTime >= holdEndTime) {
                // hold 音符时间到，自动完成，按原判定给分
                note.hit = true;
                updateStats(m_holdJudgment[track]);
                m_latestJudgmentTime = currentTime;
                m_activeHoldIndex[track] = -1;
                m_holdJudgment[track] = 0;
            }
        }
    }

    // auto-miss：未被按下的音符超时自动 miss
    for (auto& note : m_chart.notes) {
        if (!note.hit) {
            // hold 音符用 duration 作为超时窗口，tap 用 0.15s
            double missWindow = (note.type == 1 && note.duration > 0.0) ? note.duration + 0.15 : 0.15;
            if (currentTime - note.time > missWindow) {
                note.hit = true;
                updateStats(0);
            }
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
            if (closestNote->type == 1 && closestNote->duration > 0.0) {
                // hold 音符：按下时标记为正在按住，暂不结算
                m_activeHoldIndex[track] = static_cast<int>(closestNote - &m_chart.notes[0]);
                m_holdJudgment[track] = judgment;
                m_latestJudgment = static_cast<Judgment>(3 - judgment);
                m_latestJudgmentTime = currentTime;
                // 不设 hit = true，等松开或自动结束时再设
            } else {
                // tap 音符：直接结算
                closestNote->hit = true;
                updateStats(judgment);
                m_latestJudgmentTime = currentTime;
            }
        } else {
            // 空按惩罚：按键离最近音符太远，算 Miss
            updateStats(0);
            m_latestJudgmentTime = currentTime;
        }
    } else {
        // 空按惩罚：轨道上没有未击中音符，算 Miss
        updateStats(0);
        m_latestJudgmentTime = currentTime;
    }
}

void GameFramework::handleKeyRelease(int track) {
    m_keyPressed[track] = false;

    // 检查该轨道是否有活跃的 hold 音符
    int holdIdx = m_activeHoldIndex[track];
    if (holdIdx >= 0) {
        Note& note = m_chart.notes[holdIdx];
        double currentTime = getMusicTime();
        double holdEndTime = note.time + note.duration;

        // 如果接近结束（0.05s 内），视为完整 hold
        if (currentTime >= holdEndTime - 0.05) {
            note.hit = true;
            updateStats(m_holdJudgment[track]);
            m_latestJudgmentTime = currentTime;
        } else {
            // 提前松开，算 Miss
            note.hit = true;
            updateStats(0);
            m_latestJudgmentTime = currentTime;
        }

        m_activeHoldIndex[track] = -1;
        m_holdJudgment[track] = 0;
    }
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
    m_activeHoldIndex = std::vector<int>(4, -1);
    m_holdJudgment = std::vector<int>(4, 0);
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
