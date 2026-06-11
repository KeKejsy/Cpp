// ConfirmScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include "common.h"

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
        , m_difficultyIndex(1)     // 榛樿 Normal
        , m_titleText(font, "Chart Ready!", 40)
        , m_infoText(font, "", 22)
        , m_difficultyLabel(font, "Difficulty:", 24)
        , m_difficultyValue(font, "", 28)
        , m_diffLeftArrow(font, "<", 28)
        , m_diffRightArrow(font, ">", 28) {
        setupUI();
    }

    void setChart(const Chart& chart, const std::string& musicPath) {
        m_chart = chart;
        m_musicPath = musicPath;

        // 缁熻闊崇绫诲瀷
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

        // 鏍煎紡鍖栨椂闀? (绉? 鈫? m:ss)
        int minutes = static_cast<int>(chart.duration) / 60;
        int seconds = static_cast<int>(chart.duration) % 60;

        // 姣忚建閬撻煶绗﹀垎甯?
        int trackCounts[4] = {0, 0, 0, 0};
        for (const auto& note : chart.notes) {
            if (note.track >= 0 && note.track < 4) {
                trackCounts[note.track]++;
            }
        }

        std::stringstream ss;
        ss << "Song: " << chart.songName << "\n\n";
        ss << "BPM: " << std::fixed << std::setprecision(1) << chart.bpm << " (estimated)\n";
        ss << "Duration: " << minutes << ":" << std::setw(2) << std::setfill('0') << seconds << "\n\n";
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
    const std::string& getMusicPath() const { return m_musicPath; }
    Difficulty getDifficulty() const {
        return static_cast<Difficulty>(m_difficultyIndex);
    }

    ConfirmResult handleEvent(const sf::Event& event) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            switch (keyEvent->code) {
                case sf::Keyboard::Key::Up:
                    if (m_selectedIndex == 0) {
                        // 鍦? Start/Back 鎸夐挳鏃讹紝鎸変笂璺冲埌闅惧害閫夋嫨
                        m_selectedIndex = 2;  // difficulty row
                    } else if (m_selectedIndex == 2) {
                        m_selectedIndex = 2;  // 宸茬粡鍦ㄩ毦搴﹁
                    } else {
                        m_selectedIndex = (m_selectedIndex - 1 + 3) % 3;
                    }
                    updateSelection();
                    break;
                case sf::Keyboard::Key::Down:
                    if (m_selectedIndex == 2) {
                        m_selectedIndex = 0;  // 浠庨毦搴﹁烦鍒版寜閽?
                    } else {
                        m_selectedIndex = (m_selectedIndex + 1) % 3;
                    }
                    updateSelection();
                    break;
                case sf::Keyboard::Key::Left:
                    if (m_selectedIndex == 2) {
                        // 闄嶄綆闅惧害
                        m_difficultyIndex = (m_difficultyIndex - 1 + 3) % 3;
                        updateDifficultyDisplay();
                    }
                    break;
                case sf::Keyboard::Key::Right:
                    if (m_selectedIndex == 2) {
                        // 鎻愰珮闅惧害
                        m_difficultyIndex = (m_difficultyIndex + 1) % 3;
                        updateDifficultyDisplay();
                    }
                    break;
                case sf::Keyboard::Key::Enter:
                case sf::Keyboard::Key::Space:
                    if (m_selectedIndex < 2) {
                        return m_buttons[m_selectedIndex].action;
                    }
                    // 闅惧害琛屼笉鍝嶅簲 Enter
                    break;
                default:
                    break;
            }
        }

        if (const auto* mouseEvent = event.getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x),
                                  static_cast<float>(mouseEvent->position.y));
            // 妫�鏌ユ寜閽?
            for (size_t i = 0; i < m_buttons.size(); i++) {
                if (m_buttons[i].background.getGlobalBounds().contains(mousePos)) {
                    if (static_cast<int>(i) != m_selectedIndex) {
                        m_selectedIndex = static_cast<int>(i);
                        updateSelection();
                    }
                    break;
                }
            }
            // 妫�鏌ラ毦搴﹂�夋嫨鍣?
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
                // 鎸夐挳鐐瑰嚮
                for (const auto& btn : m_buttons) {
                    if (btn.background.getGlobalBounds().contains(mousePos)) {
                        return btn.action;
                    }
                }
                // 闅惧害宸︾澶?
                if (m_diffLeftArrow.getGlobalBounds().contains(mousePos)) {
                    m_difficultyIndex = (m_difficultyIndex - 1 + 3) % 3;
                    updateDifficultyDisplay();
                }
                // 闅惧害鍙崇澶?
                if (m_diffRightArrow.getGlobalBounds().contains(mousePos)) {
                    m_difficultyIndex = (m_difficultyIndex + 1) % 3;
                    updateDifficultyDisplay();
                }
            }
        }

        return ConfirmResult::None;
    }

    void update(float /*deltaTime*/) {
        // 闈欐�佺‘璁ょ晫闈紝鏃犻渶姣忓抚鏇存柊
    }

    void draw() {
        m_window.draw(m_background);
        m_window.draw(m_titleText);
        m_window.draw(m_infoText);

        // 闅惧害閫夋嫨鍣?
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

        Button(const sf::Font& font, const std::string& label, ConfirmResult act)
            : background()
            , text(font, label, 26)
            , action(act) {}
    };

    void setupUI() {
        sf::Vector2u windowSize = m_window.getSize();
        float centerX = static_cast<float>(windowSize.x) / 2.0f;

        // 背景
        m_background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        m_background.setFillColor(sf::Color(25, 25, 45));

        // 标题
        m_titleText.setFillColor(sf::Color::Cyan);
        m_titleText.setStyle(sf::Text::Bold);
        sf::FloatRect titleBounds = m_titleText.getLocalBounds();
        m_titleText.setPosition(sf::Vector2f(centerX - titleBounds.size.x / 2.0f, 30.0f));

        // 信息文字
        m_infoText.setFillColor(sf::Color(220, 220, 240));
        m_infoText.setPosition(sf::Vector2f(centerX - 280.0f, 95.0f));

        // ---- 难度选择器 - 上移到信息文字和按钮之间 ----
        float diffY = 440.0f;  // 从 430 下移到 440，给信息文字更多空间
        m_difficultyLabel.setFillColor(sf::Color(180, 180, 200));
        m_difficultyLabel.setPosition(sf::Vector2f(centerX - 130.0f, diffY));  // 左移

        m_diffLeftArrow.setFont(m_font);
        m_diffLeftArrow.setString("<");
        m_diffLeftArrow.setCharacterSize(28);
        m_diffLeftArrow.setFillColor(sf::Color::White);
        m_diffLeftArrow.setPosition(sf::Vector2f(centerX + 10.0f, diffY - 3.0f));

        m_difficultyValue.setFillColor(sf::Color::Yellow);
        m_difficultyValue.setStyle(sf::Text::Bold);
        m_difficultyValue.setPosition(sf::Vector2f(centerX + 45.0f, diffY - 2.0f));

        m_diffRightArrow.setFont(m_font);
        m_diffRightArrow.setString(">");
        m_diffRightArrow.setCharacterSize(28);
        m_diffRightArrow.setFillColor(sf::Color::White);
        m_diffRightArrow.setPosition(sf::Vector2f(centerX + 165.0f, diffY - 3.0f));

        updateDifficultyDisplay();

        // ---- 按钮 - 下移，加大间距 ----
        struct ButtonData {
            const char* label;
            ConfirmResult action;
        } buttons[] = {
            {"Start Game", ConfirmResult::StartGame},
            {"Back", ConfirmResult::Back}
        };

        float startY = 530.0f;        // 从 500 下移到 530
        float buttonSpacing = 260.0f; // 从 200 增加到 260
        float buttonWidth = 180.0f;   // 从 220 减小到 180
        float buttonHeight = 55.0f;

        for (int i = 0; i < 2; i++) {
            Button btn(m_font, buttons[i].label, buttons[i].action);

            btn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
            btn.background.setFillColor(sf::Color(60, 60, 100));
            btn.background.setOutlineColor(sf::Color::White);
            btn.background.setOutlineThickness(2.0f);
            btn.background.setPosition(sf::Vector2f(
                centerX - buttonWidth / 2.0f + (i - 0.5f) * buttonSpacing,
                startY
            ));

            btn.text.setFillColor(sf::Color::White);

            sf::FloatRect textBounds = btn.text.getLocalBounds();
            btn.text.setPosition(sf::Vector2f(
                centerX - textBounds.size.x / 2.0f + (i - 0.5f) * buttonSpacing,
                startY + (buttonHeight - textBounds.size.y) / 2.0f - 5.0f
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

        // 灞呬腑鏂囨湰
        sf::FloatRect bounds = m_difficultyValue.getLocalBounds();
        float centerX = static_cast<float>(m_window.getSize().x) / 2.0f;
        m_difficultyValue.setPosition(sf::Vector2f(centerX + 55.0f, 428.0f));
    }

    void updateSelection() {
        for (size_t i = 0; i < m_buttons.size(); i++) {
            if (static_cast<int>(i) == m_selectedIndex) {
                m_buttons[i].background.setFillColor(sf::Color(100, 100, 200));
                m_buttons[i].background.setOutlineColor(sf::Color::Yellow);
                m_buttons[i].background.setOutlineThickness(3.0f);
                m_buttons[i].text.setFillColor(sf::Color::Yellow);
            } else {
                m_buttons[i].background.setFillColor(sf::Color(60, 60, 100));
                m_buttons[i].background.setOutlineColor(sf::Color::White);
                m_buttons[i].background.setOutlineThickness(2.0f);
                m_buttons[i].text.setFillColor(sf::Color::White);
            }
        }

        // 闅惧害琛岄珮浜?
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
    std::string m_musicPath;
    std::vector<Button> m_buttons;
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
