#pragma once

/**
 * @file
 * The entry points of the front end of the Nintendo 64 release in the SDL game. The library of the
 * front end renames the symbols both releases define with N64 appended.
 */

/** Record and read the data of the front end from the cartridge loaded in CartridgeData. */
void ReadN64FrontEndData(void);

/** Write some of the data of the front end to the log, to check it after ReadN64FrontEndData(). */
void LogN64FrontEndData(void);

extern "C" {

/** InitGameAndShowMenus() of the Nintendo 64 release: start the game at the splash screens. */
void InitGameAndShowMenusN64(void);
}
