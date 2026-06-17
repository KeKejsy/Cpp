// ResultScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <sstream>
#include <iomanip>
#include "Framework.h"
using namespace std;

enum class ResultAction {
    None,
    Restart,
    BackToMenu
};

class ResultScene {
public:
    ResultScene(sf::RenderWindow& window, sf::Font& font)
        : m_window(window)
        , m_font(font)
        , m_selectedIndex(0)
        , m_titleText(font, "=== 游戏结束 ===", 40)
        , m_statsText(font, "", 20)
        , m_rankText(font, "S", 80)
        , m_animationTimer(0.0) {
        setupUI();
    }

    void setStats(const GameStats& stats) {
        m_stats = stats;
        
        stringstream ss;
        ss << "总分: " << m_stats.totalScore << "\n\n";
        ss << "Perfect: " << m_stats.perfectCount << "\n";
        ss << "Great:   " << m_stats.greatCount << "\n";
        ss << "Good:    " << m_stats.goodCount << "\n";
        ss << "Miss:    " << m_stats.missCount << "\n\n";
        ss << "最高连击: " << m_stats.maxCombo << "\n";
        ss << "准确率: ";
        
        int totalNotes = m_stats.perfectCount + m_stats.greatCount + m_stats.goodCount + m_stats.missCount;
        if (totalNotes > 0) {
            int hitNotes = m_stats.perfectCount + m_stats.greatCount + m_stats.goodCount;
            float accuracy = (float)hitNotes / totalNotes * 100.0;
            ss << fixed << setprecision(1) << accuracy << "%";
        } else {
            ss << "0%";
        }
        
        m_statsText.setString(ss.str());
        
        char rank = calculateRank();
        m_rankText.setString(string(1, rank));
        
        switch (rank) {
            case 'S': m_rankText.setFillColor(sf::Color(255, 215, 0)); break;
            case 'A': m_rankText.setFillColor(sf::Color(0, 255, 0)); break;
            case 'B': m_rankText.setFillColor(sf::Color(100, 200, 255)); break;
            case 'C': m_rankText.setFillColor(sf::Color(255, 165, 0)); break;
            default: m_rankText.setFillColor(sf::Color(255, 100, 100)); break;
        }
    }

    ResultAction handleEvent(const sf::Event& event) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            switch (keyEvent->code) {
                case sf::Keyboard::Key::Left:
                    m_selectedIndex = (m_selectedIndex - 1 + 2) % 2;
                    updateSelection(m_selectedIndex);
                    break;
                case sf::Keyboard::Key::Right:
                    m_selectedIndex = (m_selectedIndex + 1) % 2;
                    updateSelection(m_selectedIndex);
                    break;
                case sf::Keyboard::Key::Enter:
                case sf::Keyboard::Key::Space:
                    return m_buttons[m_selectedIndex].action;
                default:
                    break;
            }
        }
        
        if (const auto* mouseEvent = event.getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f mousePos(mouseEvent->position.x, mouseEvent->position.y);
            for (size_t i = 0; i < m_buttons.size(); i++) {
                if (m_buttons[i].background.getGlobalBounds().contains(mousePos)) {
                    if (i != m_selectedIndex) {
                        m_selectedIndex = i;
                        updateSelection(m_selectedIndex);
                    }
                    break;
                }
            }
        }
        
        if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left) {
                sf::Vector2f mousePos(mouseEvent->position.x, mouseEvent->position.y);
                for (const auto& btn : m_buttons) {
                    if (btn.background.getGlobalBounds().contains(mousePos)) {
                        return btn.action;
                    }
                }
            }
        }
        
        return ResultAction::None;
    }

    void update(float deltaTime) {
        m_animationTimer += deltaTime;
    }

    void draw() {
        m_window.draw(m_background);
        m_window.draw(m_titleText);
        m_window.draw(m_statsText);
        m_window.draw(m_rankText);
        
        for (const auto& btn : m_buttons) {
            m_window.draw(btn.background);
            m_window.draw(btn.text);
        }
    }

private:
    struct Button {
        sf::RectangleShape background;
        sf::Text text;
        ResultAction action;
        
        Button(const sf::Font& font, const string& label, ResultAction act)
            : background()
            , text(font, label, 24)
            , action(act) {}
    };

    char calculateRank() const {
        int totalNotes = m_stats.perfectCount + m_stats.greatCount + m_stats.goodCount + m_stats.missCount;
        if (totalNotes == 0) return 'F';
        
        float accuracy = (float)(m_stats.perfectCount + m_stats.greatCount + m_stats.goodCount) / totalNotes;
        
        if (accuracy >= 0.98 && m_stats.missCount == 0) return 'S';
        if (accuracy >= 0.90) return 'A';
        if (accuracy >= 0.80) return 'B';
        if (accuracy >= 0.70) return 'C';
        return 'D';
    }

    void setupUI() {
        sf::Vector2u windowSize = m_window.getSize();
        float centerX = windowSize.x / 2.0;

        m_background.setSize(sf::Vector2f(windowSize.x, windowSize.y));
        m_background.setFillColor(sf::Color(30, 30, 50));
        
        m_titleText.setFillColor(sf::Color::Cyan);
        sf::FloatRect titleBounds = m_titleText.getLocalBounds();
        m_titleText.setPosition(sf::Vector2f(centerX - titleBounds.size.x / 2.0, 50.0));
        
        m_statsText.setFillColor(sf::Color::White);
        m_statsText.setPosition(sf::Vector2f(centerX - 250.0, 200.0));
        
        m_rankText.setStyle(sf::Text::Bold);
        m_rankText.setPosition(sf::Vector2f(centerX + 200.0, 220.0));
        
        struct ButtonData {
            const char* label;
            ResultAction action;
        } buttons[] = {
            {"再来一局", ResultAction::Restart},
            {"返回主菜单", ResultAction::BackToMenu}
        };
        
        // �޸���ť��� - �Ӵ�������ص�
        float startY = 540.0;
        float buttonSpacing = 260.0;  // �� 100 ��Ϊ 260
        float buttonWidth = 180.0;    // �� 200 ��Ϊ 180
        float buttonHeight = 50.0;
        
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
        
        updateSelection(0);
    }

    void updateSelection(int index) {
        m_selectedIndex = index;
        
        for (size_t i = 0; i < m_buttons.size(); i++) {
            if (i == m_selectedIndex) {
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
    }

    sf::RenderWindow& m_window;
    sf::Font& m_font;
    GameStats m_stats;
    vector<Button> m_buttons;
    int m_selectedIndex;
    
    sf::Text m_titleText;
    sf::Text m_statsText;
    sf::Text m_rankText;
    
    sf::RectangleShape m_background;
    float m_animationTimer;
};