#pragma once

/**
 * @file
 * The entry points of the front end of the Nintendo 64 release in the SDL game. The library of the
 * front end renames the symbols both releases define with N64 appended.
 */

#include <filesystem>

/** Record and read the data of the front end from the cartridge loaded in CartridgeData. */
void ReadN64FrontEndData(void);

/** Write some of the data of the front end to the log, to check it after ReadN64FrontEndData(). */
void LogN64FrontEndData(void);

namespace coastline {
class InputBindings;
class N64Audio;
} // namespace coastline

/**
 * Write the pictures of the front end that are not in a directory yet as PNG files there, named
 * after their symbols, which every release shares. The data of the front end must be read first.
 *
 * @param directory The directory of the texture pack.
 * @return The pictures written.
 */
int WriteN64TexturePack(const std::filesystem::path &directory);

/**
 * @param descriptor A Sprite or a Texture of the front end.
 * @return The name of its picture in a texture pack, or null for a descriptor of no picture.
 */
const char *N64PictureName(const void *descriptor);

/**
 * Set the bindings the front end reads its controllers from.
 *
 * @param input The bindings, or `nullptr` for no controllers.
 */
void SetN64FrontEndInput(const coastline::InputBindings *input);

/**
 * Set the player the music the front end starts plays on.
 *
 * @param audio The player, or `nullptr` for no music.
 */
void SetN64FrontEndAudio(coastline::N64Audio *audio);

extern "C" {

/** InitGameAndShowMenus() of the Nintendo 64 release: start the game at the splash screens. */
void InitGameAndShowMenusN64(void);
}
