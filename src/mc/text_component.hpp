#pragma once
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <optional>

#include "color.hpp"

enum class text_component_type
{
    TEXT, TRANSLATABLE, SCORE, SELECTOR, KEYBIND, NBT, UNSET
};

struct text_event
{
    std::string action;
    // given name at write 
    std::string corresponding_value;
};

struct text_component_formatting
{
    std::string font     = ""; // empty means ignore
    bool bold            = false;
    bool italic          = false;
    bool underlined      = false;
    bool strikethrough   = false;
    bool obfuscated      = false;
    std::optional<std::variant<int32_t, std::array<float, 4>>> shadow_color;


    std::string tostr() const
    {
        std::stringstream stream;

    }
};


struct text_component
{
    inline static std::string typeToStr(const text_component_type& t)
    {
        switch (t)
        {
            case text_component_type::TEXT:
                return "text";
            case text_component_type::TRANSLATABLE:
                return "translatable";
            case text_component_type::SCORE:
                return "score";
            case text_component_type::SELECTOR:
                return "selector";
            case text_component_type::KEYBIND:
                return "keybind";
            case text_component_type::NBT:
                return "nbt";
            default:
                return "";
        }
    }
    
    std::optional<mc_color>         color;
    text_component_formatting       formatting;

    // add rest of optional fields here
    
    // interactivity
    std::string                     insersion = "";
    std::optional<text_event>       click_event;
    std::optional<text_event>       hover_event;

    std::unique_ptr<std::vector<text_component>> extra;
};