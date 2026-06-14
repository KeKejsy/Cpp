// MenuScene.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include "AssetManager.hpp"
using namespace std;

enum class MenuResult {
    None,
    StartGame,
    Exit
};

class MenuScene {
public:
    MenuScene(sf::RenderWindow& window)
        : m_window(window)
        , m_font(AssetManager::getInstance().getFont())
        , m_selectedIndex(0)
        , m_titleText(m_font, "=== Rhythm Master ===", 48)
        , m_titleFlashTimer(0.0) {
        setupUI();
        initParticles();
    }

    MenuResult handleEvent(const sf::Event& event) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            switch (keyEvent->code) {
                case sf::Keyboard::Key::Up:
                    m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_items.size())) % static_cast<int>(m_items.size());
                    updateButtonColors();
                    break;
                case sf::Keyboard::Key::Down:
                    m_selectedIndex = (m_selectedIndex + 1) % static_cast<int>(m_items.size());
                    updateButtonColors();
                    break;
                case sf::Keyboard::Key::Enter:
                case sf::Keyboard::Key::Space:
                    return m_items[m_selectedIndex].action;
                default:
                    break;
            }
        }
        
        if (const auto* mouseEvent = event.getIf<sf::Event::MouseMoved>()) {
            sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x), 
                                  static_cast<float>(mouseEvent->position.y));
            for (size_t i = 0; i < m_items.size(); i++) {
                if (m_items[i].background.getGlobalBounds().contains(mousePos)) {
                    if (static_cast<int>(i) != m_selectedIndex) {
                        m_selectedIndex = static_cast<int>(i);
                        updateButtonColors();
                    }
                    break;
                }
            }
        }
        
        if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left) {
                sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x),
                                      static_cast<float>(mouseEvent->position.y));
                for (const auto& item : m_items) {
                    if (item.background.getGlobalBounds().contains(mousePos)) {
                        return item.action;
                    }
                }
            }
        }
        
        return MenuResult::None;
    }

    void update(float deltaTime) {
        m_titleFlashTimer += deltaTime;
        if (m_titleFlashTimer > 0.8) {
            m_titleFlashTimer = 0.0;
        }
        
        float intensity = 0.6 + 0.4 * (m_titleFlashTimer / 0.8);
        sf::Color titleColor(
            static_cast<uint8_t>(100 * intensity),
            static_cast<uint8_t>(200 * intensity),
            static_cast<uint8_t>(255 * intensity)
        );
        m_titleText.setFillColor(titleColor);
        
        updateParticles(deltaTime);
    }

    void draw() {
        m_window.draw(m_background);
        m_window.draw(m_topBar);
        m_window.draw(m_bottomBar);
        
        for (const auto& particle : m_particles) {
            m_window.draw(particle.shape);
        }
        
        m_window.draw(m_titleText);
        
        for (const auto& item : m_items) {
            m_window.draw(item.background);
            m_window.draw(item.text);
        }
    }

