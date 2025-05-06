
#include "textwriter.h"
#include "util.h"

namespace {
    // Extra spacing between letters.
    constexpr float SPACING = 0.0f;
}

std::shared_ptr<Font> TextWriter::getFont(float fontSize) {
    std::shared_ptr<Font> font;

    // Loading a font is always at integer sizes.
    int roundedFontSize = static_cast<int>(fontSize + 0.5);

    if (mFontCache.contains(roundedFontSize)) {
        font = mFontCache.at(roundedFontSize);
    } else {
        font = makeFontSharedPtr(LoadFontEx("FreeSans.ttf", roundedFontSize, nullptr, 0));
        mFontCache[roundedFontSize] = font;
    }

    return font;
}

void TextWriter::write(std::string const &text,
        Vector2 position,
        float fontSize,
        Color color,
        Alignment horizontal,
        Alignment vertical) {

    std::shared_ptr<Font> font = getFont(fontSize);

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

std::shared_ptr<Image> TextWriter::makeImage(std::string const &text, float fontSize, Color color) {
    std::shared_ptr<Font> font = getFont(fontSize);
    return makeImageSharedPtr(ImageTextEx(*font, text.c_str(), fontSize, SPACING, color));
}
