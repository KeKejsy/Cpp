#pragma once

#include <string>
#include <vector>

struct Note {
    double time;
    int track;
    int type;
    bool hit;
};

struct Chart {
    std::string songName;
    double bpm;
    double duration;
    std::vector<Note> notes;
};
