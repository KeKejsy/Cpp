#pragma once

#include "common.h"
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
using namespace std;

// MapGenerator - 自动谱面生成器
// 基于 FFT 频段 Spectral Flux（频谱通量）分析：将音频按频率拆分为 4 个频段，
// 每个频段独立检测能量变化（而非能量大小），频段直接映射到游戏轨道
// （低频→轨道0, 中低频→轨道1, 中高频→轨道2, 高频→轨道3）
// Spectral Flux 突出鼓点、镲片、钢琴击键等瞬态，压制持续人声
class MapGenerator {
public:
    MapGenerator();
    MapGenerator(Difficulty difficulty);

    // 核心接口：传入音频文件路径，返回生成的 Chart
    Chart generate(const string& audioFilePath);

    // 运行时调整难度
    void setDifficulty(Difficulty difficulty);

private:
    // 频段状态：每个频段独立追踪 flux 历史、能量历史和 hold 状态
    struct BandState {
        deque<float> fluxHistory;       // flux = max(0, energy - lastEnergy)
        float fluxHistorySum = 0.0;     // flux 历史滑动和
        float lastEnergy = 0.0;         // 上一窗口能量，用于计算 flux
        bool inHold = false;
        float holdStartTime = 0.0;
        float holdStartEnergy = 0.0;    // 起音时能量，用于判定持续结束
        float lastBeatTime = -1.0;
        float lastFlux = 0.0;           // 上一窗口 flux，用于局部峰值检测
        bool waitingReset = false;      // 触发后等待 flux 降至阈值以下再重新启用
        vector<float> beatTimes;
        vector<float> beatDurations;
    };

    // FFT: in-place radix-2 Cooley-Tukey (迭代实现)
    void fft(vector<float>& real, vector<float>& imag);

    // 应用 Hann 窗，减少频谱泄漏
    void applyHannWindow(vector<float>& samples);

    // 从 FFT 结果计算 4 个频段的能量
    // bandEdges[5] = {low, midLow, midHigh, high, nyquist} 频段边界频率 (Hz)
    void computeBandEnergies(const vector<float>& real,
                              const vector<float>& imag,
                              float sampleRate,
                              float bandEnergies[4]);

    // 将立体声 int16 采样转换为单声道 float，返回有效采样数
    int convertToMono(const int16_t* buffer, int numSamples, int channelCount,
                      vector<float>& monoOut);

    // 根据难度设置参数
    void applyDifficulty();

    // ---- 可调参数 ----
    int m_windowSize;           // 窗口大小（采样点数），默认 1024
    int m_historySize;          // flux 历史窗口数，默认 43
    float m_threshold;          // flux 阈值倍数（相对 avgFlux）
    float m_minInterval;        // 单轨道最小节拍间隔（秒）
    float m_globalMinInterval;  // 全局最小音符间隔（秒），跨轨道限流
    Difficulty m_difficulty;

    // 频段边界 (Hz) — 4 个频段 → 4 个轨道
    // Band 0: 20-150 Hz   → Track 0 (D)  低频：底鼓、贝斯
    // Band 1: 150-500 Hz  → Track 1 (F)  中低频：钢琴低音、节奏吉他
    // Band 2: 500-2000 Hz → Track 2 (J)  中高频：人声、主旋律
    // Band 3: 2000-8000 Hz→ Track 3 (K)  高频：镲片、hi-hat
    static const float BAND_EDGES[5];

    // Hold 检测参数
    static const float SUSTAIN_RATIO;       // 持续阈值 = 平均能量 × 此值
    static const float MIN_HOLD_DURATION;   // 最短 hold 时长（秒）
    static const float ABSOLUTE_MIN_ENERGY; // 绝对最小能量/flux，避免静音误触
};
