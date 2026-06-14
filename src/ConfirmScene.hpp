// ConfirmScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <sstream>
#include <iomanip>
#include "common.h"
using namespace std;

enum class ConfirmResult {
    None,
    StartGame,
    Back
};

class ConfirmScene {
public:
    ConfirmScene(sf::RenderWindow& window, sf::Font& font)
        : m_window(window)
        , m_font(font)
        , m_selectedIndex(0)
        , m_difficultyIndex(1)     // 默认 Normal
        , m_titleText(font, "Chart Ready!", 40)
        , m_infoText(font, "", 22)
        , m_difficultyLabel(font, "Difficulty:", 24)
        , m_difficultyValue(font, "", 28)
        , m_diffLeftArrow(font, "<", 28)
        , m_diffRightArrow(font, ">", 28) {
        setupUI();
    }

    void setChart(const Chart& chart, const string& musicPath) {
        m_chart = chart;
        m_musicPath = musicPath;

        // 统计音符类型
        int tapCount = 0;
        int holdCount = 0;
        for (const auto& note : chart.notes) {
            if (note.type == 1) {
                holdCount++;
            } else {
                tapCount++;
            }
        }
        int totalNotes = static_cast<int>(chart.notes.size());

        // 格式化时�? (�? �? m:ss)
        int minutes = static_cast<int>(chart.duration) / 60;
        int seconds = static_cast<int>(chart.duration) % 60;

        // 每轨道音符分�?
        int trackCounts[4] = {0, 0, 0, 0};
        for (const auto& note : chart.notes) {
            if (note.track >= 0 && note.track < 4) {
                trackCounts[note.track]++;
            }
        }

        stringstream ss;
        ss << "Song: " << chart.songName << "\n\n";
        ss << "Note Density: " << fixed << setprecision(1) << chart.noteDensity << " NPM\n";
        ss << "Duration: " << minutes << ":" << setw(2) << setfill('0') << seconds << "\n\n";
        ss << "Total Notes: " << totalNotes << "\n";
        ss << "  Tap:  " << tapCount << "\n";
        ss << "  Hold: " << holdCount << "\n\n";
        ss << "Per Track:\n";
        ss << "  D (Bass):  " << trackCounts[0] << "\n";
        ss << "  F (LoMid): " << trackCounts[1] << "\n";
        ss << "  J (HiMid): " << trackCounts[2] << "\n";
        ss << "  K (High):  " << trackCounts[3];

        m_infoText.setString(ss.str());
    }

    const Chart& getChart() const { return m_chart; }
    const string& getMusicPath() const { return m_musicPath; }
    Difficulty getDifficulty() const {
        return static_cast<Difficulty>(m_difficultyIndex);
    }

