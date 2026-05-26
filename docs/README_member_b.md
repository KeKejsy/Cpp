# 节奏大师 - 成员 B 工作说明

## 已完成的工作

### 1. 数据结构定义 (`common.h`)
- `Note` 结构体：音符数据（时间、轨道、类型、是否击中）
- `Chart` 结构体：谱面数据（歌曲名、BPM、时长、音符列表）

### 2. 游戏框架类 (`GameFramework.h/cpp`)
- **游戏状态管理**：Menu → SongSelect → Playing → Result
- **判定系统**：
  - Perfect (±50ms)：+300分
  - Great (±100ms)：+200分
  - Good (±150ms)：+100分
  - Miss (>150ms)：+0分，断连击
- **得分系统**：实时分数、连击数、最大连击
- **音乐播放**：集成 SFML Music，支持 MP3 播放
- **按键处理**：D/F/J/K 对应轨道 0/1/2/3

## 文件结构

```
src/
├── common.h              # 公共数据结构（Note, Chart）
├── GameFramework.h       # 游戏框架类声明
├── GameFramework.cpp     # 游戏框架类实现
├── main.cpp              # 主程序入口
├── test_member_b.cpp     # 成员 B 完整功能测试
└── interactive_test.cpp  # 交互式测试（SFML 窗口）
```

## 如何构建和运行

### 使用 CMake + MSYS2

```bash
# 在项目根目录
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make

# 运行
./rhythm_master.exe
```

### 使用 VS Code

1. 打开项目文件夹
2. 按 `Ctrl+Shift+B` 编译项目（自动执行 cmake + build）
3. 按 `F5` 调试运行

## 核心功能测试

运行 `test_member_b.exe` 可以验证所有核心功能：
- 数据结构定义
- 谱面生成逻辑
- 判定系统精度
- 得分系统计算
- 游戏状态管理

## 待集成模块

### 成员 A（前端渲染）
- 创建 SFML 窗口
- 绘制轨道和音符下落
- 按键视觉反馈
- 得分和判定显示

### 成员 C（谱面生成）
- MP3 解码
- 节拍检测算法
- 自动生成谱面

## 接口说明

### 成员 A 需要调用的接口
```cpp
GameFramework game;

// 加载谱面
game.loadChart(chart, "music.mp3");

// 开始游戏
game.startGame();

// 更新游戏状态（每帧调用）
game.update(game.getMusicTime());

// 处理按键事件
game.handleKeyPress(track);    // track: 0-3
game.handleKeyRelease(track);

// 获取渲染数据
const auto& notes = game.getNotes();        // 获取所有音符
const auto& stats = game.getStats();        // 获取游戏统计
GameState state = game.getState();          // 获取游戏状态
Judgment judgment = game.getLatestJudgment(); // 获取最新判定
```

### 成员 C 需要提供的数据
```cpp
Chart chart;
chart.songName = "歌曲名称";
chart.bpm = 120.0;
chart.duration = 180.0;

// 添加音符
Note note;
note.time = 1.5;      // 出现时间（秒）
note.track = 2;        // 轨道编号（0-3）
note.type = 0;         // 0=单按，1=长按
note.hit = false;      // 初始为未击中
chart.notes.push_back(note);
```

## 注意事项

1. **游戏时钟**：使用 `music.getPlayingOffset().asSeconds()` 获取，不要自己计时
2. **判定逻辑**：同一个 Note 不能判定两次，判定后 `note.hit` 设为 true
3. **数据结构**：第一天定好，后续修改需三人协商
