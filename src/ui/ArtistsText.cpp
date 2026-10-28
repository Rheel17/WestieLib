#include "ui/ArtistsText.h"

#include "platform/win32/Utf16.h"

namespace westie::ui {

std::wstring artists_text(const std::vector<std::string>& artists) {
    std::wstring text;
    for (const std::string& artist : artists) {
        if (!text.empty()) {
            text += L", ";
        }
        text += platform::win32::to_utf16(artist);
    }
    return text;
}

}
