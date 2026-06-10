// LoadingScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdint>

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
        , m_rotationAngle(0.0f)
        , m_titleFlashTimer(0.0f)
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
        m_rotationAngle += 180.0f * deltaTime;
        if (m_rotationAngle > 360.0f) {
            m_rotationAngle -= 360.0f;
        }
        m_spinner.setRotation(sf::degrees(m_rotationAngle));

        // 标题脉冲动画
        m_titleFlashTimer += deltaTime;
        if (m_titleFlashTimer > 1.2f) {
            m_titleFlashTimer = 0.0f;
        }
        float intensity = 0.5f + 0.5f * std::sin(m_titleFlashTimer / 1.2f * 3.14159f * 2.0f);
        sf::Color titleColor(
            static_cast<std::uint8_t>(180 * intensity),
            static_cast<std::uint8_t>(220 * intensity),
            static_cast<std::uint8_t>(255 * intensity)
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
        float centerX = static_cast<float>(windowSize.x) / 2.0f;
        float centerY = static_cast<float>(windowSize.y) / 2.0f;

        // 背景
        m_background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        m_background.setFillColor(sf::Color(20, 20, 40));

        // 旋转进度条
        m_spinner.setSize(sf::Vector2f(200.0f, 8.0f));
        m_spinner.setOrigin(sf::Vector2f(100.0f, 4.0f));
        m_spinner.setPosition(sf::Vector2f(centerX, centerY + 60.0f));
        m_spinner.setFillColor(sf::Color(100, 150, 255));

        // 标题
        m_titleText.setOutlineColor(sf::Color::White);
        m_titleText.setOutlineThickness(1.5f);
        sf::FloatRect titleBounds = m_titleText.getLocalBounds();
        m_titleText.setOrigin(sf::Vector2f(titleBounds.size.x / 2.0f, titleBounds.size.y / 2.0f));
        m_titleText.setPosition(sf::Vector2f(centerX, centerY - 30.0f));

        // 提示文字
        m_subtitleText.setFillColor(sf::Color(150, 150, 180));
        sf::FloatRect subBounds = m_subtitleText.getLocalBounds();
        m_subtitleText.setOrigin(sf::Vector2f(subBounds.size.x / 2.0f, subBounds.size.y / 2.0f));
        m_subtitleText.setPosition(sf::Vector2f(centerX, centerY + 90.0f));

        // 退出提示
        m_hintText = sf::Text(m_font, "Press ESC to cancel", 16);
        m_hintText.setFillColor(sf::Color(120, 120, 150));
        sf::FloatRect hintBounds = m_hintText.getLocalBounds();
        m_hintText.setOrigin(sf::Vector2f(hintBounds.size.x / 2.0f, hintBounds.size.y / 2.0f));
        m_hintText.setPosition(sf::Vector2f(centerX, centerY + 130.0f));
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
