#include "platform/win/frontend.h"
#include "platform/win/input.h"

const signed char g_abCarShadowImageIds[2] = {99, 98};

const signed char g_abInputDeviceImageIds[4] = {50, 51, 49, 96};

const signed char g_abServiceProviderImageIds[4] = {69, 68, 67, 70};

const signed char g_abSpecularLightingImageIds[2] = {101, 100};

const signed char g_abSuspensionImageIds[3] = {13, 14, 15};

const signed char g_abTireImageIds[3] = {26, 25, 24};

const signed char g_abTransmissionImageIds[2] = {29, 12};

int g_adwCarMenuIndexToCarId[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

const int g_adwCarShadowMenuIndexToValue[2] = {1, 0};

const int g_adwForceFeedbackMenuIndexToValue[2] = {1, 0};

const int g_adwSkidMarksMenuIndexToValue[2] = {1, 0};

const int g_adwSpecularLightingMenuIndexToValue[2] = {1, 0};

int g_adwSuspensionMenuIndexToType[3] = {0, 1, 2};

int g_adwTireMenuIndexToTireType[3] = {0, 1, 2};

int g_adwTrackMenuIndexToTrackId[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

int g_adwTransmissionMenuIndexToType[2] = {1, 0};

int g_adwWeatherMenuIndexToWeather[5] = {0, 1, 2, 3, 4};

const int g_awSkidMarksImageIds[2] = {140, 141};

int g_bRaceSetupShowCarSetup = 1;

int g_dwCapturedKeyBindingValue = -1;

InputControlName g_aKeyboardControlNames[KEYBOARD_CONTROL_NAME_COUNT] = {
    {1, "ESCAPE"},
    {2, "1"},
    {3, "2"},
    {4, "3"},
    {5, "4"},
    {6, "5"},
    {7, "6"},
    {8, "7"},
    {9, "8"},
    {10, "9"},
    {11, "0"},
    {12, "-"},
    {13, "="},
    {14, "BACKSPACE"},
    {15, "TAB"},
    {16, "Q"},
    {17, "W"},
    {18, "E"},
    {19, "R"},
    {20, "T"},
    {21, "Y"},
    {22, "U"},
    {23, "I"},
    {24, "O"},
    {25, "P"},
    {26, "["},
    {27, "]"},
    {28, "ENTER"},
    {29, "LEFT CONTROL"},
    {30, "A"},
    {31, "S"},
    {32, "D"},
    {33, "F"},
    {34, "G"},
    {35, "H"},
    {36, "J"},
    {37, "K"},
    {38, "L"},
    {39, ";"},
    {40, "'"},
    {41, "`"},
    {42, "LEFT SHIFT"},
    {43, "\\"},
    {44, "Z"},
    {45, "X"},
    {46, "C"},
    {47, "V"},
    {48, "B"},
    {49, "N"},
    {50, "M"},
    {51, ","},
    {52, "."},
    {53, "/"},
    {54, "RIGHT SHIFT"},
    {55, "*"},
    {56, "LEFT ALT"},
    {57, "SPACE"},
    {58, "CAPS LOCK"},
    {59, "F1"},
    {60, "F2"},
    {61, "F3"},
    {62, "F4"},
    {63, "F5"},
    {64, "F6"},
    {65, "F7"},
    {66, "F8"},
    {67, "F9"},
    {68, "F10"},
    {69, "NUMLOCK"},
    {70, "SCROLL LOCK"},
    {71, "NUMPAD 7"},
    {72, "NUMPAD 8"},
    {73, "NUMPAD 9"},
    {74, "NUMPAD -"},
    {75, "NUMPAD 4"},
    {76, "NUMPAD 5"},
    {77, "NUMPAD 6"},
    {78, "NUMPAD +"},
    {79, "NUMPAD 1"},
    {80, "NUMPAD 2"},
    {81, "NUMPAD 3"},
    {82, "NUMPAD 0"},
    {83, "NUMPAD ."},
    {87, "F11"},
    {88, "F12"},
    {100, "F13"},
    {101, "F14"},
    {102, "F15"},
    {112, "KANA"},
    {121, "CONVERT"},
    {123, "NOCONVERT"},
    {125, "YEN"},
    {141, "NUMPAD ="},
    {144, "CIRCUMFLEX"},
    {145, "AT"},
    {146, ":"},
    {147, "_"},
    {148, "KANJI"},
    {149, "STOP"},
    {150, "AX"},
    {151, "UNLABELED"},
    {156, "NUMPAD ENTER"},
    {157, "RIGHT CONTROL"},
    {179, "NUMPAD ,"},
    {181, "/"},
    {183, "SYSRQ"},
    {184, "RIGHT ALT"},
    {199, "KEYPAD HOME"},
    {200, "UP ARROW"},
    {201, "KEYPAD PGUP"},
    {203, "LEFT ARROW"},
    {205, "RIGHT ARROW"},
    {207, "KEYPAD END"},
    {208, "DOWN ARROW"},
    {209, "KEYPAD PGDN"},
    {210, "KEYPAD INSERT"},
    {211, "KEYPAD DELETE"},
    {219, "LEFT WINDOWS"},
    {220, "RIGHT WINDOWS"},
    {221, "APP MENU"},
};

KeyCharacter g_KeyNameLookupTable[KEY_CHARACTER_COUNT] = {
    {32u, ' '},  {33u, '!'},  {34u, '"'},  {35u, '#'},  {36u, '$'},  {37u, '%'},  {38u, '&'},
    {39u, '\''}, {40u, '('},  {41u, ')'},  {42u, '*'},  {43u, '+'},  {44u, ','},  {45u, '-'},
    {46u, '.'},  {47u, '/'},  {186u, ':'}, {189u, '-'}, {190u, '.'}, {48u, '0'},  {49u, '1'},
    {50u, '2'},  {51u, '3'},  {52u, '4'},  {53u, '5'},  {54u, '6'},  {55u, '7'},  {56u, '8'},
    {57u, '9'},  {58u, ':'},  {59u, ';'},  {60u, '<'},  {61u, '='},  {62u, '>'},  {63u, '?'},
    {64u, '@'},  {65u, 'A'},  {66u, 'B'},  {67u, 'C'},  {68u, 'D'},  {69u, 'E'},  {70u, 'F'},
    {71u, 'G'},  {72u, 'H'},  {73u, 'I'},  {74u, 'J'},  {75u, 'K'},  {76u, 'L'},  {77u, 'M'},
    {78u, 'N'},  {79u, 'O'},  {80u, 'P'},  {81u, 'Q'},  {82u, 'R'},  {83u, 'S'},  {84u, 'T'},
    {85u, 'U'},  {86u, 'V'},  {87u, 'W'},  {88u, 'X'},  {89u, 'Y'},  {90u, 'Z'},  {91u, '['},
    {92u, '\\'}, {93u, ']'},  {94u, '^'},  {95u, '_'},  {96u, '`'},  {97u, 'a'},  {98u, 'b'},
    {99u, 'c'},  {100u, 'd'}, {101u, 'e'}, {102u, 'f'}, {103u, 'g'}, {104u, 'h'}, {105u, 'i'},
    {106u, 'j'}, {107u, 'k'}, {108u, 'l'}, {109u, 'm'}, {110u, 'n'}, {111u, 'o'}, {112u, 'p'},
    {113u, 'q'}, {114u, 'r'}, {115u, 's'}, {116u, 't'}, {117u, 'u'}, {118u, 'v'}, {119u, 'w'},
    {120u, 'x'}, {121u, 'y'}, {122u, 'z'}, {123u, '{'}, {124u, '|'}, {125u, '}'}, {126u, '~'},
};
