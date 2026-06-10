#pragma once

#include <string>
#include <vector>

enum class Difficulty {
    Easy,
    Normal,
    Hard
};

struct Note {
    double time;
    int track;
    int type;      // 0 = tap（单击）, 1 = hold（长按）
    bool hit;
    double duration;  // hold 音符的持续时长（秒），tap 音符为 0
    double hitTime;   // 被击中/miss 的时间，用于延迟消失
};

struct Chart {
    std::string songName;
    double bpm;
    double duration;
    std::vector<Note> notes;
};
