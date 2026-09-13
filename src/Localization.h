#pragma once

namespace pixel_world {

enum class Language {
    English,
    Chinese,
};

struct UiText {
    const char* loginTitle;
    const char* loginSubtitle;
    const char* loginButton;
    const char* exitButton;
    const char* loginHint;
    const char* exitPromptTitle;
    const char* exitPromptSubtitle;
    const char* confirmButton;
    const char* cancelButton;
};

const UiText& uiText(Language language);

}  // namespace pixel_world
