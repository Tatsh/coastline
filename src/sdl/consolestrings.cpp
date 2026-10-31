#include "consolestrings.h"

namespace coastline {

// Each row lists the key, then the lines of the USA, the European, and the Japanese release. A
// release that draws the same message from more than one place has the lines of the first.
const std::array<ConsoleString, 147> CONSOLE_STRINGS{{
    {"n64.controllerCheck.controllerNotFound", {0x802a748c}, {0x802a8edc}, {0x802a88cc}},
    {"n64.controllerCheck.controllerPakNotFound",
     {0x802a74a4, 0x802a74c0, 0x802a74e0},
     {0x802a8ef4, 0x802a8f10, 0x802a8f34, 0x802a8f48},
     {0x802a88e0, 0x802a88f8, 0x802a8910, 0x802a892c}},
    {"n64.controllerCheck.otherGameData",
     {0x802a74fc, 0x802a751c, 0x802a7540, 0x802a7564, 0x802a7588},
     {0x802a8f64, 0x802a8f84, 0x802a8fa8, 0x802a8fcc, 0x802a8ff0},
     {0x802a8940, 0x802a8950, 0x802a8964, 0x802a8978, 0x802a8988}},
    {"n64.carSelect.rumblePakNote",
     {0x802a7c1c, 0x802a7c34, 0x802a7c4c, 0x802a7c68, 0x802a7c80},
     {0x802a967c, 0x802a9694, 0x802a96ac, 0x802a96c8, 0x802a96e0},
     {0x802a8fb8, 0x802a8fc8, 0x802a8fd4}},
    {"n64.carSelect.rumblePakNoteOneLine",
     {0x802a7d9c, 0x802a7dd0},
     {0x802a97fc, 0x802a9830},
     {0x802a90a8}},
    {"n64.playerSelect.twoControllers",
     {0x802a8008, 0x802a801c, 0x802a8030},
     {0x802a9a78, 0x802a9a8c, 0x802a9aa0},
     {0x802a92e8, 0x802a92f8, 0x802a9308}},
    {"n64.saveLoad.saveSeasonDataQuestion", {0x802a81dc}, {}, {}},
    {"n64.saveLoad.saveGhostCarDataQuestion", {0x802a81f0}, {}, {}},
    {"n64.saveLoad.saveSeasonToGamePak", {}, {0x802a9cc4}, {0x802a94a4}},
    {"n64.saveLoad.saveSeasonToControllerPak", {}, {0x802a9ce8}, {0x802a94c4}},
    {"n64.saveLoad.saveGhostCarToControllerPak", {}, {0x802a9d1c}, {0x802a94e8}},
    {"n64.saveLoad.newControllerPak",
     {0x802a823c, 0x802a8254, 0x802a8264, 0x802a8278},
     {0x802a9e48, 0x802a9e60, 0x802a9e70, 0x802a9e84},
     {0x802a9548, 0x802a955c, 0x802a956c, 0x802a957c}},
    {"n64.saveLoad.confirmErase",
     {},
     {0x802a9fbc, 0x802a9fdc, 0x802a9ffc},
     {0x802a9584, 0x802a95a8}},
    {"n64.saveLoad.confirmOverwrite",
     {0x802a83b0, 0x802a83c0},
     {0x802aa030, 0x802aa040},
     {0x802a95d4}},
    {"n64.saveLoad.controllerPakBroken",
     {0x802a8478, 0x802a848c},
     {0x802aa0f8, 0x802aa10c},
     {0x802a95f4, 0x802a960c}},
    {"n64.saveLoad.gamePakNotAccessible", {}, {0x802aa120}, {0x802a9628}},
    {"n64.saveLoad.controllerPakNotFound", {0x802a84a0}, {0x802aa144}, {0x802a963c}},
    {"n64.saveLoad.dataCannotBeLoaded", {0x802a84bc}, {0x802aa160}, {0x802a9650}},
    {"n64.saveLoad.dataCannotBeSaved", {0x802a84d4}, {0x802aa178}, {0x802a9660}},
    {"n64.saveLoad.removeRumblePak",
     {0x802a84ec, 0x802a850c, 0x802a852c, 0x802a8548},
     {0x802aa190, 0x802aa1b0, 0x802aa1d0, 0x802aa1ec},
     {0x802a9670, 0x802a967c, 0x802a968c}},
    {"n64.saveLoad.seasonPagesFull",
     {0x802a8578, 0x802a8590, 0x802a85a8, 0x802a85b8, 0x802a85d0, 0x802a85e8},
     {0x802aa21c, 0x802aa234, 0x802aa24c, 0x802aa25c, 0x802aa274, 0x802aa28c},
     {0x802a96ac, 0x802a96bc, 0x802a96cc, 0x802a96e0}},
    {"n64.saveLoad.ghostCarPagesFull",
     {0x802a85f8, 0x802a8610, 0x802a8628, 0x802a8638, 0x802a8650, 0x802a8668},
     {0x802aa29c, 0x802aa2b4, 0x802aa2cc, 0x802aa2dc, 0x802aa2f4, 0x802aa30c},
     {0x802a96ec, 0x802a96fc, 0x802a970c, 0x802a9720}},
    {"n64.saveLoad.optionsPagesFull",
     {0x802a8678, 0x802a8690, 0x802a86a8, 0x802a86b8, 0x802a86d0, 0x802a86e8},
     {0x802aa31c, 0x802aa334, 0x802aa34c, 0x802aa35c, 0x802aa374, 0x802aa38c},
     {0x802a9730, 0x802a9740, 0x802a9750, 0x802a9764}},
    {"n64.saveLoad.ghostCarTooLarge",
     {0x802a86f4, 0x802a8714, 0x802a8734, 0x802a8750},
     {0x802aa398, 0x802aa3b8, 0x802aa3d8, 0x802aa3f4},
     {0x802a9770, 0x802a9780, 0x802a978c, 0x802a979c}},
    {"n64.saveLoad.controllerError",
     {0x802a8768, 0x802a877c},
     {0x802aa40c, 0x802aa420},
     {0x802a97ac, 0x802a97bc}},
    {"n64.saveLoad.gamePakDataNotFound", {}, {0x802aa434, 0x802aa44c}, {0x802a97c8, 0x802a97d4}},
    {"n64.saveLoad.gamePakWriteFailed", {}, {0x802aa45c, 0x802aa474}, {0x802a97e4, 0x802a97f0}},
    {"n64.saveLoad.controllerPakDataNotFound",
     {0x802a8790, 0x802a87a8},
     {0x802aa488, 0x802aa4a0},
     {0x802a9800, 0x802a9810}},
    {"n64.saveLoad.controllerPakWriteFailed",
     {0x802a87bc, 0x802a87d0},
     {0x802aa4b4, 0x802aa4c8},
     {0x802a9820, 0x802a9830}},
    {"n64.saveLoad.controllerPakError",
     {0x802a8830, 0x802a8848},
     {0x802aa528, 0x802aa540},
     {0x802a9878, 0x802a9884}},
    {"n64.saveLoad.seasonLoaded", {0x802a885c, 0x802a886c}, {0x802aa554, 0x802aa564}, {0x802a9890}},
    {"n64.saveLoad.ghostCarLoaded",
     {0x802a887c, 0x802a888c},
     {0x802aa574, 0x802aa584},
     {0x802a98a0}},
    {"n64.saveLoad.seasonSaved", {0x802a889c, 0x802a88ac}, {0x802aa594, 0x802aa5a4}, {0x802a98b0}},
    {"n64.saveLoad.ghostCarSaved",
     {0x802a88bc, 0x802a88d0},
     {0x802aa5b4, 0x802aa5c8},
     {0x802a98c0}},
    {"n64.saveLoad.optionsLoaded", {0x802a88e0}, {0x802aa5d8}, {0x802a98d0}},
    {"n64.saveLoad.optionsSaved", {0x802a88f8}, {0x802aa5f0}, {0x802a98e0}},
    {"n64.saveLoad.insertRumblePak",
     {0x802a8910, 0x802a8934, 0x802a8954, 0x802a8974, 0x802a8994},
     {0x802aa614, 0x802aa638, 0x802aa658, 0x802aa678, 0x802aa698},
     {0x802a98fc, 0x802a990c, 0x802a991c}},
    {"n64.rumblePakPrompt.insertRumblePak",
     {0x802a89cc, 0x802a89f0, 0x802a8a10, 0x802a8a30, 0x802a8a50},
     {0x802aa6d0, 0x802aa6f4, 0x802aa714, 0x802aa734, 0x802aa754},
     {0x802a9930, 0x802a9940, 0x802a9950}},
    {"n64.pakData.newControllerPak",
     {0x802a8a84, 0x802a8a9c, 0x802a8aac, 0x802a8ac0},
     {0x802aa788, 0x802aa7a0, 0x802aa7b0, 0x802aa7c4},
     {0x802a9964, 0x802a9978, 0x802a9988, 0x802a9998}},
    {"n64.pakData.controllerPakBroken",
     {0x802a8bd8, 0x802a8bec},
     {0x802aa8dc, 0x802aa8f0},
     {0x802a99a0, 0x802a99b8}},
    {"n64.pakData.controllerPakNotFound", {0x802a8c00}, {0x802aa904}, {0x802a99d4}},
    {"n64.pakData.dataCannotBeSaved", {0x802a8c1c}, {0x802aa920}, {0x802a99e8}},
    {"n64.pakData.dataCannotBeLoaded", {0x802a8c34}, {0x802aa938}, {0x802a99f8}},
    {"n64.pakData.removeRumblePak",
     {0x802a8c4c, 0x802a8c6c, 0x802a8c8c, 0x802a8ca8},
     {0x802aa950, 0x802aa970, 0x802aa990, 0x802aa9ac},
     {0x802a9a08, 0x802a9a14, 0x802a9a24}},
    {"n64.pakData.optionsPagesFull",
     {0x802a8cd8, 0x802a8cf0, 0x802a8d08, 0x802a8d18, 0x802a8d30, 0x802a8d48},
     {0x802aa9dc, 0x802aa9f4, 0x802aaa0c, 0x802aaa1c, 0x802aaa34, 0x802aaa4c},
     {0x802a9a44, 0x802a9a54, 0x802a9a64, 0x802a9a78}},
    {"n64.pakData.controllerError",
     {0x802a8d54, 0x802a8d68},
     {0x802aaa58, 0x802aaa6c},
     {0x802a9a84, 0x802a9a94}},
    {"n64.pakData.controllerPakError",
     {0x802a8d7c, 0x802a8d94},
     {0x802aaa80, 0x802aaa98},
     {0x802a9aa0, 0x802a9aac}},
    {"n64.pakData.controllerPakDataNotFound",
     {0x802a8da8, 0x802a8dc0},
     {0x802aaaac, 0x802aaac4},
     {0x802a9ab8, 0x802a9ac8}},
    {"n64.pakData.newControllerPakError",
     {0x802a8e20, 0x802a8e38},
     {0x802aab24, 0x802aab3c},
     {0x802a9b14, 0x802a9b20}},
    {"n64.pakData.seasonLoaded", {0x802a8e4c, 0x802a8e5c}, {0x802aab50, 0x802aab60}, {0x802a9b2c}},
    {"n64.pakData.ghostCarLoaded",
     {0x802a8e6c, 0x802a8e7c},
     {0x802aab70, 0x802aab80},
     {0x802a9b3c}},
    {"n64.pakData.configurationLoaded", {0x802a8e8c, 0x802a8ea0}, {}, {}},
    {"n64.pakData.configurationSaved", {0x802a8e8c, 0x802a8eb0}, {}, {}},
    {"n64.paintShop.controllerPakNotInserted",
     {0x802aa6d8, 0x802aa6e8},
     {0x802ac428, 0x802ac438},
     {0x802ab220, 0x802ab230}},
    {"n64.paintShop.decalPagesFull",
     {0x802aa6f8, 0x802aa710, 0x802aa728, 0x802aa738, 0x802aa750, 0x802aa768},
     {0x802ac448, 0x802ac460, 0x802ac478, 0x802ac488, 0x802ac4a0, 0x802ac4b8},
     {0x802ab23c, 0x802ab24c, 0x802ab25c, 0x802ab26c}},
    {"n64.paintShop.readFailed",
     {0x802aa778, 0x802aa7a0},
     {0x802ac4c8, 0x802ac4f0},
     {0x802ab27c, 0x802ab288}},
    {"n64.paintShop.writeFailed",
     {0x802aa78c, 0x802aa7a0},
     {0x802ac4dc, 0x802ac4f0},
     {0x802ab298, 0x802ab2a4}},
    {"n64.paintShop.decalsNotFound",
     {0x802aa7b4, 0x802aa7c8},
     {0x802ac504, 0x802ac518},
     {0x802ab2b4, 0x802ab2c4}},
    {"n64.paintShop.saveFailed",
     {0x802aa7dc, 0x802aa7f0},
     {0x802ac52c, 0x802ac540},
     {0x802ab2d4, 0x802ab2e0}},
    {"n64.paintShop.loadTransferFailed",
     {0x802aa804, 0x802aa818},
     {0x802ac554, 0x802ac568},
     {0x802ab2f0, 0x802ab2fc}},
    {"n64.paintShop.saveTransferFailed",
     {0x802aa804, 0x802aa830},
     {0x802ac554, 0x802ac580},
     {0x802ab30c, 0x802ab318}},
    {"n64.paintShop.loadBadData",
     {0x802aa844, 0x802aa85c},
     {0x802ac594, 0x802ac5ac},
     {0x802ab328, 0x802ab338}},
    {"n64.paintShop.saveBadData",
     {0x802aa844, 0x802aa874},
     {0x802ac594, 0x802ac5c4},
     {0x802ab348, 0x802ab358}},
    {"n64.paintShop.loadControllerError",
     {0x802aa888, 0x802aa8a0, 0x802aa8b0, 0x802aa8c8},
     {0x802ac5d8, 0x802ac5f0, 0x802ac600, 0x802ac618},
     {0x802ab368, 0x802ab378, 0x802ab380, 0x802ab388}},
    {"n64.paintShop.saveControllerError",
     {0x802aa888, 0x802aa8a0, 0x802aa8b0, 0x802aa8d4},
     {0x802ac5d8, 0x802ac5f0, 0x802ac600, 0x802ac624},
     {0x802ab368, 0x802ab378, 0x802ab394, 0x802ab39c}},
    {"n64.paintShop.controllerPakBroken",
     {0x802aa8e0, 0x802aa8f0},
     {0x802ac630, 0x802ac640},
     {0x802ab3a8, 0x802ab3c0}},
    {"n64.paintShop.clearDecalToSelectedColor",
     {0x802aa904, 0x802aa92c},
     {0x802ac654, 0x802ac67c},
     {0x802ab3dc, 0x802ab3e8}},
    {"n64.paintShop.clearDecalToBodyColor",
     {0x802aa904, 0x802aa940},
     {0x802ac654, 0x802ac690},
     {0x802ab3fc, 0x802ab408}},
    {"n64.paintShop.clearAllDecalsToSelectedColor",
     {0x802aa918, 0x802aa92c},
     {0x802ac668, 0x802ac67c},
     {0x802ab41c, 0x802ab428}},
    {"n64.paintShop.clearAllDecalsToBodyColor",
     {0x802aa918, 0x802aa940},
     {0x802ac668, 0x802ac690},
     {0x802ab43c, 0x802ab448}},
    {"n64.paintShop.flipDecalHorizontally",
     {0x802aa97c, 0x802aa9a4},
     {0x802ac6cc, 0x802ac6f4},
     {0x802ab488, 0x802ab494}},
    {"n64.paintShop.flipDecalVertically",
     {0x802aa990, 0x802aa9b4},
     {0x802ac6e0, 0x802ac704},
     {0x802ab4a0, 0x802ab4ac}},
    {"n64.paintShop.mirrorToRightDoor",
     {0x802aaa04, 0x802aaa1c},
     {0x802ac710, 0x802ac728},
     {0x802ab4b8, 0x802ab4c4}},
    {"n64.paintShop.mirrorToLeftDoor",
     {0x802aaa30, 0x802aaa48},
     {0x802ac73c, 0x802ac754},
     {0x802ab4d4, 0x802ab4e0}},
    {"n64.paintShop.mirrorToRightFrontFender",
     {0x802aaa5c, 0x802aaa74},
     {0x802ac768, 0x802ac780},
     {0x802ab4f0, 0x802ab500}},
    {"n64.paintShop.mirrorToLeftFrontFender",
     {0x802aaa8c, 0x802aaaa4},
     {0x802ac798, 0x802ac7b0},
     {0x802ab510, 0x802ab520}},
    {"n64.paintShop.mirrorToRightRearFender",
     {0x802aaabc, 0x802aaad4},
     {0x802ac7c8, 0x802ac7e0},
     {0x802ab530, 0x802ab540}},
    {"n64.paintShop.mirrorToLeftRearFender",
     {0x802aaaec, 0x802aab04},
     {0x802ac7f8, 0x802ac810},
     {0x802ab550, 0x802ab560}},
    {"n64.paintShop.mirrorNotApplicable",
     {0x802aab1c, 0x802aab38},
     {0x802ac86c, 0x802ac888},
     {0x802ab5b4, 0x802ab5c0}},
    {"n64.paintShop.loadDecalsQuestion", {0x802aab94}, {0x802ac8e4}, {0x802ab5e8}},
    {"n64.paintShop.saveDecalsQuestion", {0x802aaba4}, {0x802ac8f4}, {0x802ab5fc}},
    {"n64.paintShop.confirmOverwrite",
     {0x802aac14, 0x802aac24},
     {0x802ac93c, 0x802ac94c},
     {0x802ab624}},
    {"n64.paintShop.loadingDecals", {0x802aac48}, {0x802ac970}, {0x802ab644}},
    {"n64.paintShop.savingDecals", {0x802aac5c}, {0x802ac984}, {0x802ab650}},
    {"n64.paintShop.newControllerPak",
     {0x802aad30, 0x802aad48, 0x802aad58, 0x802aad6c},
     {0x802aca60, 0x802aca78, 0x802aca88, 0x802aca9c},
     {0x802ab670, 0x802ab684, 0x802ab694, 0x802ab6a4}},
    {"n64.paintShop.removeRumblePak",
     {0x802aad7c, 0x802aad9c, 0x802aadbc, 0x802aaddc},
     {0x802acaac, 0x802acacc, 0x802acaec, 0x802acb0c},
     {0x802ab6ac, 0x802ab6b8, 0x802ab6c8}},
    {"n64.paintShop.insertRumblePak",
     {0x802aae04, 0x802aae28, 0x802aae48, 0x802aae68, 0x802aae88},
     {0x802acb34, 0x802acb58, 0x802acb78, 0x802acb98, 0x802acbb8},
     {0x802ab6e8, 0x802ab6f8, 0x802ab708}},
    {"n64.paintShop.exitWithoutSaving",
     {0x802ab088, 0x802ab09c},
     {0x802acdb8, 0x802acdcc},
     {0x802ab810, 0x802ab81c}},
    {"n64.pakMenu.gameNote", {0x802ab290}, {0x802acfa0}, {0x802ab9f0}},
    {"n64.pakMenu.pages", {0x802ab2a0}, {0x802acfb0}, {0x802ab9fc}},
    {"n64.pakMenu.controllerPakError",
     {0x802ab2e4, 0x802ab2fc},
     {0x802acff4, 0x802ad00c},
     {0x802aba44, 0x802aba54}},
    {"n64.pakMenu.newControllerPak",
     {0x802ab314, 0x802ab32c, 0x802ab340, 0x802ab358},
     {0x802ad024, 0x802ad03c, 0x802ad050, 0x802ad068},
     {0x802aba64, 0x802aba78, 0x802aba88, 0x802aba98}},
    // The race, which the PC release draws with the strings of LIBRARY_STRING_KEYS.
    {"race.placeSuffixSt", {0x802aa114}, {0x802abe64}, {0x802aacf4}},
    {"race.placeSuffixNd", {0x802aa118}, {0x802abe68}, {0x802aacf8}},
    {"race.placeSuffixRd", {0x802aa11c}, {0x802abe6c}, {0x802aacfc}},
    {"race.placeSuffixTh", {0x802aa120}, {0x802abe70}, {0x802aad00}},
    {"race.lapLabel", {0x802aa0fc}, {0x802abe4c}, {0x802aacdc}},
    {"race.finished", {0x802aa104}, {0x802abe54}, {0x802aace4}},
    {"race.totalTimeLabel", {0x802aa050}, {0x802abda0}, {0x802aac30}},
    {"race.bestLapLabel", {0x802aa064}, {0x802abdb4}, {0x802aac44}},
    {"race.lapTimeLabel", {0x802aa074}, {0x802abdc4}, {0x802aac54}},
    {"race.timeLeftLabel", {0x802aa0a8}, {0x802abdf8}, {0x802aac88}},
    // The units of the Japanese release (Mile/h at 0x802aad1c, Km/h at 0x802aad28) are wider than
    // the place the speedometer of the PC release leaves them, and Japanese takes those of en-us.
    {"race.speedUnitMph", {0x802aa13c}, {0x802abe8c}, {}},
    {"race.speedUnitKph", {0x802aa144}, {0x802abe94}, {}},
    {"race.newRecord", {0x802a6e9c}, {0x802a896c}, {0x802a841c}},
    {"race.seasonWinner", {0x802a6eb4}, {0x802a8984}, {0x802a8434}},
    {"race.champion", {0x802a6ec8}, {0x802a8998}, {0x802a8448}},
    {"race.instantReplay", {0x802a6ed8}, {0x802a89a8}, {}},
    {"race.wrongWay", {0x8028b308}, {0x8028d5b8}, {0x8028cfe8}},
    {"race.pause.paused", {0x802a6f10}, {0x802a89e0}, {0x802a8478}},
    {"race.pause.continueSelected", {0x802a6f1c}, {0x802a89ec}, {0x802a8484}},
    {"race.pause.continue", {0x802a6f28}, {0x802a89f8}, {0x802a8490}},
    {"race.pause.restartRaceSelected", {0x802a6f34}, {0x802a8a04}, {0x802a849c}},
    {"race.pause.restartRace", {0x802a6f44}, {0x802a8a14}, {0x802a84ac}},
    {"race.pause.sfxVolume", {0x802a6f70}, {0x802a8a40}, {0x802a84d8}},
    {"race.newLapRecord", {0x802a98a8}, {0x802ab5e8}, {0x802aa578}},
    {"race.bestLap", {0x802a98bc}, {0x802ab5fc}, {0x802aa58c}},
    {"race.lapNumber", {0x802a9920}, {0x802ab660}, {0x802aa5f0}},
    {"race.place.first", {0x802a992c}, {0x802ab66c}, {0x802aa5fc}},
    {"race.place.second", {0x802a9938}, {0x802ab678}, {0x802aa608}},
    {"race.place.third", {0x802a9944}, {0x802ab684}, {0x802aa614}},
    {"race.place.fourth", {0x802a9950}, {0x802ab690}, {0x802aa620}},
    {"race.place.fifth", {0x802a995c}, {0x802ab69c}, {0x802aa62c}},
    {"race.place.sixth", {0x802a9968}, {0x802ab6a8}, {0x802aa638}},
    {"race.place.place7", {0x802a9974}, {0x802ab6b4}, {0x802aa644}},
    {"race.place.place8", {0x802a997c}, {0x802ab6bc}, {0x802aa64c}},
    {"race.place.place9", {0x802a9984}, {0x802ab6c4}, {0x802aa654}},
    {"race.place.place10", {0x802a998c}, {0x802ab6cc}, {0x802aa65c}},
    {"race.place.place11", {0x802a9994}, {0x802ab6d4}, {0x802aa664}},
    {"race.place.place12", {0x802a999c}, {0x802ab6dc}, {0x802aa66c}},
    {"race.place.place13", {0x802a99a4}, {0x802ab6e4}, {0x802aa674}},
    {"race.place.place14", {0x802a99ac}, {0x802ab6ec}, {0x802aa67c}},
    {"race.place.place15", {0x802a99b4}, {0x802ab6f4}, {0x802aa684}},
    {"race.place.place16", {0x802a99bc}, {0x802ab6fc}, {0x802aa68c}},
    {"race.place.place17", {0x802a99c4}, {0x802ab704}, {0x802aa694}},
    {"race.place.place18", {0x802a99cc}, {0x802ab70c}, {0x802aa69c}},
    {"race.place.place19", {0x802a99d4}, {0x802ab714}, {0x802aa6a4}},
    {"race.place.place20", {0x802a99dc}, {0x802ab71c}, {0x802aa6ac}},
    {"race.newCourseRecord", {0x802a99e4}, {0x802ab724}, {0x802aa6b4}},
    {"race.checkPoint", {0x802a99fc}, {0x802ab73c}, {0x802aa6cc}},
    {"race.wayBehind", {0x802a9a24}, {0x802ab764}, {0x802aa6f4}},
    {"race.behindSuffix", {0x802a9a3c}, {0x802ab77c}, {0x802aa70c}},
    {"race.aheadSuffix", {0x802a9a44}, {0x802ab784}, {0x802aa714}},
    // The race of the console alone, where the PC release has other text in the place.
    {"n64.race.pause.bgmVolume", {0x802a6f54}, {0x802a8a24}, {0x802a84bc}},
    {"n64.race.pause.exitToMainMenuSelected", {0x802a6f8c}, {0x802a8a5c}, {0x802a84f4}},
    {"n64.race.pause.exitToMainMenu", {0x802a6fa4}, {0x802a8a74}, {0x802a850c}},
    {"n64.race.replayExitHint", {0x802a6efc}, {0x802a89cc}, {0x802a8468}},
}};

// Each key follows the key of the message of the console in its place, under sdl.
const std::array<SaveFolderString, 10> SAVE_FOLDER_STRINGS{{
    {"sdl.saveLoad.seasonPagesFull",
     "INSUFFICIENT FREE PAGES\nOR FREE NOTES IN THE\nSAVE FOLDER.\nTWO PAGES AND ONE NOTE\n"
     "ARE NEEDED TO SAVE THE\nSEASON DATA."},
    {"sdl.saveLoad.ghostCarPagesFull",
     "INSUFFICIENT FREE PAGES\nOR FREE NOTES IN THE\nSAVE FOLDER.\n58 PAGES AND ONE NOTE\n"
     "ARE NEEDED TO SAVE THE\nGHOST CAR DATA."},
    {"sdl.saveLoad.optionsPagesFull",
     "INSUFFICIENT FREE PAGES\nOR FREE NOTES IN THE\nSAVE FOLDER.\nONE PAGE AND ONE NOTE\n"
     "ARE NEEDED TO SAVE THE\nOPTIONS."},
    {"sdl.saveLoad.ghostCarTooLarge",
     "UNABLE TO SAVE GHOST CAR DATA.\nGHOST CAR DATA IS LARGER THAN\nTHE MAXIMUM ALLOCATED SPACE\n"
     "IN THE SAVE FOLDER."},
    {"sdl.saveLoad.saveFolderDataNotFound", "SAVED DATA NOT FOUND\nIN THE SAVE FOLDER."},
    {"sdl.saveLoad.saveFolderWriteFailed", "UNABLE TO SAVE DATA\nTO THE SAVE FOLDER."},
    {"sdl.saveLoad.saveFolderError", "SAVE FOLDER ERROR\nHAS BEEN DETECTED!"},
    {"sdl.pakData.optionsPagesFull",
     "INSUFFICIENT FREE PAGES\nOR FREE NOTES IN THE\nSAVE FOLDER.\nONE PAGE AND ONE NOTE\n"
     "ARE NEEDED TO SAVE THE\nOPTIONS."},
    {"sdl.pakData.saveFolderDataNotFound", "SAVED DATA NOT FOUND\nIN THE SAVE FOLDER."},
    {"sdl.pakData.saveFolderError", "SAVE FOLDER ERROR\nHAS BEEN DETECTED!"},
}};

// Each key is followed by the number of its string.
const std::array<std::string_view, LIBRARY_STRING_COUNT> LIBRARY_STRING_KEYS{
    "pc.menu.mainMenu",                          // 1
    "pc.menu.championship",                      // 2
    "pc.menu.multiplayer",                       // 3
    "pc.menu.timeAttack",                        // 4
    "pc.menu.quickRace",                         // 5
    "pc.menu.options",                           // 6
    "pc.menu.credits",                           // 7
    "pc.menu.quit",                              // 8
    "pc.menu.championshipSeasonRaces",           // 9
    "pc.menu.loadSeason",                        // 10
    "pc.menu.newSeason",                         // 11
    "pc.menu.back",                              // 12
    "pc.menu.pleaseInsertDisc",                  // 13
    "pc.menu.quitGame",                          // 14
    "pc.menu.yesQuit",                           // 15
    "pc.menu.confirmExitSeason",                 // 16
    "pc.menu.yes",                               // 17
    "pc.menu.no",                                // 18
    "pc.menu.carSetup",                          // 19
    "pc.menu.car",                               // 20
    "pc.menu.tires",                             // 21
    "pc.menu.shocks",                            // 22
    "pc.menu.steering",                          // 23
    "pc.menu.transmission",                      // 24
    "pc.menu.startRace",                         // 25
    "pc.menu.raceSetup",                         // 26
    "pc.menu.track",                             // 27
    "pc.menu.weather",                           // 28
    "pc.menu.laps",                              // 29
    "pc.menu.continue",                          // 30
    "pc.menu.inputOptions",                      // 31
    "pc.menu.videoOptions",                      // 32
    "pc.menu.audioOptions",                      // 33
    "pc.menu.gameOptions",                       // 34
    "pc.menu.inputDevice",                       // 35
    "pc.menu.configure",                         // 36
    "pc.menu.restoreDefaults",                   // 37
    "pc.menu.gameKeyConfiguration",              // 38
    "pc.menu.changeKey",                         // 39
    "pc.menu.setting",                           // 40
    "pc.menu.direct3dDevice",                    // 41
    "pc.menu.gameResolution",                    // 42
    "pc.menu.direct3dModeSelection",             // 43
    "pc.menu.setDriver",                         // 44
    "pc.menu.setMode",                           // 45
    "pc.menu.sfxVolume",                         // 46
    "pc.menu.cdVolume",                          // 47
    "pc.menu.forceFeedback",                     // 48
    "pc.menu.skidMarks",                         // 49
    "pc.menu.specularLighting",                  // 50
    "pc.menu.carShadow",                         // 51
    "pc.menu.loadChampionshipSeason",            // 52
    "pc.menu.restoreAutosave",                   // 53
    "pc.menu.year",                              // 54
    "pc.menu.round",                             // 55
    "pc.menu.season",                            // 56
    "pc.menu.saveChampionshipSeason",            // 57
    "pc.menu.overwriteExistingGame",             // 58
    "pc.menu.newChampionshipSeason",             // 59
    "pc.menu.playerName",                        // 60
    "pc.menu.preRaceInformation",                // 61
    "pc.menu.bestTime",                          // 62
    "pc.menu.bestLap",                           // 63
    "pc.menu.points",                            // 64
    "pc.menu.total",                             // 65
    "pc.menu.pressAnyKeyToContinue",             // 66
    "pc.menu.notEnoughPointsToAdvance",          // 67
    "pc.menu.seasonProgressInformation",         // 68
    "pc.menu.resetRound",                        // 69
    "pc.menu.needed",                            // 70
    "pc.menu.raceResultsInformation",            // 71
    "pc.menu.lap1",                              // 72
    "pc.menu.lap2",                              // 73
    "pc.menu.lap3",                              // 74
    "pc.menu.totalTime",                         // 75
    "pc.menu.place",                             // 76
    "pc.menu.lapTime",                           // 77
    "pc.menu.raceResultsSummary",                // 78
    "pc.menu.raceResultsTotals",                 // 79
    "pc.menu.nextRace",                          // 80
    "pc.menu.exit",                              // 81
    "pc.menu.after",                             // 82
    "pc.menu.rounds",                            // 83
    "pc.network.multiPlayerConnection",          // 84
    "pc.network.ipx",                            // 85
    "pc.network.tcpIp",                          // 86
    "pc.network.modem",                          // 87
    "pc.network.serialCable",                    // 88
    "pc.network.additionalTcpIpInfo",            // 89
    "pc.network.portNumber",                     // 90
    "pc.network.ipAddress",                      // 91
    "pc.network.multiPlayerName",                // 92
    "pc.network.modemDialUp",                    // 93
    "pc.network.phoneNumber",                    // 94
    "pc.network.multiPlayerJoinCreateGame",      // 95
    "pc.network.join",                           // 96
    "pc.network.host",                           // 97
    "pc.network.multiPlayerCreateGame",          // 98
    "pc.network.gameName",                       // 99
    "pc.network.multiPlayerGameSetup",           // 100
    "pc.network.carGroup",                       // 101
    "pc.network.createGame",                     // 102
    "pc.network.quitToLobby",                    // 103
    "pc.network.bootPlayer",                     // 104
    "pc.network.ready",                          // 105
    "pc.menu.timeAttackMenu",                    // 106
    "pc.menu.newRace",                           // 107
    "pc.menu.loadRace",                          // 108
    "pc.menu.timeAttackResults",                 // 109
    "pc.menu.loadTimeAttack",                    // 110
    "pc.menu.raceTime",                          // 111
    "pc.menu.overwriteExistingGhostRace",        // 112
    "pc.menu.y",                                 // 113
    "pc.menu.n",                                 // 114
    "pc.menu.on",                                // 115
    "pc.menu.off",                               // 116
    "pc.menu.auto",                              // 117
    "pc.menu.manual",                            // 118
    "pc.menu.desert",                            // 119
    "pc.menu.mountain",                          // 120
    "pc.menu.coastline",                         // 121
    "pc.menu.stripMine",                         // 122
    "pc.menu.jungle",                            // 123
    "pc.menu.race",                              // 124
    "pc.menu.bonus",                             // 125
    "pc.menu.slippy",                            // 126
    "pc.menu.normal",                            // 127
    "pc.menu.grippy",                            // 128
    "pc.menu.softer",                            // 129
    "pc.menu.harder",                            // 130
    "pc.menu.sunny",                             // 131
    "pc.menu.foggy",                             // 132
    "pc.menu.rainy",                             // 133
    "pc.menu.snowy",                             // 134
    "pc.menu.night",                             // 135
    "pc.menu.keyboard",                          // 136
    "pc.menu.wheel",                             // 137
    "pc.menu.joystick",                          // 138
    "pc.menu.mouse",                             // 139
    "pc.menu.serial",                            // 140
    "pc.car.n4wdCe",                             // 141
    "pc.car.n4wdEs",                             // 142
    "pc.car.n4wdNs",                             // 143
    "pc.car.n4wdRs",                             // 144
    "pc.car.rwdSp",                              // 145
    "pc.car.n4wdPs",                             // 146
    "pc.car.rwdM3",                              // 147
    "pc.car.n4wdIp",                             // 148
    "pc.car.n4wdLd",                             // 149
    "pc.car.rwdHm",                              // 150
    "pc.car.rwdMt",                              // 151
    "pc.car.rwdCu",                              // 152
    "pc.car.rwdBb",                              // 153
    "pc.car.n4wdPj",                             // 154
    "pc.car.rwdTr",                              // 155
    "pc.car.fwdMn",                              // 156
    "pc.car.esLd",                               // 157
    "pc.car.ceIpMn",                             // 158
    "pc.car.m3TrSp",                             // 159
    "pc.car.rsPsPjNs",                           // 160
    "pc.car.cuBbHmMt",                           // 161
    "pc.error.couldNotAllocateRallySetupObject", // 162
    "pc.error.couldNotAllocateMenuUtilObject",   // 163
    "pc.error.fileLengthIs0",                    // 164
    "pc.error.nullFileName",                     // 165
    "pc.error.outOfMemory",                      // 166
    "pc.error.badReturnValue",                   // 167
    "pc.error.couldNotAllocateFileListObject",   // 168
    "pc.error.invalidSeasonFile",                // 169
    "pc.menu.frontEndTitle",                     // 170
    "pc.error.couldNotCreateDirectInputDevice",  // 171
    "pc.error.couldNotInitializeMouseDevice",    // 172
    "pc.error.couldNotSetMouseDataFormat",       // 173
    "pc.error.couldNotSetMouseCooperativeLevel", // 174
    "pc.menu.repeat",                            // 175
    "pc.menu.mirrored",                          // 176
    "pc.menu.undefined",                         // 177
    "pc.menu.pressAKey",                         // 178
    "race.placeSuffixSt",                        // 179
    "race.placeSuffixNd",                        // 180
    "race.placeSuffixRd",                        // 181
    "race.placeSuffixTh",                        // 182
    "pc.lobby.selectedCar",                      // 183
    "pc.lobby.changedTrackTo",                   // 184
    "pc.lobby.changedLapsTo",                    // 185
    "pc.lobby.changedWeatherTo",                 // 186
    "pc.lobby.youNeedSomeoneToRaceWith",         // 187
    "pc.lobby.notEveryoneIsReady",               // 188
    "pc.lobby.changedTheCarGroupTo",             // 189
    "pc.menu.emptySlot",                         // 190
    "pc.menu.roundInfo",                         // 191
    "pc.menu.player",                            // 192
    "pc.menu.myGame",                            // 193
    "pc.menu.saveTimeAttack",                    // 194
    "pc.controls.button",                        // 195
    "pc.controls.left",                          // 196
    "pc.controls.right",                         // 197
    "pc.controls.forward",                       // 198
    "pc.controls.backward",                      // 199
    "pc.controls.zNegative",                     // 200
    "pc.controls.zPositive",                     // 201
    "pc.controls.turnLeft",                      // 202
    "pc.controls.turnRight",                     // 203
    "pc.controls.accelerate",                    // 204
    "pc.controls.brake",                         // 205
    "pc.controls.reverse",                       // 206
    "pc.controls.shiftUp",                       // 207
    "pc.controls.shiftDown",                     // 208
    "pc.controls.resetCar",                      // 209
    "pc.controls.horn",                          // 210
    "pc.controls.taunt1",                        // 211
    "pc.controls.taunt2",                        // 212
    "pc.controls.taunt3",                        // 213
    "pc.controls.taunt4",                        // 214
    "pc.controls.raceMenu",                      // 215
    "pc.controls.menuUp",                        // 216
    "pc.controls.menuDown",                      // 217
    "pc.controls.menuPick",                      // 218
    "pc.controls.inCar",                         // 219
    "pc.controls.rearView",                      // 220
    "pc.controls.behindCar",                     // 221
    "pc.controls.trackside",                     // 222
    "pc.menu.spring",                            // 223
    "pc.menu.summer",                            // 224
    "pc.menu.fall",                              // 225
    "pc.menu.winter",                            // 226
    "pc.menu.pro",                               // 227
    "pc.menu.expert",                            // 228
    "race.lapLabel",                             // 229
    "race.finished",                             // 230
    "race.totalTimeLabel",                       // 231
    "race.bestLapLabel",                         // 232
    "race.lapTimeLabel",                         // 233
    "race.timeLeftLabel",                        // 234
    "race.speedUnitMph",                         // 235
    "race.speedUnitKph",                         // 236
    "pc.race.waitingForOthers",                  // 237
    "pc.race.pressSpaceToStart",                 // 238
    "race.newRecord",                            // 239
    "race.seasonWinner",                         // 240
    "race.champion",                             // 241
    "race.instantReplay",                        // 242
    "race.wrongWay",                             // 243
    "pc.race.pressEscToExit",                    // 244
    "race.pause.paused",                         // 245
    "pc.race.reallyQuitGame",                    // 246
    "pc.race.yesSelected",                       // 247
    "pc.race.yes",                               // 248
    "pc.race.noSelected",                        // 249
    "pc.race.no",                                // 250
    "race.pause.continueSelected",               // 251
    "race.pause.continue",                       // 252
    "pc.error.earError",                         // 253
    "pc.error.couldNotRegisterEarServiceTable",  // 254
    "race.pause.restartRaceSelected",            // 255
    "race.pause.restartRace",                    // 256
    "pc.race.musicVolume",                       // 257
    "race.pause.sfxVolume",                      // 258
    "pc.race.quitChampionshipSelected",          // 259
    "pc.race.quitChampionship",                  // 260
    "pc.race.quitRaceSelected",                  // 261
    "pc.race.quitRace",                          // 262
    "pc.race.quitGameSelected",                  // 263
    "pc.race.quitGame",                          // 264
    "race.newLapRecord",                         // 265
    "race.bestLap",                              // 266
    "pc.race.finalLap",                          // 267
    "race.lapNumber",                            // 268
    "race.place.first",                          // 269
    "race.place.second",                         // 270
    "race.place.third",                          // 271
    "race.place.fourth",                         // 272
    "race.place.fifth",                          // 273
    "race.place.sixth",                          // 274
    "race.place.place7",                         // 275
    "race.place.place8",                         // 276
    "race.place.place9",                         // 277
    "race.place.place10",                        // 278
    "race.place.place11",                        // 279
    "race.place.place12",                        // 280
    "race.place.place13",                        // 281
    "race.place.place14",                        // 282
    "race.place.place15",                        // 283
    "race.place.place16",                        // 284
    "race.place.place17",                        // 285
    "race.place.place18",                        // 286
    "race.place.place19",                        // 287
    "race.place.place20",                        // 288
    "race.newCourseRecord",                      // 289
    "race.checkPoint",                           // 290
    "race.wayBehind",                            // 291
    "race.behindSuffix",                         // 292
    "race.aheadSuffix",                          // 293
    "pc.error.bossRallyError",                   // 294
    "pc.error.unableToInitializeDirectinput",    // 295
    "pc.error.thisGameRequiresDirectx6",         // 296
    "pc.error.failedToReopenGlideWindow",        // 297
    "pc.error.couldNotConnectDirectPlayLobby",   // 298
    "pc.error.couldNotCreateDirectPlayObject",   // 299
    "pc.error.gearImageOverlap",                 // 300
    "pc.error.gearImagesTooLarge",               // 301
    "pc.error.audioCardRequired",                // 302
    "pc.menu.period",                            // 303
};

} // namespace coastline
