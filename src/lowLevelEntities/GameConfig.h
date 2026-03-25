#ifndef GAMECONFIG_H
#define GAMECONFIG_H

namespace GameConfig {
    constexpr int WINDOW_WIDTH = 683;
    constexpr int WINDOW_HEIGHT = 384;
    constexpr int BLACKJACK_VALUE = 21;
    constexpr int DEALER_STAND_VALUE = 17;
    constexpr int ACE_HIGH_VALUE = 11;
    constexpr int ACE_MID_VALUE = 10;
    constexpr int ACE_LOW_VALUE = 1;
    constexpr int FACE_CARD_VALUE = 10;
    constexpr int CARD_RANKS = 13;
    constexpr int HAND_START_VALUE = 2;
    constexpr int WINNING_POINTS = 15;
    constexpr int POINTS_GAIN_WON = 2;
    constexpr int POINTS_GAIN_TIE = 1;
    constexpr int POINTS_GAIN_BLACKJACK = 3;
    constexpr int POINTS_GAIN_QUINTET = 5;
    constexpr int REACTIVE_PROMPT_DURATION_DEFAULT = 12;
    constexpr int ACTION_PROMPT_DURATION = 30;
    constexpr int TARGET_PROMPT_DURATION = 20;
    constexpr float PLAYER_ICON_SCALE = 0.20f;
    // Bot behavior constants
    constexpr double BURST_RISK_WEIGHT = 0.5;
    constexpr double OPPONENT_PRESSURE_WEIGHT = 0.70;
    constexpr double LOSING_PRESSURE_WEIGHT = 0.15;
    const std::string BUST_TEXT = "BUSTED! T_T";
    constexpr bool RANDOM_SKILLS_EACH_ROUND_DEFAULT = true;
}

// Runtime settings — modifiable from the options menu
struct GameSettings {
    bool randomSkillsEachRound = GameConfig::RANDOM_SKILLS_EACH_ROUND_DEFAULT;
    bool smallWindow = false;  // false = 90% desktop, true = 683x384

    // UI scale factor: 1.0 for classic 683x384, ~2.5 for 90% desktop
    float S = 2.5f;

    void recalcScale(unsigned int windowW) {
        S = smallWindow ? 1.0f : (float)windowW / (float)GameConfig::WINDOW_WIDTH;
    }

    static GameSettings& instance() {
        static GameSettings s;
        return s;
    }
};

#endif
