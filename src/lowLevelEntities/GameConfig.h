#ifndef GAMECONFIG_H
#define GAMECONFIG_H

namespace GameConfig {
    constexpr int WINDOW_WIDTH = 683;
    constexpr int WINDOW_HEIGHT = 384;
    constexpr int BLACKJACK_VALUE = 21;
    constexpr int DEALER_STAND_VALUE = 17;
    constexpr int ACE_HIGH_VALUE = 11;
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
    constexpr float PLAYER_ICON_SCALE = 0.18f;
    // Bot behavior constants
    constexpr double BURST_RISK_WEIGHT = 0.5;
    constexpr double OPPONENT_PRESSURE_WEIGHT = 0.70;
    constexpr double LOSING_PRESSURE_WEIGHT = 0.15;
    
}

#endif
