#pragma once

#include "common.h"
#include <cstdint>
#include <string>

// BeatDetector - 自动谱面生成器
// 基于能量的节拍检测算法：分析音频 PCM 数据，检测节奏点，生成 Note 列表
class BeatDetector {
public:
    BeatDetector();

    // 核心接口：传入音频文件路径，返回生成的 Chart
    Chart generate(const std::string& audioFilePath);

private:
    // 计算一个窗口的能量（均方值）
    // samples: PCM 采样数据指针
    // startIdx: 窗口起始采样索引
    // numSamples: 窗口内采样点数
    // channelCount: 声道数
    float computeWindowEnergy(const int16_t* samples, int startIdx,
                              int numSamples, int channelCount);

    // 分配轨道：随机但避免连续相同
    int assignLane(int lastLane);

    // 可调参数
    int m_windowSize;        // 窗口大小（采样点数），默认 1024
    int m_historySize;       // 历史窗口数，默认 43（约 1 秒）
    float m_threshold;       // 能量阈值倍数，默认 1.5
    float m_minInterval;     // 最小节拍间隔（秒），默认 0.2
};