private:
    struct MenuItem {
        sf::RectangleShape background;
        sf::Text text;
        MenuResult action;
        
        MenuItem(const sf::Font& font, const string& label, MenuResult act)
            : background()
            , text(font, label, 28)
            , action(act) {}
    };
    
    struct Particle {
        sf::CircleShape shape;
        float speed;
        float x, y;
        
        Particle() : shape(), speed(0.0), x(0.0), y(0.0) {}
    };

    void setupUI() {
        sf::Vector2u windowSize = m_window.getSize();
        float centerX = static_cast<float>(windowSize.x) / 2.0;
        
        m_background.setSize(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
        m_background.setFillColor(sf::Color(20, 20, 40));
        
        m_topBar.setSize(sf::Vector2f(static_cast<float>(windowSize.x), 5.0));
        m_topBar.setFillColor(sf::Color(100, 100, 200));
        m_topBar.setPosition(sf::Vector2f(0.0, 0.0));
        
        m_bottomBar.setSize(sf::Vector2f(static_cast<float>(windowSize.x), 5.0));
        m_bottomBar.setFillColor(sf::Color(100, 100, 200));
        m_bottomBar.setPosition(sf::Vector2f(0.0, static_cast<float>(windowSize.y) - 5.0));
        
        m_titleText.setOutlineColor(sf::Color::White);
        m_titleText.setOutlineThickness(2.0);
        
        sf::FloatRect titleBounds = m_titleText.getLocalBounds();
        m_titleText.setPosition(sf::Vector2f(
            centerX - titleBounds.size.x / 2.0,
            100.0
        ));
        
        struct ItemData {
            const char* label;
            MenuResult action;
        } items[] = {
            {"Start Game", MenuResult::StartGame},
            {"Exit", MenuResult::Exit}
        };
        
        float startY = 300.0;
        float itemSpacing = 80.0;
        float itemWidth = 250.0;
        float itemHeight = 60.0;
        
        for (int i = 0; i < 2; i++) {
            MenuItem item(m_font, items[i].label, items[i].action);
            
            item.background.setSize(sf::Vector2f(itemWidth, itemHeight));
            item.background.setFillColor(sf::Color(60, 60, 100));
            item.background.setOutlineColor(sf::Color::White);
            item.background.setOutlineThickness(2.0);
            item.background.setPosition(sf::Vector2f(
                centerX - itemWidth / 2.0,
                startY + i * itemSpacing
            ));
            
            item.text.setFillColor(sf::Color::White);
            
            sf::FloatRect textBounds = item.text.getLocalBounds();
            item.text.setPosition(sf::Vector2f(
                centerX - textBounds.size.x / 2.0,
                startY + i * itemSpacing + (itemHeight - textBounds.size.y) / 2.0 - 5.0
            ));
            
            m_items.push_back(item);
        }
        
        updateButtonColors();
    }
    
    void updateButtonColors() {
        for (size_t i = 0; i < m_items.size(); i++) {
            if (static_cast<int>(i) == m_selectedIndex) {
                m_items[i].background.setFillColor(sf::Color(100, 100, 200));
                m_items[i].background.setOutlineColor(sf::Color::Yellow);
                m_items[i].background.setOutlineThickness(3.0);
                m_items[i].text.setFillColor(sf::Color::Yellow);
            } else {
                m_items[i].background.setFillColor(sf::Color(60, 60, 100));
                m_items[i].background.setOutlineColor(sf::Color::White);
                m_items[i].background.setOutlineThickness(2.0);
                m_items[i].text.setFillColor(sf::Color::White);
            }
        }
    }
    
    void initParticles() {
        sf::Vector2u windowSize = m_window.getSize();
        m_particles.clear();
        
        for (int i = 0; i < 100; i++) {
            Particle p;
            float radius = 2.0 + static_cast<float>(rand() % 3);
            p.shape.setRadius(radius);
            p.shape.setFillColor(sf::Color(
                static_cast<uint8_t>(100 + rand() % 155),
                static_cast<uint8_t>(100 + rand() % 155),
                static_cast<uint8_t>(200 + rand() % 55),
                100
            ));
            
            p.x = static_cast<float>(rand() % windowSize.x);
            p.y = static_cast<float>(rand() % windowSize.y);
            p.shape.setPosition(sf::Vector2f(p.x, p.y));
            p.speed = 20.0 + static_cast<float>(rand() % 50);
            
            m_particles.push_back(p);
        }
    }
    
    void updateParticles(float deltaTime) {
        sf::Vector2u windowSize = m_window.getSize();
        
        for (auto& p : m_particles) {
            p.y += p.speed * deltaTime;
            
            if (p.y > windowSize.y) {
                p.y = 0;
                p.x = static_cast<float>(rand() % windowSize.x);
            }
            
            p.shape.setPosition(sf::Vector2f(p.x, p.y));
        }
    }

    sf::RenderWindow& m_window;
    sf::Font& m_font;
    vector<MenuItem> m_items;
    int m_selectedIndex;
    
    sf::Text m_titleText;
    float m_titleFlashTimer;
    
    vector<Particle> m_particles;
    sf::RectangleShape m_background;
    sf::RectangleShape m_topBar;
    sf::RectangleShape m_bottomBar;
};