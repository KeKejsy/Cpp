#include "GameFramework.h"
#include <cmath>
using namespace std;

GameFramework::GameFramework()
    : m_state(GameState::Menu)
    , m_latestJudgment(Judgment::None)
    , m_latestJudgmentTime(0.0)
    , m_latestJudgmentTrack(-1)
    , m_keyPressed(4, false)
    , m_testTime(-1.0)
    , m_activeHoldIndex(4, -1)
    , m_holdJudgment(4, 0) {
    resetStats();
}

GameFramework::~GameFramework() = default;

void GameFramework::loadChart(const Chart& chart, const string& musicPath) {
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
                note.hitTime = currentTime;
                updateStats(m_holdJudgment[track]);
                m_latestJudgmentTrack = track;
                m_latestJudgmentTime = currentTime;
                m_activeHoldIndex[track] = -1;
                m_holdJudgment[track] = 0;
            }
        }
    }

    // auto-miss：未被按下的音符超时自动 miss
    for (auto& note : m_chart.notes) {
        if (!note.hit) {
            if (note.type == 1 && note.duration > 0.0) {
                // hold 音符：尾部到达判定线时才 miss
                // 安全检查：如果正被按住，由 auto-complete 处理，这里不抢
                if (currentTime >= note.time + note.duration) {
                    int noteIdx = static_cast<int>(&note - &m_chart.notes[0]);
                    bool isActive = false;
                    for (int t = 0; t < 4; t++) {
                        if (m_activeHoldIndex[t] == noteIdx) {
                            isActive = true;
                            break;
                        }
                    }
                    if (!isActive) {
                        note.hit = true;
                        note.hitTime = currentTime;
                        updateStats(0);
                        m_latestJudgmentTrack = note.track;
                        m_latestJudgmentTime = currentTime;
                    }
                }
            } else {
                // tap 音符：超出判定窗口 miss（250ms = 37.5px，明显超出 Good 窗口）
                if (currentTime - note.time > 0.250) {
                    note.hit = true;
                    note.hitTime = currentTime;
                    updateStats(0);
                    m_latestJudgmentTrack = note.track;
                    m_latestJudgmentTime = currentTime;
                }
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
    const double MAX_SEARCH_WINDOW = 0.28;  // 只搜 ±280ms 内的音符（略大于判定窗口）

    for (auto& note : m_chart.notes) {
        if (note.hit || note.track != track) {
            continue;
        }

        double diff = abs(currentTime - note.time);
        if (diff < closestTime && diff <= MAX_SEARCH_WINDOW) {
            closestTime = diff;
            closestNote = &note;
        }
    }

    if (!closestNote) {
        // 轨道上附近无音符，空按惩罚
        updateStats(0);
        m_latestJudgmentTrack = track;
        m_latestJudgmentTime = currentTime;
        return;
    }

    int judgment = judgeNote(closestNote->time, currentTime);
    if (judgment < 0) {
        // 超出判定窗口：空按惩罚，同时标记音符避免 auto-miss 双重扣分
        closestNote->hit = true;
        closestNote->hitTime = currentTime;
        updateStats(0);
        m_latestJudgmentTrack = track;
        m_latestJudgmentTime = currentTime;
        return;
    }

    if (closestNote->type == 1 && closestNote->duration > 0.0) {
        // hold 音符：按下时标记为正在按住，暂不结算
        m_activeHoldIndex[track] = static_cast<int>(closestNote - &m_chart.notes[0]);
        m_holdJudgment[track] = judgment;
    } else {
        // tap 音符：直接结算
        closestNote->hit = true;
        closestNote->hitTime = currentTime;
        updateStats(judgment);
        m_latestJudgmentTrack = track;
        m_latestJudgmentTime = currentTime;
    }
}

void GameFramework::handleKeyRelease(int track) {
    m_keyPressed[track] = false;

    // 检查该轨道是否有活跃的 hold 音符
    int holdIdx = m_activeHoldIndex[track];
    if (holdIdx >= 0) {
        Note& note = m_chart.notes[holdIdx];

        // 防御：如果 auto-complete 已经处理过，跳过
        if (note.hit) {
            m_activeHoldIndex[track] = -1;
            m_holdJudgment[track] = 0;
            return;
        }
        double currentTime = getMusicTime();
        double holdEndTime = note.time + note.duration;
        double heldDuration = currentTime - note.time;
        double totalDuration = note.duration;

        // 接近结尾（0.10s 窗口）→ 完整分数，使用初始按下时的判定
        if (currentTime >= holdEndTime - 0.10) {
            note.hit = true;
            note.hitTime = currentTime;
            updateStats(m_holdJudgment[track]);
            m_latestJudgmentTrack = track;
            m_latestJudgmentTime = currentTime;
        }
        // 按住超过 60% 时长 → 部分分数（Good），保留连击
        else if (totalDuration > 0.0 && heldDuration >= totalDuration * 0.6) {
            note.hit = true;
            note.hitTime = currentTime;
            updateStats(1);  // Good — 惩罚提前松手但非完全失败
            m_latestJudgmentTrack = track;
            m_latestJudgmentTime = currentTime;
        }
        // 按住不足 60% → Miss
        else {
            note.hit = true;
            note.hitTime = currentTime;
            updateStats(0);
            m_latestJudgmentTrack = track;
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
    double diff = abs(currentTime - noteTime);

    // 浮点精度容差（1e-9），避免边界值误判
    const double eps = 1e-9;

    // 判定窗口（毫秒 -> 像素距离 @150px/s）：
    // Perfect: 60ms / 9px   Great: 130ms / 19.5px   Good: 200ms / 30px
    // 窗口略大于音符高度(30px)，确保视觉上重叠时能命中
    if (diff <= 0.060 + eps) {
        m_latestJudgment = Judgment::Perfect;
        return 3;
    } else if (diff <= 0.130 + eps) {
        m_latestJudgment = Judgment::Great;
        return 2;
    } else if (diff <= 0.200 + eps) {
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
            m_latestJudgment = Judgment::Perfect;
            break;
        case 2:
            m_stats.greatCount++;
            m_stats.totalScore += 200;
            m_stats.combo++;
            m_latestJudgment = Judgment::Great;
            break;
        case 1:
            m_stats.goodCount++;
            m_stats.totalScore += 100;
            m_stats.combo++;
            m_latestJudgment = Judgment::Good;
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
    m_latestJudgmentTrack = -1;
    m_activeHoldIndex = vector<int>(4, -1);
    m_holdJudgment = vector<int>(4, 0);
}

bool GameFramework::isNoteBeingHeld(int noteIndex) const {
    for (int i = 0; i < 4; i++) {
        if (m_activeHoldIndex[i] == noteIndex) return true;
    }
    return false;
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
