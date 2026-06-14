#include "BeatDetector.h"
#include <SFML/Audio.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
using namespace std;

// ---------------------------------------------------------------------------
// 构造 / 难度
// ---------------------------------------------------------------------------
BeatDetector::BeatDetector()
    : m_windowSize(1024)
    , m_historySize(43)
    , m_threshold(1.8f)
    , m_minInterval(0.22f)
    , m_difficulty(Difficulty::Normal) {
    srand(static_cast<unsigned int>(time(nullptr)));
}

BeatDetector::BeatDetector(Difficulty difficulty)
    : BeatDetector() {
    setDifficulty(difficulty);
}

void BeatDetector::setDifficulty(Difficulty difficulty) {
    m_difficulty = difficulty;
    applyDifficulty();
}

void BeatDetector::applyDifficulty() {
    switch (m_difficulty) {
        case Difficulty::Easy:
            m_threshold = 2.5f;
            m_minInterval = 0.35f;
            m_historySize = 55;       // 更长历史 → 更稳定平均值 → 更少节拍
            break;
        case Difficulty::Normal:
            m_threshold = 1.8f;
            m_minInterval = 0.22f;
            m_historySize = 43;
            break;
        case Difficulty::Hard:
            m_threshold = 1.2f;
            m_minInterval = 0.12f;
            m_historySize = 30;       // 更短历史 → 更敏感 → 更多节拍
            break;
    }
}

