#pragma once

#include "common/soundbuffer.h"

/**
 * Read a sample into a sound buffer node.
 *
 * @param path The path of the wave file.
 * @param node Receives the sample data and format.
 * @param sample The race sample index, used by the SDL sound selection.
 * @return 0 on success, or a nonzero error.
 */
int LoadSoundBufferSample(const char *path, SoundBufferNode *node, int sample);
