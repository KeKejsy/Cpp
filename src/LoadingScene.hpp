// LoadingScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
using namespace std;

enum class LoadingResult {
    None,
    Cancelled
};

class LoadingScene {
public:
    LoadingScene(sf::RenderWindow& window, sf::Font& font)
        : m_window(window)
        , m_font(font)
        , m_titleText(font, "Generating Chart...", 40)
        , m_subtitleText(font, "Please wait...", 20)
        , m_hintText(font, "Press ESC to cancel", 16)
        , m_rotationAngle(0.0)
        , m_titleFlashTimer(0.0)
        , m_cancelled(false) {
        setupUI();
    }

    LoadingResult handleEvent(const sf::Event& event) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (keyEvent->code == sf::Keyboard::Key::Escape) {
                m_cancelled = true;
                return LoadingResult::Cancelled;
            }
        }
        return LoadingResult::None;
    }

    void update(float deltaTime) {
        // 旋转进度条
        m_rotationAngle += 180.0 * deltaTime;
        if (m_rotationAngle > 360.0) {
            m_rotationAngle -= 360.0;
        }
        m_spinner.setRotation(sf::degrees(m_rotationAngle));

        // 标题脉冲动画
        m_titleFlashTimer += deltaTime;
        if (m_titleFlashTimer > 1.2) {
            m_titleFlashTimer = 0.0;
        }
        float intensity = 0.5 + 0.5 * sin(m_titleFlashTimer / 1.2 * 3.14159 * 2.0);
        sf::Color titleColor(
            180 * intensity,
            220 * intensity,
            255 * intensity
        );
        m_titleText.setFillColor(titleColor);
    }

    void draw() {
        m_window.draw(m_background);
        m_window.draw(m_spinner);
        m_window.draw(m_titleText);
        m_window.draw(m_subtitleText);
        m_window.draw(m_hintText);
    }

    bool isCancelled() const { return m_cancelled; }
    void resetCancelled() { m_cancelled = false; }

private:
    void setupUI() {
        sf::Vector2u windowSize = m_window.getSize();
        float centerX = windowSize.x / 2.0;
        float centerY = windowSize.y / 2.0;

        // 背景
        m_background.setSize(sf::Vector2f(windowSize.x, windowSize.y));
        m_background.setFillColor(sf::Color(20, 20, 40));

        // 旋转进度条
        m_spinner.setSize(sf::Vector2f(200.0, 8.0));
        m_spinner.setOrigin(sf::Vector2f(100.0, 4.0));
        m_spinner.setPosition(sf::Vector2f(centerX, centerY + 60.0));
        m_spinner.setFillColor(sf::Color(100, 150, 255));

        // 标题
        m_titleText.setOutlineColor(sf::Color::White);
        m_titleText.setOutlineThickness(1.5);
        sf::FloatRect titleBounds = m_titleText.getLocalBounds();
        m_titleText.setOrigin(sf::Vector2f(titleBounds.size.x / 2.0, titleBounds.size.y / 2.0));
        m_titleText.setPosition(sf::Vector2f(centerX, centerY - 30.0));

        // 提示文字
        m_subtitleText.setFillColor(sf::Color(150, 150, 180));
        sf::FloatRect subBounds = m_subtitleText.getLocalBounds();
        m_subtitleText.setOrigin(sf::Vector2f(subBounds.size.x / 2.0, subBounds.size.y / 2.0));
        m_subtitleText.setPosition(sf::Vector2f(centerX, centerY + 90.0));

        // 退出提示
        m_hintText = sf::Text(m_font, "Press ESC to cancel", 16);
        m_hintText.setFillColor(sf::Color(120, 120, 150));
        sf::FloatRect hintBounds = m_hintText.getLocalBounds();
        m_hintText.setOrigin(sf::Vector2f(hintBounds.size.x / 2.0, hintBounds.size.y / 2.0));
        m_hintText.setPosition(sf::Vector2f(centerX, centerY + 130.0));
    }

    sf::RenderWindow& m_window;
    sf::Font& m_font;
    sf::Text m_titleText;
    sf::Text m_subtitleText;
    sf::Text m_hintText;
    sf::RectangleShape m_spinner;
    sf::RectangleShape m_background;
    float m_rotationAngle;
    float m_titleFlashTimer;
    bool m_cancelled;
};
