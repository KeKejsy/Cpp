# 节奏大师 - C++ 大作业项目分工文档

## 项目概述

基于 C++ 和 SFML 3 实现一款类"节奏大师"的下落式音游。核心亮点：支持导入任意 MP3 文件，通过音频分析自动生成可玩的游戏谱面。

## 项目架构

```
┌─────────────┐    ┌──────────────┐    ┌─────────────┐
│  成员 C     │    │  成员 B      │    │  成员 A     │
│  谱面生成器  │───→│  游戏框架     │───→│  渲染 & UI   │
│             │    │              │    │             │
│ MP3→节拍    │    │ 主循环       │    │ SFML 绘制   │
│ 节拍→Note   │    │ 判定系统     │    │ 按键动画    │
│ 输出 Chart  │    │ 得分系统     │    │ 结算画面    │
└─────────────┘    │ Chart 数据结构│    └─────────────┘
                   └──────────────┘
```

## 技术栈

| 模块 | 库/工具 | 说明 |
|------|---------|------|
| 图形渲染 & 音频播放 | SFML 3 | 窗口管理、图形绘制、音乐播放、键盘输入 |
| MP3 解码 | miniaudio | 单头文件库，零依赖，将 MP3 解码为 PCM 数据 |
| 节拍检测 | 自实现 | 基于能量突变的 onset detection 算法 |
| 构建工具 | CMake + MSYS2 | MinGW-w64 工具链，pacman 管理依赖 |

## 核心数据结构（公共接口）

三个成员共享以下数据结构，**项目启动第一天必须确定，后续不得擅自修改**：

```cpp
struct Note {
    double time;   // 出现时间（秒），相对于歌曲开始
    int track;     // 轨道编号，0-3 对应 D/F/J/K 四个键
    int type;      // 0 = 单按，1 = 长按
    bool hit;      // 是否已被玩家击中
};

struct Chart {
    std::string songName;    // 歌曲名称
    double bpm;              // 检测到的 BPM
    double duration;         // 歌曲总时长（秒）
    std::vector<Note> notes; // 所有音符
};
```

## 成员分工

---

### 成员 A —— 前端渲染

**职责：用 SFML 把游戏画面绘制出来，实现玩家的视觉和交互体验。**

#### 需要完成的内容

1. **游戏主界面**
   - 创建 800×600 的 SFML 窗口
   - 绘制 4 条轨道的背景划分（建议每列使用不同颜色）
   - 底部绘制判定线（玩家需关注的目标区域）

2. **Note 下落动画**
   - 从屏幕顶部匀速下落至底部判定线
   - 下落速度需与歌曲时钟同步（由成员 B 提供时钟接口）

3. **按键反馈**
   - 检测 D/F/J/K 四个按键的按下事件
   - 按下时对应轨道底部显示按下效果（变色或闪光）

4. **判定显示**
   - 在判定线位置显示文字：Perfect / Great / Good / Miss
   - 可选：添加简单的缩放或淡出动画

5. **得分与结算画面**
   - 实时显示当前分数和连击数
   - 歌曲结束后显示结算统计（各判定数量、总分、最大连击）

#### 技术要点

- 使用 `sf::RenderWindow` 创建窗口，`setFramerateLimit(60)` 控制帧率
- Note 使用 `sf::RectangleShape` 绘制，矩形色块即可，不需要复杂贴图
- 按键使用 `sf::Keyboard::isKeyPressed` 实时轮询
- 使用 `sf::Clock` + `deltaTime` 控制动画速度，保证不同帧率下速度一致
- **A 只负责绘制，判定逻辑和游戏数据全部由成员 B 提供**

#### 预估代码量

400 - 500 行

---

### 成员 B —— 游戏框架

**职责：定义公共数据结构，实现游戏核心逻辑，将前端渲染与谱面生成模块整合。**

#### 需要完成的内容

1. **Chart 数据结构定义**
   - 定义 `Note` 和 `Chart` 结构体（见上方公共接口）
   - 这是三个人的协作契约，必须在第一天确定

2. **游戏主循环**
   - 管理游戏状态：主菜单 → 歌曲选择 → 游戏进行中 → 结算
   - 帧率控制与游戏时钟管理

3. **判定系统**
   - 玩家按键时，对比当前播放时间与最近 Note 的时间差：
     - ±50ms → Perfect（300 分）
     - ±100ms → Great（200 分）
     - ±150ms → Good（100 分）
     - 超过 150ms 或 Note 离开判定区域 → Miss（0 分，断连击）
   - 判定函数签名建议：
     ```cpp
     int judgeNote(double noteTime, double currentTime);
     // 返回值：3=Perfect, 2=Great, 1=Good, 0=Miss
     ```

4. **得分系统**
   - 实时累加分数
   - 统计连击数（Combo），Miss 时归零
   - 记录最大连击数

5. **模块整合**
   - 接收成员 C 生成的 Chart 数据
   - 将 Chart 中的 Note 信息传递给成员 A 进行渲染
   - 接收成员 A 的按键事件，驱动判定系统

#### 技术要点

- 游戏时钟使用 `music.getPlayingOffset().asSeconds()` 获取，**不要自己用 Clock 计时**，否则会和音频不同步
- 判定逻辑要处理"同一个 Note 不能判定两次"的情况，判定后将 `note.hit` 设为 true
- 主循环结构参考：
  ```cpp
  while (window.isOpen()) {
      // 处理事件
      // 更新游戏状态（调用判定）
      // 渲染（调用 A 的绘制函数）
  }
  ```

#### 预估代码量

300 - 400 行

---

### 成员 C —— 谱面生成

**职责：实现 MP3 音频解码与节拍检测，自动将任意音频文件转化为可玩的游戏谱面。**

