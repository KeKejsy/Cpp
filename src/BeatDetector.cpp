#include "BeatDetector.h"
#include <SFML/Audio.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <deque>
#include <vector>

BeatDetector::BeatDetector()
    : m_windowSize(1024)
    , m_historySize(20)
    , m_threshold(1.2f)
    , m_minInterval(0.15f) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

int BeatDetector::assignLane(int lastLane) {
    int lane = std::rand() % 3;
    if (lane >= lastLane) {
        lane++;
    }
    return lane;
}

Chart BeatDetector::generate(const std::string& audioFilePath) {
    Chart chart;
    chart.songName = audioFilePath;
    chart.bpm = 0.0;
    chart.duration = 0.0;

    // 使用 InputSoundFile 逐块读取音频（避免一次性加载整个文件到内存）
    sf::InputSoundFile file;
    if (!file.openFromFile(audioFilePath)) {
        std::cerr << "[BeatDetector] 无法打开音频文件: " << audioFilePath << std::endl;
        return chart;
    }

    int64_t totalFrames = file.getSampleCount();   // 总采样帧数
    unsigned int sampleRate = file.getSampleRate();
    unsigned int channelCount = file.getChannelCount();

    if (totalFrames <= 0 || sampleRate <= 0) {
        std::cerr << "[BeatDetector] 音频数据无效" << std::endl;
        return chart;
    }

    chart.duration = static_cast<double>(totalFrames) / sampleRate;

    std::cout << "[BeatDetector] 音频加载成功" << std::endl;
    std::cout << "  采样率: " << sampleRate << " Hz" << std::endl;
    std::cout << "  声道数: " << channelCount << std::endl;
    std::cout << "  总帧数: " << totalFrames << std::endl;
    std::cout << "  时长: " << chart.duration << " 秒" << std::endl;

    // 分配读取缓冲区：每个窗口 = m_windowSize 帧 × channelCount 个采样点
    int samplesPerWindow = m_windowSize * channelCount;
    std::vector<int16_t> buffer(samplesPerWindow);

    // 节拍检测：滑动窗口能量比较
    std::deque<float> energyHistory;
    float historySum = 0.0f;

    std::vector<float> beatTimes;
    std::vector<float> beatDurations;  // 每个 beat 对应的持续时长（0 = tap, >0 = hold）
    float lastBeatTime = -1.0f;

    // hold 检测状态
    bool inHold = false;
    float holdStartTime = 0.0f;
    const float sustainThreshold = 0.002f;  // 能量持续阈值：低于此值认为声音结束
    const float minHoldDuration = 0.3f;     // 最短 hold 时长

    int64_t framesRead = 0;
    int windowIndex = 0;

    std::cout << "[BeatDetector] 开始分析..." << std::endl;

    while (framesRead < totalFrames) {
        // 读取一个窗口的采样数据
        int64_t read = file.read(buffer.data(), samplesPerWindow);
        if (read <= 0) break;

        int framesInChunk = static_cast<int>(read / channelCount);
        framesRead += framesInChunk;

        // 计算窗口能量（均方值，归一化到 [-1,1]）
        float energy = 0.0f;
        for (int i = 0; i < read; i++) {
            float sample = static_cast<float>(buffer[i]) / 32768.0f;
            energy += sample * sample;
        }
        energy /= framesInChunk;

        // 计算历史平均能量
        float avgEnergy = 0.0f;
        if (!energyHistory.empty()) {
            avgEnergy = historySum / energyHistory.size();
        }

        float currentTime = static_cast<float>(framesRead) / sampleRate;

        // 节拍判定：需要同时满足：
        // 1. 能量 > 平均能量 × 阈值（相对条件）
        // 2. 能量 > 绝对最小值（避免静音段误触发）
        // 3. 距上次节拍超过最小间隔
        const float absoluteMinEnergy = 0.001f;
        float dynamicThreshold = avgEnergy * m_threshold;
        if (dynamicThreshold < absoluteMinEnergy) {
            dynamicThreshold = absoluteMinEnergy;
        }

        if (!energyHistory.empty() && energy > dynamicThreshold) {
            if (currentTime - lastBeatTime >= m_minInterval) {
                // 检测到新的 beat
                if (inHold) {
                    // hold 期间能量再次突增 → 结束当前 hold，开始新的
                    float holdDuration = currentTime - holdStartTime;
                    if (holdDuration >= minHoldDuration) {
                        beatTimes.push_back(holdStartTime);
                        beatDurations.push_back(holdDuration);
                    } else {
                        beatTimes.push_back(holdStartTime);
                        beatDurations.push_back(0.0f);
                    }
                }
                // 标记进入新的 hold 状态
                inHold = true;
                holdStartTime = currentTime;
                lastBeatTime = currentTime;
            }
        }

        // hold 结束判定：在 hold 状态下，能量降到持续阈值以下
        if (inHold && energy < sustainThreshold) {
            float holdDuration = currentTime - holdStartTime;
            if (holdDuration >= minHoldDuration) {
                // 持续时间够长 → hold 音符
                beatTimes.push_back(holdStartTime);
                beatDurations.push_back(holdDuration);
            } else {
                // 短促声音 → tap 音符
                beatTimes.push_back(holdStartTime);
                beatDurations.push_back(0.0f);
            }
            inHold = false;
        }

        // 更新滑动历史
        energyHistory.push_back(energy);
        historySum += energy;

        if (static_cast<int>(energyHistory.size()) > m_historySize) {
            historySum -= energyHistory.front();
            energyHistory.pop_front();
        }

        // 进度输出（每 5000 个窗口）
        if (windowIndex % 5000 == 0) {
            std::cout << "  [" << windowIndex << "] t=" << currentTime
                      << "s energy=" << energy << std::endl;
        }

        windowIndex++;
    }

    // 处理循环结束时仍在 hold 状态的情况
    if (inHold) {
        float endTime = static_cast<float>(framesRead) / sampleRate;
        float holdDuration = endTime - holdStartTime;
        if (holdDuration >= minHoldDuration) {
            beatTimes.push_back(holdStartTime);
            beatDurations.push_back(holdDuration);
        } else {
            beatTimes.push_back(holdStartTime);
            beatDurations.push_back(0.0f);
        }
    }

    std::cout << "[BeatDetector] 分析完成, 检测到 " << beatTimes.size() << " 个节拍" << std::endl;

    // 统计 hold 音符数量
    int holdCount = 0;
    for (float d : beatDurations) {
        if (d > 0.0f) holdCount++;
    }
    std::cout << "[BeatDetector] 其中 hold 音符: " << holdCount << " 个" << std::endl;

    // 生成 Note 列表
    int lastLane = -1;
    for (size_t i = 0; i < beatTimes.size(); i++) {
        Note note;
        note.time = beatTimes[i];
        note.track = assignLane(lastLane);
        note.duration = beatDurations[i];
        note.type = (note.duration > 0.0f) ? 1 : 0;
        note.hit = false;
        lastLane = note.track;
        chart.notes.push_back(note);
    }

    if (!chart.notes.empty()) {
        chart.bpm = static_cast<double>(chart.notes.size()) / chart.duration * 60.0;
        std::cout << "[BeatDetector] 生成谱面完成: " << chart.notes.size()
                  << " 个音符, 估算 BPM: " << chart.bpm << std::endl;
    } else {
        std::cout << "[BeatDetector] 未检测到节拍，请尝试调低阈值参数" << std::endl;
    }

    return chart;
}