    ConfirmResult handleEvent(const sf::Event& event) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            switch (keyEvent->code) {
                case sf::Keyboard::Key::Up:
                    if (m_selectedIndex == 0) {
                        // �? Start/Back 按钮时，按上跳到难度选择
                        m_selectedIndex = 2;  // difficulty row
                    } else if (m_selectedIndex == 2) {
                        m_selectedIndex = 2;  // 已经在难度行
                    } else {
                        m_selectedIndex = (m_selectedIndex - 1 + 3) % 3;
                    }
                    updateSelection();
                    break;
                case sf::Keyboard::Key::Down:
                    if (m_selectedIndex == 2) {
                        m_selectedIndex = 0;  // 从难度跳到按�?
                    } else {
                        m_selectedIndex = (m_selectedIndex + 1) % 3;
                    }
                    updateSelection();
                    break;
                case sf::Keyboard::Key::Left:
                    if (m_selectedIndex == 2) {
                        // 降低难度
                        m_difficultyIndex = (m_difficultyIndex - 1 + 3) % 3;
                        updateDifficultyDisplay();
                    }
                    break;
                case sf::Keyboard::Key::Right:
                    if (m_selectedIndex == 2) {
                        // 提高难度
                        m_difficultyIndex = (m_difficultyIndex + 1) % 3;
                        updateDifficultyDisplay();
                    }
                    break;
                case sf::Keyboard::Key::Enter:
                case sf::Keyboard::Key::Space:
                    if (m_selectedIndex < 2) {
                        return m_buttons[m_selectedIndex].action;
                    }
                    // 难度行不响应 Enter
                    break;
                default:
                    break;
            }
        }

        if (const auto* mouseEvent = event.getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x),
                                  static_cast<float>(mouseEvent->position.y));
            // 检查按�?
            for (size_t i = 0; i < m_buttons.size(); i++) {
                if (m_buttons[i].background.getGlobalBounds().contains(mousePos)) {
                    if (static_cast<int>(i) != m_selectedIndex) {
                        m_selectedIndex = static_cast<int>(i);
                        updateSelection();
                    }
                    break;
                }
            }
            // 检查难度选择�?
            if (m_diffLeftArrow.getGlobalBounds().contains(mousePos) ||
                m_diffRightArrow.getGlobalBounds().contains(mousePos) ||
                m_difficultyValue.getGlobalBounds().contains(mousePos)) {
                if (m_selectedIndex != 2) {
                    m_selectedIndex = 2;
                    updateSelection();
                }
            }
        }

        if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left) {
                sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x),
                                      static_cast<float>(mouseEvent->position.y));
                // 按钮点击
                for (const auto& btn : m_buttons) {
                    if (btn.background.getGlobalBounds().contains(mousePos)) {
                        return btn.action;
                    }
                }
                // 难度左箭�?
                if (m_diffLeftArrow.getGlobalBounds().contains(mousePos)) {
                    m_difficultyIndex = (m_difficultyIndex - 1 + 3) % 3;
                    updateDifficultyDisplay();
                }
                // 难度右箭�?
                if (m_diffRightArrow.getGlobalBounds().contains(mousePos)) {
                    m_difficultyIndex = (m_difficultyIndex + 1) % 3;
                    updateDifficultyDisplay();
                }
            }
        }

        return ConfirmResult::None;
    }

    void update(float /*deltaTime*/) {
        // 静态确认界面，无需每帧更新
    }

    void draw() {
        m_window.draw(m_background);
        m_window.draw(m_titleText);
        m_window.draw(m_infoText);

        // 难度选择�?
        m_window.draw(m_difficultyLabel);
        m_window.draw(m_difficultyValue);
        m_window.draw(m_diffLeftArrow);
        m_window.draw(m_diffRightArrow);

        for (const auto& btn : m_buttons) {
            m_window.draw(btn.background);
            m_window.draw(btn.text);
        }
    }

