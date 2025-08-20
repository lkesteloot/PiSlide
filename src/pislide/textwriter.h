
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

    /**
     * Get or load (and cache) a font at the given size.
     */
    std::shared_ptr<Font> getFont(float fontSize);

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
     * Write the text at the given position, size, color, and alignment. Return the
     * rectangle on screen where the text was written.
     */
    Rectangle write(std::string const &text,
               Vector2 position,
               float fontSize,
               Color color,
               Alignment horizontal,
               Alignment vertical);

    /**
     * Make an image with the given text.
     */
    std::shared_ptr<Image> makeImage(std::string const &text,
               float fontSize,
               Color color);
};