#### 需要完成的内容

1. **MP3 解码**
   - 使用 miniaudio 将 MP3 文件解码为 PCM 采样数据
   - 获取采样率、声道数、总帧数等基本信息
   - 示例代码：
     ```cpp
     #define MINIAUDIO_IMPLEMENTATION
     #include "miniaudio.h"

     ma_decoder decoder;
     ma_decoder_init_file("song.mp3", NULL, &decoder);
     // 读取 PCM 数据到 float 数组
     std::vector<float> samples(totalFrames);
     ma_decoder_read_pcm_frames(&decoder, samples.data(), totalFrames, &framesRead);
     ma_decoder_uninit(&decoder);
     ```

2. **节拍检测（Onset Detection）**
   - 将 PCM 数据按固定窗口分帧（建议每帧 1024 个采样点）
   - 计算每帧的能量值（所有采样点的平方和）
   - 检测能量突变点作为节拍位置：
     ```cpp
     std::vector<double> detectBeats(const std::vector<float>& samples, int sampleRate) {
         std::vector<double> beats;
         const int frameSize = 1024;
         float prevEnergy = 0.0f;
         float threshold = 0.0f; // 需要根据歌曲动态调整

         for (int i = 0; i < samples.size() / frameSize; i++) {
             float energy = 0.0f;
             for (int j = 0; j < frameSize; j++) {
                 float s = samples[i * frameSize + j];
                 energy += s * s;
             }
             energy /= frameSize; // 取平均

             if (energy - prevEnergy > threshold && energy > threshold) {
                 double time = static_cast<double>(i * frameSize) / sampleRate;
                 beats.push_back(time);
             }
             prevEnergy = energy * 0.8f + prevEnergy * 0.2f; // 平滑
         }
         return beats;
     }
     ```

3. **谱面生成规则**
   - 将检测到的节拍点映射到 4 个轨道的 Note：
     ```cpp
     Chart generateChart(const std::vector<double>& beats, const std::string& songName, int sampleRate) {
         Chart chart;
         chart.songName = songName;
         chart.duration = /* 根据采样率和总帧数计算 */;

         for (int i = 0; i < beats.size(); i++) {
             Note note;
             note.time = beats[i];
             note.track = i % 4;         // 基础策略：轮流分配
             note.type = 0;
             note.hit = false;
             chart.notes.push_back(note);
         }
         return chart;
     }
     ```
   - 后续可优化分配策略（如根据能量大小决定单押/双押，根据节奏密度调整分布）

4. **BPM 估算（可选加分项）**
   - 统计相邻节拍间隔的中位数，反推 BPM
   - 用于辅助生成更合理的谱面

#### 技术要点

- miniaudio 是单头文件库，无需编译安装，直接 `#include` 即可。需要在**一个** `.cpp` 文件中定义 `MINIAUDIO_IMPLEMENTATION` 宏
- 能量阈值需要归一化处理，不同歌曲音量差异大，建议先对整首歌做一次扫描找到最大能量，再按比例设定阈值
- **节拍检测模块必须独立可测试**，不依赖 SFML 和游戏框架。单独写一个 `main` 函数验证：输入 MP3，输出检测到的节拍时间列表
- 检测结果建议同时输出到文本文件方便调试，每行一个时间戳

#### 预估代码量

300 - 400 行

---

## 协作流程

### 第一阶段：各自开发（第 1 周）

| 成员 | 任务 | 产出 |
|------|------|------|
| A | SFML 窗口 + Note 下落 + 按键检测 | 能看到方块下落并响应按键的窗口 |
| B | Chart 数据结构 + 判定逻辑 + 主循环 | 框架代码，能加载 Chart 并运行判定 |
| C | MP3 解码 + 节拍检测 + 谱面生成 | 能输出 Chart 数据或谱面文本文件 |

**本周目标：三个人各自模块独立跑通，不需要互相依赖。**

### 第二阶段：联调集成（第 2 周前半）

1. 成员 C 将 Chart 数据接入成员 B 的框架
2. 成员 B 将 Note 数据传递给成员 A 进行渲染
3. 打通完整链路：MP3 → 谱面生成 → 游戏渲染 → 按键判定 → 得分

### 第三阶段：测试优化（第 2 周后半）

- 调试节拍检测参数，使生成的谱面节奏合理
- 调试判定时机，保证音画同步
- UI 美化，修复 Bug

### 第四阶段：报告撰写（第 3 周）

- 撰写实验报告
- 准备答辩 PPT

## 注意事项

1. **Chart 数据结构第一天定好，后续不得擅自修改。** 如需修改必须三人协商。改动数据结构意味着三个人的代码都要同步修改。
2. **成员 C 的节拍检测模块必须独立可测试。** 不要等游戏框架写好再测试，用单独的 `main` 函数验证检测结果。
3. **游戏时钟统一使用 SFML 音乐播放器的时间**（`music.getPlayingOffset()`），不要用 `sf::Clock` 自己计时，否则会和音频产生漂移。
4. **版本管理建议使用 Git。** 三个人各写各的模块，通过 Chart 结构体对接，冲突概率低。
5. 如果遇到问题及时沟通，不要闷头做到死胡同再反馈。

## 依赖库安装

### 环境准备（MSYS2）

```bash
# 1. 安装 MSYS2：https://www.msys2.org/
# 2. 打开 MSYS2 UCRT64 终端，执行：
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sfml
# 3. 将 C:\msys64\ucrt64\bin 加入 Windows PATH
```

### miniaudio

```bash
# 直接下载头文件放入项目
# https://github.com/mackron/miniaudio/blob/master/miniaudio.h
```

## 构建项目

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
./rhythm_master.exe
```