private:
    struct Button {
        sf::RectangleShape background;
        sf::Text text;
        ConfirmResult action;

        Button(const sf::Font& font, const string& label, ConfirmResult act)
            : background()
            , text(font, label, 26)
            , action(act) {}
    };

    void setupUI() {
        sf::Vector2u windowSize = m_window.getSize();
        float centerX = static_cast<float>(windowSize.x) / 2.0;

        // ����
        m_background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        m_background.setFillColor(sf::Color(25, 25, 45));

        // ����
        m_titleText.setFillColor(sf::Color::Cyan);
        m_titleText.setStyle(sf::Text::Bold);
        sf::FloatRect titleBounds = m_titleText.getLocalBounds();
        m_titleText.setPosition(sf::Vector2f(centerX - titleBounds.size.x / 2.0, 30.0));

        // ��Ϣ����
        m_infoText.setFillColor(sf::Color(220, 220, 240));
        m_infoText.setPosition(sf::Vector2f(centerX - 280.0, 95.0));

        // ---- �Ѷ�ѡ���� - ���Ƶ���Ϣ���ֺͰ�ť֮�� ----
        float diffY = 440.0;  // �� 430 ���Ƶ� 440������Ϣ���ָ���ռ�
        m_difficultyLabel.setFillColor(sf::Color(180, 180, 200));
        m_difficultyLabel.setPosition(sf::Vector2f(centerX - 130.0, diffY));  // ����

        m_diffLeftArrow.setFont(m_font);
        m_diffLeftArrow.setString("<");
        m_diffLeftArrow.setCharacterSize(28);
        m_diffLeftArrow.setFillColor(sf::Color::White);
        m_diffLeftArrow.setPosition(sf::Vector2f(centerX + 10.0, diffY - 3.0));

        m_difficultyValue.setFillColor(sf::Color::Yellow);
        m_difficultyValue.setStyle(sf::Text::Bold);
        m_difficultyValue.setPosition(sf::Vector2f(centerX + 45.0, diffY - 2.0));

        m_diffRightArrow.setFont(m_font);
        m_diffRightArrow.setString(">");
        m_diffRightArrow.setCharacterSize(28);
        m_diffRightArrow.setFillColor(sf::Color::White);
        m_diffRightArrow.setPosition(sf::Vector2f(centerX + 165.0, diffY - 3.0));

        updateDifficultyDisplay();

        // ---- ��ť - ���ƣ��Ӵ��� ----
        struct ButtonData {
            const char* label;
            ConfirmResult action;
        } buttons[] = {
            {"Start Game", ConfirmResult::StartGame},
            {"Back", ConfirmResult::Back}
        };

        float startY = 530.0;        // �� 500 ���Ƶ� 530
        float buttonSpacing = 260.0; // �� 200 ���ӵ� 260
        float buttonWidth = 180.0;   // �� 220 ��С�� 180
        float buttonHeight = 55.0;

        for (int i = 0; i < 2; i++) {
            Button btn(m_font, buttons[i].label, buttons[i].action);

            btn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
            btn.background.setFillColor(sf::Color(60, 60, 100));
            btn.background.setOutlineColor(sf::Color::White);
            btn.background.setOutlineThickness(2.0);
            btn.background.setPosition(sf::Vector2f(
                centerX - buttonWidth / 2.0 + (i - 0.5) * buttonSpacing,
                startY
            ));

            btn.text.setFillColor(sf::Color::White);

            sf::FloatRect textBounds = btn.text.getLocalBounds();
            btn.text.setPosition(sf::Vector2f(
                centerX - textBounds.size.x / 2.0 + (i - 0.5) * buttonSpacing,
                startY + (buttonHeight - textBounds.size.y) / 2.0 - 5.0
            ));

            m_buttons.push_back(btn);
        }

        updateSelection();
    }

    void updateDifficultyDisplay() {
        static const char* names[] = {"Easy", "Normal", "Hard"};
        static sf::Color colors[] = {
            sf::Color(100, 255, 100),   // Easy: green
            sf::Color(255, 255, 100),   // Normal: yellow
            sf::Color(255, 100, 100)    // Hard: red
        };

        m_difficultyValue.setString(names[m_difficultyIndex]);
        m_difficultyValue.setFillColor(colors[m_difficultyIndex]);

        // 居中文本
        sf::FloatRect bounds = m_difficultyValue.getLocalBounds();
        float centerX = static_cast<float>(m_window.getSize().x) / 2.0;
        m_difficultyValue.setPosition(sf::Vector2f(centerX + 55.0, 428.0));
    }

    void updateSelection() {
        for (size_t i = 0; i < m_buttons.size(); i++) {
            if (static_cast<int>(i) == m_selectedIndex) {
                m_buttons[i].background.setFillColor(sf::Color(100, 100, 200));
                m_buttons[i].background.setOutlineColor(sf::Color::Yellow);
                m_buttons[i].background.setOutlineThickness(3.0);
                m_buttons[i].text.setFillColor(sf::Color::Yellow);
            } else {
                m_buttons[i].background.setFillColor(sf::Color(60, 60, 100));
                m_buttons[i].background.setOutlineColor(sf::Color::White);
                m_buttons[i].background.setOutlineThickness(2.0);
                m_buttons[i].text.setFillColor(sf::Color::White);
            }
        }

        // 难度行高�?
        if (m_selectedIndex == 2) {
            m_difficultyLabel.setFillColor(sf::Color::Yellow);
            m_diffLeftArrow.setFillColor(sf::Color::Yellow);
            m_diffRightArrow.setFillColor(sf::Color::Yellow);
        } else {
            m_difficultyLabel.setFillColor(sf::Color(180, 180, 200));
            m_diffLeftArrow.setFillColor(sf::Color::White);
            m_diffRightArrow.setFillColor(sf::Color::White);
        }
    }

    sf::RenderWindow& m_window;
    sf::Font& m_font;
    Chart m_chart;
    string m_musicPath;
    vector<Button> m_buttons;
    int m_selectedIndex;
    int m_difficultyIndex;  // 0=Easy, 1=Normal, 2=Hard

    sf::Text m_titleText;
    sf::Text m_infoText;
    sf::Text m_difficultyLabel;
    sf::Text m_difficultyValue;
    sf::Text m_diffLeftArrow;
    sf::Text m_diffRightArrow;
    sf::RectangleShape m_background;
};
