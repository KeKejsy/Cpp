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
        , m_titleText(font, "Chart Ready!", 40)
        , m_infoText(font, "", 22) {
        setupUI();
    }

    void setChart(const Chart& chart, const std::string& musicPath) {
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

        // 格式化时长 (秒 → m:ss)
        int minutes = static_cast<int>(chart.duration) / 60;
        int seconds = static_cast<int>(chart.duration) % 60;

        std::stringstream ss;
        ss << "Song: " << chart.songName << "\n\n";
        ss << "BPM: " << std::fixed << std::setprecision(1) << chart.bpm << " (estimated)\n";
        ss << "Duration: " << minutes << ":" << std::setw(2) << std::setfill('0') << seconds << "\n\n";
        ss << "Total Notes: " << totalNotes << "\n";
        ss << "  Tap:  " << tapCount << "\n";
        ss << "  Hold: " << holdCount;

        m_infoText.setString(ss.str());
    }

    const Chart& getChart() const { return m_chart; }
    const std::string& getMusicPath() const { return m_musicPath; }

    ConfirmResult handleEvent(const sf::Event& event) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            switch (keyEvent->code) {
                case sf::Keyboard::Key::Left:
                    m_selectedIndex = (m_selectedIndex - 1 + 2) % 2;
                    updateSelection();
                    break;
                case sf::Keyboard::Key::Right:
                    m_selectedIndex = (m_selectedIndex + 1) % 2;
                    updateSelection();
                    break;
                case sf::Keyboard::Key::Enter:
                case sf::Keyboard::Key::Space:
                    return m_buttons[m_selectedIndex].action;
                default:
                    break;
            }
        }

        if (const auto* mouseEvent = event.getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x),
                                  static_cast<float>(mouseEvent->position.y));
            for (size_t i = 0; i < m_buttons.size(); i++) {
                if (m_buttons[i].background.getGlobalBounds().contains(mousePos)) {
                    if (static_cast<int>(i) != m_selectedIndex) {
                        m_selectedIndex = static_cast<int>(i);
                        updateSelection();
                    }
                    break;
                }
            }
        }

        if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left) {
                sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x),
                                      static_cast<float>(mouseEvent->position.y));
                for (const auto& btn : m_buttons) {
                    if (btn.background.getGlobalBounds().contains(mousePos)) {
                        return btn.action;
                    }
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
        m_titleText.setPosition(sf::Vector2f(centerX - titleBounds.size.x / 2.0f, 50.0f));

        // 信息文字
        m_infoText.setFillColor(sf::Color(220, 220, 240));
        m_infoText.setPosition(sf::Vector2f(centerX - 250.0f, 150.0f));

        // 按钮
        struct ButtonData {
            const char* label;
            ConfirmResult action;
        } buttons[] = {
            {"Start Game", ConfirmResult::StartGame},
            {"Back", ConfirmResult::Back}
        };

        float startY = 500.0f;
        float buttonSpacing = 200.0f;
        float buttonWidth = 220.0f;
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
    }

    sf::RenderWindow& m_window;
    sf::Font& m_font;
    Chart m_chart;
    std::string m_musicPath;
    std::vector<Button> m_buttons;
    int m_selectedIndex;

    sf::Text m_titleText;
    sf::Text m_infoText;
    sf::RectangleShape m_background;
};
