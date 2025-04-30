
#pragma once

#include <string>
#include <map>
#include <memory>

#include "raylib.h"

/**
 * Writes text to the screen, loading and caching fonts of the right size.
 */
class TextWriter final {
    // Font size to font.
    std::map<float,std::shared_ptr<Font>> mFontCache;

public:
    TextWriter() = default;

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

    /**
     * Write the text at the given position, size, color, and alignment.
     */
    void write(std::string const &text,
               Vector2 position,
               float fontSize,
               Color color,
               Alignment horizontal,
               Alignment vertical);
};
