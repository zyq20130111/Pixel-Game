#pragma once

namespace pixel_world {

enum class Language {
    English,
    Chinese,
};

struct UiText {
    const char* mainTitle;
    const char* mainSubtitle;
    const char* continueButton;
    const char* newGameButton;
    const char* chapterButton;
    const char* settingsButton;
    const char* creditsButton;
    const char* exitButton;
    const char* mainHint;
    const char* difficultyTitle;
    const char* difficultySubtitle;
    const char* easyButton;
    const char* normalButton;
    const char* hardButton;
    const char* backButton;
    const char* loadingTitle;
    const char* loadingSubtitle;
    const char* loadingStatus;
    const char* chapterTitle;
    const char* chapterSubtitle;
    const char* chapterItem;
    const char* settingsTitle;
    const char* settingsSubtitle;
    const char* settingsItem;
    const char* creditsTitle;
    const char* creditsSubtitle;
    const char* creditsItem;
    const char* exitPromptTitle;
    const char* exitPromptSubtitle;
    const char* confirmButton;
    const char* cancelButton;
};

const UiText& uiText(Language language);

}  // namespace pixel_world
