#include "MapGenerator.h"
#include <SFML/Audio.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iomanip>
using namespace std;


// 频段边界：20-150, 150-500, 500-2000, 2000-8000 Hz
const float MapGenerator::BAND_EDGES[5] = {20, 150, 500, 2000, 8000};
const float MapGenerator::SUSTAIN_RATIO = 0.25;
const float MapGenerator::MIN_HOLD_DURATION = 0.25;
const float MapGenerator::ABSOLUTE_MIN_ENERGY = 0.0005;


#pragma region 构造&难度 
MapGenerator::MapGenerator(Difficulty difficulty)
    : m_windowSize(1024),
      m_historySize(45),
      m_threshold(2.0),
      m_minInterval(0.28),
      m_globalMinInterval(0.12),
      m_difficulty(Difficulty::Normal)
{
    setDifficulty(difficulty);
}

void MapGenerator::setDifficulty(Difficulty difficulty) 
{
    m_difficulty = difficulty;
    switch (m_difficulty) 
    {
        case Difficulty::Easy:
        {
            m_threshold = 3.0;
            m_minInterval = 0.45;
            m_historySize = 60;
            m_globalMinInterval = 0.20;
            break;
        }
        case Difficulty::Normal:
        {
            m_threshold = 2.5;
            m_minInterval = 0.28;
            m_historySize = 45;
            m_globalMinInterval = 0.12;
            break;
        }
        case Difficulty::Hard:
        {
            m_threshold = 2.0;
            m_minInterval = 0.18;
            m_historySize = 30;
            m_globalMinInterval = 0.08;
            break;
        }
    }
}
#pragma endregion


