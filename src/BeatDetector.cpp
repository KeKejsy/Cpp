#include "BeatDetector.h"
#include <SFML/Audio.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <queue>

BeatDetector::BeatDetector()
    : m_windowSize(1024)
    , m_historySize(43)
    , m_threshold(1.5f)
    , m_minInterval(0.2f) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

float BeatDetector::computeWindowEnergy(const int16_t* samples, int startIdx,
                                         int numSamples, int channelCount) {
    float energy = 0.0f;
    int totalSamples = numSamples * channelCount;
    for (int i = 0; i < totalSamples; i++) {
        float sample = static_cast<float>(samples[startIdx + i]) / 32768.0f;
        energy += sample * sample;
    }
    return energy / numSamples;
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

    // 加载音频文件
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(audioFilePath)) {
        std::cerr << "[BeatDetector] 无法加载音频文件: " << audioFilePath << std::endl;
        return chart;
    }

    // 获取音频信息
    const int16_t* samples = buffer.getSamples();
    int64_t totalSampleFrames = buffer.getSampleCount();  // 总采样帧数（每帧含所有声道）
    unsigned int sampleRate = buffer.getSampleRate();
    unsigned int channelCount = buffer.getChannelCount();

    if (!samples || totalSampleFrames <= 0 || sampleRate <= 0) {
        std::cerr << "[BeatDetector] 音频数据无效" << std::endl;
        return chart;
    }

    chart.duration = static_cast<double>(totalSampleFrames) / sampleRate;

    std::cout << "[BeatDetector] 音频加载成功" << std::endl;
    std::cout << "  采样率: " << sampleRate << " Hz" << std::endl;
    std::cout << "  声道数: " << channelCount << std::endl;
    std::cout << "  总帧数: " << totalSampleFrames << std::endl;
    std::cout << "  时长: " << chart.duration << " 秒" << std::endl;

    // 节拍检测：滑动窗口能量比较
    std::vector<float> energyHistory;
    energyHistory.reserve(m_historySize);
    float historySum = 0.0f;

    std::vector<float> beatTimes;  // 检测到的节拍时间点
    float lastBeatTime = -1.0f;    // 上一次节拍的时间

    int numWindows = static_cast<int>(totalSampleFrames / m_windowSize);

    for (int w = 0; w < numWindows; w++) {
        int startIdx = w * m_windowSize * channelCount;
        float energy = computeWindowEnergy(samples, startIdx, m_windowSize, channelCount);

        // 计算历史平均能量
        float avgEnergy = 0.0f;
        if (!energyHistory.empty()) {
            avgEnergy = historySum / energyHistory.size();
        }

        // 节拍判定：当前能量 > 平均能量 * 阈值，且距上次节拍超过最小间隔
        float currentTime = static_cast<float>(w * m_windowSize) / sampleRate;

        if (energy > avgEnergy * m_threshold && !energyHistory.empty()) {
            if (currentTime - lastBeatTime >= m_minInterval) {
                beatTimes.push_back(currentTime);
                lastBeatTime = currentTime;
            }
        }

        // 更新滑动历史
        energyHistory.push_back(energy);
        historySum += energy;

        // 超出历史窗口大小，移除最早的
        if (static_cast<int>(energyHistory.size()) > m_historySize) {
            historySum -= energyHistory.front();
            energyHistory.erase(energyHistory.begin());
        }
    }

    std::cout << "[BeatDetector] 检测到 " << beatTimes.size() << " 个节拍" << std::endl;

    // 生成 Note 列表
    int lastLane = -1;
    for (size_t i = 0; i < beatTimes.size(); i++) {
        Note note;
        note.time = beatTimes[i];
        note.track = assignLane(lastLane);
        note.type = 0;
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