// ---------------------------------------------------------------------------
// FFT: radix-2 Cooley-Tukey, 迭代实现
// ---------------------------------------------------------------------------
void BeatDetector::fft(vector<float>& real, vector<float>& imag) {
    int n = static_cast<int>(real.size());
    if (n <= 1) return;

    // 位逆序重排
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; (j & bit) != 0; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            swap(real[i], real[j]);
            swap(imag[i], imag[j]);
        }
    }

    // 蝶形运算
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * 3.14159265358979f / len;
        float wReal = cos(angle);
        float wImag = sin(angle);
        for (int i = 0; i < n; i += len) {
            float curReal = 1.0f;
            float curImag = 0.0f;
            for (int j = 0; j < len / 2; j++) {
                int u = i + j;
                int v = i + j + len / 2;
                float tReal = curReal * real[v] - curImag * imag[v];
                float tImag = curReal * imag[v] + curImag * real[v];
                real[v] = real[u] - tReal;
                imag[v] = imag[u] - tImag;
                real[u] += tReal;
                imag[u] += tImag;
                float nextReal = curReal * wReal - curImag * wImag;
                curImag = curReal * wImag + curImag * wReal;
                curReal = nextReal;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Hann 窗
// ---------------------------------------------------------------------------
void BeatDetector::applyHannWindow(vector<float>& samples) {
    int n = static_cast<int>(samples.size());
    for (int i = 0; i < n; i++) {
        float multiplier = 0.5f * (1.0f - cos(2.0f * 3.14159265358979f * i / (n - 1)));
        samples[i] *= multiplier;
    }
}

// ---------------------------------------------------------------------------
// 频段能量计算
// ---------------------------------------------------------------------------
void BeatDetector::computeBandEnergies(const vector<float>& real,
                                        const vector<float>& imag,
                                        float sampleRate,
                                        float bandEnergies[4]) {
    int n = static_cast<int>(real.size());
    float binWidth = sampleRate / n;

    for (int b = 0; b < 4; b++) {
        bandEnergies[b] = 0.0f;
        float lowFreq = BAND_EDGES[b];
        float highFreq = BAND_EDGES[b + 1];

        int startBin = max(1, static_cast<int>(lowFreq / binWidth));
        int endBin = min(static_cast<int>(highFreq / binWidth), n / 2);

        for (int i = startBin; i <= endBin; i++) {
            float mag2 = real[i] * real[i] + imag[i] * imag[i];
            bandEnergies[b] += mag2;
        }

        // 归一化：除以频段覆盖的 bin 数，使得不同宽度的频段可比较
        int binCount = endBin - startBin + 1;
        if (binCount > 0) {
            bandEnergies[b] /= binCount;
        }
    }
}

// ---------------------------------------------------------------------------
// 立体声 → 单声道
// ---------------------------------------------------------------------------
int BeatDetector::convertToMono(const int16_t* buffer, int numSamples,
                                 int channelCount, vector<float>& monoOut) {
    int frameCount = numSamples / channelCount;
    monoOut.resize(frameCount);

    for (int i = 0; i < frameCount; i++) {
        float sum = 0.0f;
        for (int ch = 0; ch < channelCount; ch++) {
            sum += static_cast<float>(buffer[i * channelCount + ch]) / 32768.0f;
        }
        monoOut[i] = sum / channelCount;
    }

    return frameCount;
}

// ---------------------------------------------------------------------------
// 核心：谱面生成
// ---------------------------------------------------------------------------
Chart BeatDetector::generate(const string& audioFilePath) {
    Chart chart;
    chart.songName = audioFilePath;
    chart.bpm = 0.0;
    chart.duration = 0.0;

    // 打开音频文件
    sf::InputSoundFile file;
    if (!file.openFromFile(audioFilePath)) {
        cerr << "[BeatDetector] 无法打开音频文件: " << audioFilePath << endl;
        return chart;
    }

    int64_t totalFrames = file.getSampleCount();
    unsigned int sampleRate = file.getSampleRate();
    unsigned int channelCount = file.getChannelCount();

    if (totalFrames <= 0 || sampleRate <= 0) {
        cerr << "[BeatDetector] 音频数据无效" << endl;
        return chart;
    }

    chart.duration = static_cast<double>(totalFrames) / sampleRate;

    cout << "[BeatDetector] 音频加载成功" << endl;
    cout << "  采样率: " << sampleRate << " Hz" << endl;
    cout << "  声道数: " << channelCount << endl;
    cout << "  时长: " << chart.duration << " 秒" << endl;
    cout << "  难度: "
              << (m_difficulty == Difficulty::Easy ? "Easy" :
                  m_difficulty == Difficulty::Normal ? "Normal" : "Hard")
              << " (threshold=" << m_threshold << ", minInterval=" << m_minInterval << "s)"
              << endl;

    // 每个频段独立的状态
    BandState bands[4];

    // 读取缓冲区
    int samplesPerWindow = m_windowSize * channelCount;
    vector<int16_t> buffer(samplesPerWindow);
    vector<float> monoBuf;
    vector<float> fftReal(m_windowSize);
    vector<float> fftImag(m_windowSize);

    int64_t framesRead = 0;
    int windowIndex = 0;

    // 自适应 sustain 阈值：随音乐动态调整
    float globalAvgEnergy = 0.0f;
    int globalWindowCount = 0;

    cout << "[BeatDetector] 开始 FFT 频段分析..." << endl;

    while (framesRead < totalFrames) {
        int64_t read = file.read(buffer.data(), samplesPerWindow);
        if (read <= 0) break;

        int framesInChunk = convertToMono(buffer.data(), static_cast<int>(read),
                                          channelCount, monoBuf);
        framesRead += framesInChunk;

        float currentTime = static_cast<float>(framesRead) / sampleRate;

        // 如果最后一块不足 m_windowSize，用零填充
        if (framesInChunk < m_windowSize) {
            monoBuf.resize(m_windowSize, 0.0f);
        }

        // ---- FFT 分析 ----
        copy(monoBuf.begin(), monoBuf.end(), fftReal.begin());
        fill(fftImag.begin(), fftImag.end(), 0.0f);
        applyHannWindow(fftReal);
        fft(fftReal, fftImag);

        // 计算 4 个频段的能量
        float bandEnergies[4];
        computeBandEnergies(fftReal, fftImag, static_cast<float>(sampleRate), bandEnergies);

        // 更新全局平均能量（用于自适应 sustain）
        float totalEnergy = 0.0f;
        for (int b = 0; b < 4; b++) totalEnergy += bandEnergies[b];
        totalEnergy /= 4.0f;
        globalAvgEnergy = (globalAvgEnergy * globalWindowCount + totalEnergy) / (globalWindowCount + 1);
        globalWindowCount++;

        // ---- 每个频段独立检测节拍 ----
        for (int b = 0; b < 4; b++) {
            BandState& st = bands[b];
            float energy = bandEnergies[b];

            // 更新滑动历史
            st.energyHistory.push_back(energy);
            st.historySum += energy;
            while (static_cast<int>(st.energyHistory.size()) > m_historySize) {
                st.historySum -= st.energyHistory.front();
                st.energyHistory.pop_front();
            }

            // 历史不够 → 跳过
            if (static_cast<int>(st.energyHistory.size()) < m_historySize) continue;

            float avgEnergy = st.historySum / st.energyHistory.size();

            // 动态阈值：取相对阈值和绝对阈值中的较大者
            float dynamicThreshold = avgEnergy * m_threshold;
            if (dynamicThreshold < ABSOLUTE_MIN_ENERGY) {
                dynamicThreshold = ABSOLUTE_MIN_ENERGY;
            }

            // 自适应 sustain 阈值
            float sustainThreshold = max(ABSOLUTE_MIN_ENERGY * 0.5f,
                                               avgEnergy * SUSTAIN_RATIO);

            // ---- 节拍检测 ----
            if (energy > dynamicThreshold) {
                if (currentTime - st.lastBeatTime >= m_minInterval) {
                    if (st.inHold) {
                        // hold 期间能量再次突增 → 结束当前 hold，开始新的
                        float holdDuration = currentTime - st.holdStartTime;
                        if (holdDuration >= MIN_HOLD_DURATION) {
                            st.beatTimes.push_back(st.holdStartTime);
                            st.beatDurations.push_back(holdDuration);
                        } else {
                            st.beatTimes.push_back(st.holdStartTime);
                            st.beatDurations.push_back(0.0f);
                        }
                    }
                    // 开始新的 hold
                    st.inHold = true;
                    st.holdStartTime = currentTime;
                    st.lastBeatTime = currentTime;
                }
            }

            // ---- hold 结束判定 ----
            if (st.inHold && energy < sustainThreshold) {
                float holdDuration = currentTime - st.holdStartTime;
                if (holdDuration >= MIN_HOLD_DURATION) {
                    st.beatTimes.push_back(st.holdStartTime);
                    st.beatDurations.push_back(holdDuration);
                } else {
                    st.beatTimes.push_back(st.holdStartTime);
                    st.beatDurations.push_back(0.0f);
                }
                st.inHold = false;
            }
        }

        // 进度
        if (windowIndex % 5000 == 0) {
            cout << "  [" << windowIndex << "] t=" << currentTime << "s"
                      << " bands=[" << bandEnergies[0] << "," << bandEnergies[1]
                      << "," << bandEnergies[2] << "," << bandEnergies[3] << "]"
                      << endl;
        }

        windowIndex++;
    }

    // 处理循环结束时仍在 hold 的频段
    float endTime = static_cast<float>(framesRead) / sampleRate;
    for (int b = 0; b < 4; b++) {
        BandState& st = bands[b];
        if (st.inHold) {
            float holdDuration = endTime - st.holdStartTime;
            if (holdDuration >= MIN_HOLD_DURATION) {
                st.beatTimes.push_back(st.holdStartTime);
                st.beatDurations.push_back(holdDuration);
            } else {
                st.beatTimes.push_back(st.holdStartTime);
                st.beatDurations.push_back(0.0f);
            }
        }
    }

    // ---- 统计 ----
    int totalBeats = 0;
    int totalHolds = 0;
    for (int b = 0; b < 4; b++) {
        totalBeats += static_cast<int>(bands[b].beatTimes.size());
        for (float d : bands[b].beatDurations) {
            if (d > 0.0f) totalHolds++;
        }
    }

    cout << "[BeatDetector] 分析完成" << endl;
    cout << "  Band 0 (20-200Hz,   Bass):    " << bands[0].beatTimes.size() << " notes" << endl;
    cout << "  Band 1 (200-800Hz,  Low-Mid): " << bands[1].beatTimes.size() << " notes" << endl;
    cout << "  Band 2 (800-3000Hz, High-Mid):" << bands[2].beatTimes.size() << " notes" << endl;
    cout << "  Band 3 (3000+Hz,    High):    " << bands[3].beatTimes.size() << " notes" << endl;
    cout << "  Total: " << totalBeats << " notes (" << totalHolds << " holds)" << endl;

    // ---- 生成 Note 列表 ----
    // 每个频段的 beat 直接映射到对应轨道 (band index = track index)
    struct NoteCandidate {
        float time;
        float duration;
        int track;
    };
    vector<NoteCandidate> candidates;

    for (int b = 0; b < 4; b++) {
        const BandState& st = bands[b];
        for (size_t i = 0; i < st.beatTimes.size(); i++) {
            candidates.push_back({st.beatTimes[i], st.beatDurations[i], b});
        }
    }

    // 按时间排序
    sort(candidates.begin(), candidates.end(),
              [](const NoteCandidate& a, const NoteCandidate& b) {
                  return a.time < b.time;
              });

    // ---- Pass 1: 合并过于接近的跨轨道音符 ----
    // 同一音乐事件（如鼓点）可能同时激发多个频段 → 保留 duration 最长的
    for (size_t i = 0; i < candidates.size(); i++) {
        if (candidates[i].time < 0) continue;
        for (size_t j = i + 1; j < candidates.size(); j++) {
            if (candidates[j].time < 0) continue;
            if (candidates[j].time - candidates[i].time < 0.030f) {
                if (candidates[j].duration > candidates[i].duration) {
                    candidates[i].time = -1.0f;
                    break;
                } else {
                    candidates[j].time = -1.0f;
                }
            } else {
                break;
            }
        }
    }

    // ---- Pass 2: 统一轨道分配 ----
    // 约束 1：同一轨道上音符时间不能重叠（hold 期间不能有其他音符）
    // 约束 2：避免同一轨道连续出现太多音符（> MAX_CONSECUTIVE 个）
    //
    // 对于重叠的音符：优先换轨道，无可用的轨道则跳过该音符
    // 对于连续过多的音符：优先换轨道，无可用的轨道则允许继续（重叠 > 单调）

    constexpr float MIN_GAP = 0.06f;          // 同轨道相邻音符最小间隔
    constexpr int MAX_CONSECUTIVE = 3;        // 同轨道最大连续音符数（超过则尝试换轨道）

    float trackFreeUntil[4] = {0, 0, 0, 0};
    int trackConsecutive[4] = {0, 0, 0, 0};

    for (size_t i = 0; i < candidates.size(); i++) {
        NoteCandidate& c = candidates[i];
        if (c.time < 0) continue;

        float noteEnd = c.time + max(c.duration, 0.03f) + MIN_GAP;
        int preferredTrack = c.track;

        // 检查是否需要换轨道
        bool overlapOnPreferred = (c.time < trackFreeUntil[preferredTrack]);
        bool tooManyConsecutive = (trackConsecutive[preferredTrack] >= MAX_CONSECUTIVE - 1);

        if (overlapOnPreferred || tooManyConsecutive) {
            // 按优先级找替代轨道：
            //   情况 A（重叠）：必须换，无可用轨道则删除
            //   情况 B（连续过多）：尽量换，无可用轨道则留在原轨道
            int bestTrack = -1;
            int bestScore = 999;  // 低分 = 更好

            for (int t = 0; t < 4; t++) {
                if (c.time >= trackFreeUntil[t]) {
                    // 该轨道空闲，打分：consecutive 越少越好
                    int score = trackConsecutive[t] * 2 + abs(t - preferredTrack);
                    if (score < bestScore) {
                        bestScore = score;
                        bestTrack = t;
                    }
                }
            }

            if (bestTrack >= 0) {
                c.track = bestTrack;
            } else if (overlapOnPreferred) {
                // 所有轨道都被占用 → 只能跳过这个音符
                c.time = -1.0f;
                continue;
            }
            // 如果只是连续过多但无可用轨道 → 留在原轨道（可以接受）
        }

        int chosenTrack = c.track;

        // 更新轨道状态
        trackFreeUntil[chosenTrack] = noteEnd;

        for (int t = 0; t < 4; t++) {
            if (t == chosenTrack) trackConsecutive[t]++;
            else trackConsecutive[t] = 0;
        }
    }

    // 轨道分配统计
    int trackNoteCount[4] = {0, 0, 0, 0};
    for (const auto& c : candidates) {
        if (c.time >= 0) trackNoteCount[c.track]++;
    }
    cout << "[BeatDetector] final: T0=" << trackNoteCount[0]
              << " T1=" << trackNoteCount[1]
              << " T2=" << trackNoteCount[2]
              << " T3=" << trackNoteCount[3] << endl;

    // ---- 生成最终 Note 列表 ----
    for (const auto& c : candidates) {
        if (c.time < 0) continue;

        Note note;
        note.time = c.time;
        note.track = c.track;
        note.duration = c.duration;
        note.type = (c.duration > 0.0f) ? 1 : 0;
        note.hit = false;
        note.hitTime = 0.0;
        chart.notes.push_back(note);
    }

    // BPM 估算
    if (!chart.notes.empty()) {
        chart.bpm = static_cast<double>(chart.notes.size()) / chart.duration * 60.0;
        cout << "[BeatDetector] 最终谱面: " << chart.notes.size()
                  << " 个音符, 估算 BPM: " << chart.bpm << endl;
    } else {
        cout << "[BeatDetector] 未检测到节拍，请尝试调低难度" << endl;
    }

    return chart;
}
