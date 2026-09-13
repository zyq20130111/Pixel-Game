#include "Localization.h"

namespace pixel_world {
namespace {

constexpr UiText kEnglishText{
    "PIXEL WORLD 3D",
    "ADVENTURE AWAITS",
    "LOGIN GAME",
    "EXIT GAME",
    "WASD TO MOVE  ESC TO QUIT",
    "EXIT GAME",
    "ARE YOU SURE",
    "YES",
    "NO",
};

// Keep these literals ASCII-only so the source file encoding cannot corrupt
// the UTF-8 bytes consumed by the renderer.
constexpr UiText kChineseText{
    "\xE5\x83\x8F\xE7\xB4\xA0\xE4\xB8\x96\xE7\x95\x8C 3D",
    "\xE5\x86\x92\xE9\x99\xA9\xE5\xBC\x80\xE5\xA7\x8B",
    "\xE5\xBC\x80\xE5\xA7\x8B\xE6\xB8\xB8\xE6\x88\x8F",
    "\xE9\x80\x80\xE5\x87\xBA\xE6\xB8\xB8\xE6\x88\x8F",
    "WASD \xE7\xA7\xBB\xE5\x8A\xA8  ESC \xE9\x80\x80\xE5\x87\xBA",
    "\xE9\x80\x80\xE5\x87\xBA\xE6\xB8\xB8\xE6\x88\x8F",
    "\xE7\xA1\xAE\xE5\xAE\x9A\xE9\x80\x80\xE5\x87\xBA\xE5\x90\x97",
    "\xE6\x98\xAF",
    "\xE5\x90\xA6",
};

}  // namespace

const UiText& uiText(Language language) {
    return language == Language::Chinese ? kChineseText : kEnglishText;
}

}  // namespace pixel_world
