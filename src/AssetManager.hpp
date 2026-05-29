// AssetManager.hpp
#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <unordered_map>
#include <cstdint>

class AssetManager {
public:
    static AssetManager& getInstance() {
        static AssetManager instance;
        return instance;
    }

    sf::Font& getFont(const std::string& path = "") {
        std::string key = path.empty() ? "default" : path;
        
        if (m_fonts.find(key) == m_fonts.end()) {
            sf::Font font;
            if (key == "default") {
                const char* fontPaths[] = {
                    "C:/Windows/Fonts/arial.ttf",
                    "C:/Windows/Fonts/msyh.ttc",
                    "C:/Windows/Fonts/simhei.ttf"
                };
                bool loaded = false;
                for (const auto& p : fontPaths) {
                    if (font.openFromFile(p)) {
                        loaded = true;
                        break;
                    }
                }
                if (!loaded) {
                    std::cerr << "Warning: Cannot load font" << std::endl;
                }
            } else {
                [[maybe_unused]] bool loaded = font.openFromFile(key);
            }
            m_fonts[key] = std::move(font);
        }
        return m_fonts[key];
    }

private:
    AssetManager() = default;
    std::unordered_map<std::string, sf::Font> m_fonts;
};