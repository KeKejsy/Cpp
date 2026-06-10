#pragma once

#include <string>
#include <vector>

struct Note {
    double time;
    int track;
    int type;      // 0 = tap（单击）, 1 = hold（长按）
    bool hit;
    double duration; // hold 音符的持续时长（秒），tap 音符为 0
};

struct Chart {
    std::string songName;
    double bpm;
    double duration;
    std::vector<Note> notes;
};
