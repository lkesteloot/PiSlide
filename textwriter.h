
#pragma once

#include <string>
#include <map>
#include <memory>

#include "raylib.h"

class TextWriter {
    // Font size to font.
    std::map<float,std::shared_ptr<Font>> mFontCache;

public:
    TextWriter() {}
    virtual ~TextWriter() {}

    // Can't copy.
    TextWriter(const TextWriter &) = delete;
    TextWriter &operator=(const TextWriter &) = delete;

    enum class Alignment {
        // Left or top.
        START,

        // Centered horizontally or vertically.
        CENTER,

        // Right or bottom.
        END,
    };

    void write(std::string const &text,
            Vector2 position,
            float fontSize,
            Color color,
            Alignment horizontal,
            Alignment vertical);
};
