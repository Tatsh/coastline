#ifndef COASTLINE_SDL_ROMSEGMENTS_H
#define COASTLINE_SDL_ROMSEGMENTS_H

/**
 * @file
 * The offsets in the USA cartridge of the assets the link script of the Nintendo 64 release places
 * as segments. The SDL game reads the USA cartridge, where a symbol cannot have the offset as its
 * address.
 */

#define USA_GHOST_RECORDING_ROM 0xAD400u   /*!< The recording of the attract race. */
#define USA_CREDITS_RECORDING_ROM 0xAD740u /*!< The recording of the race of the credits. */
#define USA_ENDING_RECORDING_ROM 0xADB00u  /*!< The recording of the race of the ending. */
#define USA_MENU_MUSIC_ROM 0xEBC00u        /*!< The music module of the menus. */
#define USA_SCROLL_SOUND_ROM 0x1BF480u     /*!< The sound of scrolling through cars. */
#define USA_SELECT_SOUND_ROM 0x1BFEC0u     /*!< The sound of choosing a car. */
#define USA_START_LIGHTS_ROM 0x1CB410u     /*!< The model of the start lights. */

#endif /* COASTLINE_SDL_ROMSEGMENTS_H */
