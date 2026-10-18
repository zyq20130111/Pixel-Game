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

struct ParkingText {
    const char* title;
    const char* securityLabel;
    const char* captainLabel;
    const char* captainDown;
    const char* captainAlive;
    const char* accessCardLabel;
    const char* accessCardObtained;
    const char* accessCardRequired;
    const char* healthLabel;
    const char* playerDownPrompt;
    const char* levelCompletePrompt;
    const char* elevatorCardPrompt;
    const char* elevatorCardHint;
    const char* elevatorPrompt;
    const char* eliminateCaptainPrompt;
    const char* collectAccessCardPrompt;
    const char* reachElevatorPrompt;
    const char* elevatorLabel;
    const char* cardReaderLabel;
    const char* entryLabel;
    const char* carLabel;
    const char* truckLabel;
    const char* accessCardWorldLabel;
};

struct SecurityText {
    const char* title;
    const char* powerLabel;
    const char* powerOn;
    const char* powerOff;
    const char* alarmPrompt;
    const char* completePrompt;
    const char* switchPrompt;
    const char* objectivePrompt;
    const char* gateLabel;
    const char* switchLabel;
    const char* exitLabel;
    const char* healthLabel;
    const char* playerDownPrompt;
};

struct WarehouseText {
    const char* title;
    const char* healthLabel;
    const char* playerDownPrompt;
    const char* completePrompt;
    const char* objectivePrompt;
    const char* climbPrompt;
    const char* descendPrompt;
    const char* sniperCratePrompt;
    const char* forkliftPrompt;
    const char* forkliftDrivingPrompt;
    const char* exitPrompt;
    const char* doorBlockedPrompt;
    const char* ironDoorLabel;
    const char* platformLabel;
    const char* forkliftLabel;
    const char* shelfLabel;
    const char* crateLabel;
    const char* sniperAcquiredLabel;
    const char* entryLabel;
    const char* exitLabel;
};
const UiText& uiText(Language language);
const ParkingText& parkingText(Language language);
const SecurityText& securityText(Language language);
const WarehouseText& warehouseText(Language language);

}  // namespace pixel_world