#pragma region FFT
void MapGenerator::fft(vector<float>& real, vector<float>& imag) {
    int n = real.size();
    if (n <= 1) return;

    // 位逆序重排
    for (int i = 1, j = 0; i < n; i++) 
    {
        int bit = n >> 1;
        for (; (j & bit) != 0; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) 
        {
            swap(real[i], real[j]);
            swap(imag[i], imag[j]);
        }
    }

    // 蝶形运算
    for (int len = 2; len <= n; len <<= 1) 
    {
        float angle = -2 * 3.14159265358979 / len;
        float wReal = cos(angle);
        float wImag = sin(angle);
        for (int i = 0; i < n; i += len) 
        {
            float curReal = 1;
            float curImag = 0;
            for (int j = 0; j < len / 2; j++) 
            {
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
#pragma endregion


#pragma region Hann窗
void MapGenerator::applyHannWindow(vector<float>& samples) 
{
    int n = samples.size();
    for (int i = 0; i < n; i++) 
    {
        float multiplier = 0.5 * (1.0 - cos(2 * 3.14159265358979 * i / (n - 1)));
        samples[i] *= multiplier;
    }
}
#pragma endregion


#pragma region 频段能量计算
void MapGenerator::getBandEnergies
(
    const vector<float>& real,
    const vector<float>& imag,
    float sampleRate,
    float bandEnergies[4]
) 
{
    int n = real.size();
    float binWidth = sampleRate / n;

    for (int b = 0; b < 4; b++)
     {
        bandEnergies[b] = 0.0;
        float lowFreq = BAND_EDGES[b];
        float highFreq = BAND_EDGES[b + 1];

        int startBin = (int)(lowFreq / binWidth);
        if (startBin < 1) 
        {
            startBin = 1;
        }
        int endBin = (int)(highFreq / binWidth);
        if (endBin > n / 2) 
        {
            endBin = n / 2;
        }

        for (int i = startBin; i <= endBin; i++) 
        {
            float mag2 = real[i] * real[i] + imag[i] * imag[i];
            bandEnergies[b] += mag2;
        }

        int binCount = endBin - startBin + 1;
        if (binCount > 0) 
        {
            bandEnergies[b] /= binCount;
        }
    }
}
#pragma endregion


#pragma region 立体声转换
int MapGenerator::translate
(
    const short* buffer, 
    int sampleCount,
    int channelCount, 
    vector<float>& result
) 
{
    int finalCount = sampleCount / channelCount;
    result.resize(finalCount);

    for (int i = 0; i < finalCount; i++)
    {
        float sum = 0;
        for (int j = 0; j < channelCount; j++) 
        {
            sum += buffer[i * channelCount + j] / 32768;
        }
        result[i] = sum / channelCount;
    }

    return finalCount;
}
#pragma endregion


#pragma region Hold处理
void MapGenerator::finalizeHold(BandState& st, float endTime)
{
    float holdDuration = endTime - st.holdStartTime;

    st.beatTimes.push_back(st.holdStartTime);

    if (holdDuration >= MIN_HOLD_DURATION)
    {
        st.beatDurations.push_back(holdDuration);
    }
    else
    {
        st.beatDurations.push_back(0.0f);
    }
}


void MapGenerator::startHold(BandState& st, float currTime, float energy)
{
    st.inHold = true;
    st.holdStartTime = currTime;
    st.holdStartEnergy = energy;
    st.lastBeatTime = currTime;
    st.waitingReset = true;
}
#pragma endregion


#pragma region 谱面生成
Chart MapGenerator::generate(const string& audioFilePath) 
{
    Chart chart;
    chart.songName = audioFilePath;
    chart.noteDensity = 0;
    chart.duration = 0;

   
    sf::InputSoundFile file;
    if (!file.openFromFile(audioFilePath)) 
    {
        cout << "谱面生成器无法打开音频文件: " << audioFilePath << endl;
        return chart;
    }

    long long totalFrames = file.getSampleCount();
    unsigned int sampleRate = file.getSampleRate();
    unsigned int channelCount = file.getChannelCount();

    if (totalFrames <= 0 || sampleRate <= 0)
    {
        cout << "音频数据无效" << endl;
        return chart;
    }

    chart.duration = (double)totalFrames / sampleRate;

    cout << "音频加载成功" << endl;
    cout << "  采样率: " << sampleRate << " Hz" << endl;
    cout << "  声道数: " << channelCount << endl;
    cout << "  时长: " << chart.duration << " 秒" << endl;
    cout << "  难度: "
              << (m_difficulty == Difficulty::Easy ? "Easy" :
                  m_difficulty == Difficulty::Normal ? "Normal" : "Hard")
              << " (检测阈值=" << m_threshold
              << ", 单轨最小间隔=" << m_minInterval << "s"
              << ", 全局最小间隔=" << m_globalMinInterval << "s)"
              << endl;

    //读取音频

    vector<short> allSamples(totalFrames * channelCount);
    long long actuallyRead = file.read(allSamples.data(), totalFrames * channelCount);

    //转换为单声道
    vector<float> mono;
    int frameCount = translate(allSamples.data(), actuallyRead, channelCount, mono);

    cout << "谱面生成器读取完成，共 " << frameCount << " 帧" << endl;

    
    BandState bands[4];

   
    vector<float> fftReal(m_windowSize);
    vector<float> fftImag(m_windowSize);

    int stepSize = m_windowSize / 2;  
    int windowIndex = 0;

    cout << "谱面生成器开始谱通量分析 (重叠率50％)..." << endl;

    for (int start = 0; start + m_windowSize <= frameCount; start += stepSize) 
    {
       
        copy(mono.begin() + start, mono.begin() + start + m_windowSize, fftReal.begin());
        fill(fftImag.begin(), fftImag.end(), 0);

        
        applyHannWindow(fftReal);
        fft(fftReal, fftImag);

  
        float bandEnergies[4];
        getBandEnergies(fftReal, fftImag, sampleRate, bandEnergies);

        
        float currTime = (float)(start + m_windowSize / 2) / sampleRate;

        // 谱通量 + 局部峰值检测
        for (int b = 0; b < 4; b++) 
        {
            BandState& st = bands[b];
            float energy = bandEnergies[b];

            
            float flux = energy - st.lastEnergy;
            if (flux < 0) 
            {
                flux = 0;
            }
            st.lastEnergy = energy;

            
            st.fluxHistory.push_back(flux);
            st.fluxHistorySum += flux;
            while (st.fluxHistory.size() > m_historySize) 
            {
                st.fluxHistorySum -= st.fluxHistory.front();
                st.fluxHistory.pop_front();
            }

            if (st.fluxHistory.size() < m_historySize) 
            {
                st.lastFlux = flux;
                continue;
            }


            float avgFlux = st.fluxHistorySum / st.fluxHistory.size();
            float dynamicFluxThreshold = avgFlux * m_threshold;
            if (dynamicFluxThreshold < ABSOLUTE_MIN_ENERGY) 
            {
                dynamicFluxThreshold = ABSOLUTE_MIN_ENERGY;
            }

            // Hold 持续判定：基于 onset 能量的衰减比例
            float sustainThreshold = st.holdStartEnergy * 0.15;
            if (sustainThreshold < ABSOLUTE_MIN_ENERGY) 
            {
                sustainThreshold = ABSOLUTE_MIN_ENERGY;
            }
            if (!st.inHold) 
            {
                sustainThreshold = 0;
            }

            

            // 触发一次后进入 waitingReset，直到 flux 降至阈值的 50% 以下
            if (st.waitingReset && flux < dynamicFluxThreshold * 0.5) 
            {
                st.waitingReset = false;
            }



            // 节拍判定
            bool isRising = (flux >= st.lastFlux);
            st.lastFlux = flux;

            if (flux > dynamicFluxThreshold
                && !st.waitingReset
                && isRising
                && currTime - st.lastBeatTime >= m_minInterval)
            {
                // 如果之前已经有 Hold，先结束它
                if (st.inHold)
                {
                    finalizeHold(st, currTime);
                }
            
                // 开始新的 Hold
                startHold(st, currTime, energy);
            }
            
            // Hold 结束判定（能量衰减到起始能量的15%以下）
            if (st.inHold && energy < sustainThreshold)
            {
                finalizeHold(st, currTime);
                st.inHold = false;
            }
        }

        // 进度（约每 2 秒打印一次）
        if (windowIndex % 200 == 0) 
        {
            float af[4] = {0, 0, 0, 0};
            for (int b = 0; b < 4; b++) 
            {
                int sz = bands[b].fluxHistory.size();
                if (sz > 0) af[b] = bands[b].fluxHistorySum / sz;
            }
            cout << "  [" << windowIndex << "] t=" << currTime << "s"
                      << " avgFlux=[" << af[0] << "," << af[1] << "," << af[2] << "," << af[3] << "]"
                      << endl;
        }

        windowIndex++;
    }

    // 处理循环结束时仍在 hold 的频段
    float endTime = (float)frameCount / sampleRate;
    for (int b = 0; b < 4; b++) 
    {
        BandState& st = bands[b];
        if (st.inHold) 
        {
            float holdDuration = endTime - st.holdStartTime;
            if (holdDuration >= MIN_HOLD_DURATION) 
            {
                st.beatTimes.push_back(st.holdStartTime);
                st.beatDurations.push_back(holdDuration);
            } 
            else 
            {
                st.beatTimes.push_back(st.holdStartTime);
                st.beatDurations.push_back(0.0);
            }
        }
    }

    // ---- 统计 ----
    int totalBeats = 0;
    int totalHolds = 0;
    for (int b = 0; b < 4; b++) 
    {
        totalBeats += bands[b].beatTimes.size();
        for (size_t j = 0; j < bands[b].beatDurations.size(); j++) 
        {
            if (bands[b].beatDurations[j] > 0.0) 
            {
                totalHolds++;
            }
        }
    }

    cout << "[MapGenerator] 分析完成" << endl;
    cout << "  Band 0 (20-150Hz,   Bass):     " << bands[0].beatTimes.size() << " notes" << endl;
    cout << "  Band 1 (150-500Hz,  Low-Mid):  " << bands[1].beatTimes.size() << " notes" << endl;
    cout << "  Band 2 (500-2000Hz, High-Mid): " << bands[2].beatTimes.size() << " notes" << endl;
    cout << "  Band 3 (2000-8000Hz,High):     " << bands[3].beatTimes.size() << " notes" << endl;
    cout << "  Total: " << totalBeats << " notes (" << totalHolds << " holds)" << endl;

    // ---- 生成 Note 列表 ----
    struct NoteCandidate 
    {
        float time;
        float duration;
        int track;
    };
    vector<NoteCandidate> candidates;

    for (int b = 0; b < 4; b++) 
    {
        const BandState& st = bands[b];
        for (size_t i = 0; i < st.beatTimes.size(); i++) 
        {
            candidates.push_back({st.beatTimes[i], st.beatDurations[i], b});
        }
    }

    // 按时间排序
    sort(candidates.begin(), candidates.end(),
              [](const NoteCandidate& a, const NoteCandidate& b) 
              {
                  return a.time < b.time;
              });



    // 跨轨道限制最小间隔，防止四个轨道交替形成连续连打
    // 同时刻音符（差值≈0）视为双押/多押，放行且不更新时间基准
    float lastGlobalNoteTime = -1;
    for (size_t i = 0; i < candidates.size(); i++) 
    {
        if (candidates[i].time < 0) 
        {
            continue;
        }
        float gap = candidates[i].time - lastGlobalNoteTime;
        if (lastGlobalNoteTime >= 0 && gap > 0 && gap < m_globalMinInterval) 
        {
            // 严格晚于上一音符但间隔不足 → 删除
            candidates[i].time = -1;
        } 
        else if (gap > 0) 
        {
            // 正常间隔 → 保留并更新时间基准
            lastGlobalNoteTime = candidates[i].time;
        }
        // gap == 0: 同时刻双押/多押 → 保留，不更新时间基准
    }



#pragma region 轨道分配
    const float MIN_GAP = 0.1;
    const int MAX_CONSECUTIVE = 3;

    float trackFreeUntil[4] = {0, 0, 0, 0};
    int trackConsecutive[4] = {0, 0, 0, 0};

    for (size_t i = 0; i < candidates.size(); i++) 
    {
        NoteCandidate& c = candidates[i];
        if (c.time < 0) 
        {
            continue;
        }

        float duration = c.duration;
        if (duration < 0.03) 
        {
            duration = 0.03;
        }
        float noteEnd = c.time + duration + MIN_GAP;
        int preferredTrack = c.track;

        bool overlapOnPreferred = (c.time < trackFreeUntil[preferredTrack]);
        bool tooManyConsecutive = (trackConsecutive[preferredTrack] >= MAX_CONSECUTIVE - 1);

        if (overlapOnPreferred || tooManyConsecutive) 
        {
            int bestTrack = -1;
            int bestScore = 999;

            for (int t = 0; t < 4; t++) 
            {
                if (c.time >= trackFreeUntil[t]) 
                {
                    int score = trackConsecutive[t] * 2 + abs(t - preferredTrack);
                    if (score < bestScore) 
                    {
                        bestScore = score;
                        bestTrack = t;
                    }
                }
            }

            if (bestTrack >= 0) 
            {
                c.track = bestTrack;
            } 
            else if (overlapOnPreferred) 
            {
                c.time = -1;
                continue;
            }
        }

        int chosenTrack = c.track;

        trackFreeUntil[chosenTrack] = noteEnd;

        for (int t = 0; t < 4; t++) 
        {
            if (t == chosenTrack) 
            {
                trackConsecutive[t]++;
            }
            else 
            {
                trackConsecutive[t] = 0;
            }
        }
    }

    // 统计
    int trackNoteCount[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < candidates.size(); i++) 
    {
        if (candidates[i].time >= 0) 
        {
            trackNoteCount[candidates[i].track]++;
        }
    }
    cout << "[MapGenerator] final: T0=" << trackNoteCount[0]
              << " T1=" << trackNoteCount[1]
              << " T2=" << trackNoteCount[2]
              << " T3=" << trackNoteCount[3] << endl;

#pragma endregion



#pragma region 生成Note列表
    for (size_t i = 0; i < candidates.size(); i++) 
    {
        if (candidates[i].time < 0) 
        {
            continue;
        }

        Note note;
        note.time = candidates[i].time;
        note.track = candidates[i].track;
        note.duration = candidates[i].duration;
        note.type = (candidates[i].duration > 0) ? 1 : 0;
        note.hit = false;
        note.hitTime = 0;
        chart.notes.push_back(note);
    }
#pragma endregion



#pragma region 音符密度计算
    if (!chart.notes.empty()) 
    {
        chart.noteDensity = chart.notes.size() / chart.duration * 60;
        cout << "[MapGenerator] 最终谱面: " << chart.notes.size()
                  << " 个音符, Note Density: " << fixed << setprecision(1)
                  << chart.noteDensity << " NPM" << endl;
    }
    else 
    {
        cout << "[MapGenerator] 未检测到节拍，请尝试调低难度" << endl;
    }
#pragma endregion

#pragma endregion

    return chart;
}



