
#include "textwriter.h"

namespace {
    // Extra spacing between letters.
    constexpr float SPACING = 0.0f;

    // Unload and delete the font object.
    void deleteFont(const Font *font) {
        UnloadFont(*font);
        delete font;
    }
}

void TextWriter::write(std::string const &text,
        Vector2 position,
        float fontSize,
        Color color,
        Alignment horizontal,
        Alignment vertical) {

    std::shared_ptr<Font> font;

    if (mFontCache.contains(fontSize)) {
        font = mFontCache.at(fontSize);
    } else {
        auto allocatedFont = new Font(LoadFontEx("FreeSans.ttf",
            static_cast<int>(fontSize), nullptr, 0));
        font.reset(allocatedFont, deleteFont);
        mFontCache[fontSize] = font;
    }

    if (horizontal != Alignment::START || vertical != Alignment::START) {
        Vector2 size = MeasureTextEx(*font, text.c_str(), fontSize, SPACING);

        switch (horizontal) {
            case Alignment::START:
                // Nothing.
                break;

            case Alignment::CENTER:
                position.x -= size.x/2.0f;
                break;

            case Alignment::END:
                position.x -= size.x;
                break;
        }

        switch (vertical) {
            case Alignment::START:
                // Nothing.
                break;

            case Alignment::CENTER:
                position.y -= size.y/2.0f;
                break;

            case Alignment::END:
                position.y -= size.y;
                break;
        }
    }

    DrawTextEx(*font, text.c_str(), position, fontSize, SPACING, color);
}
