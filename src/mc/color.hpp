#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <unordered_map>

using mc_color_storage_t = std::variant<uint32_t, std::string>;

struct mc_color
{

    inline const static std::unordered_map<std::string, int32_t> mc_colors {
        { "black",        0x000000 },
        { "dark_blue",    0x0000AA },
        { "dark_green",   0x00AA00 },
        { "dark_aqua",    0x00AAAA },
        { "dark_red",     0xAA0000 },
        { "dark_purple",  0xAA00AA },
        { "gold",         0xFFAA00 },
        { "gray",         0xAAAAAA },
        { "dark_gray",    0x555555 },
        { "blue",         0x5555FF },
        { "green",        0x55FF55 },
        { "aqua",         0x55FFFF },
        { "red",          0xFF5555 },
        { "light_purple", 0xFF55FF },
        { "yellow",       0xFFFF55 },
        { "white",        0xFFFFFF }
    };
    mc_color_storage_t color;
public:
    mc_color (const std::string& colorName) : color(colorName)
    { }
    mc_color () : color((uint32_t)0xFFFFFF)
    { }
    mc_color (const mc_color_storage_t& colorValue) : color(colorValue)
    { }

    inline uint32_t getHex()
    {
        if (color.index())
        {
            auto f = mc_colors.find(std::get<std::string>(color));
            if (f == mc_colors.end())
                return 0xFFFFFF; // white
            return f->second;
        }
        return std::get<uint32_t>(color);
    }
};