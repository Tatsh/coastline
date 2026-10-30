#ifndef COASTLINE_COMBINE_H
#define COASTLINE_COMBINE_H

#include "common/gbicommand.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Encode an input of the colour combiner for a field of 5 bits: 0 for zero becomes selector 31, 1
 * for one becomes selector 6, and 1000 more than a selector becomes the selector.
 *
 * @param nInput The input.
 * @return The selector.
 * @ghidraAddress BRD3D: 0x1002faf0
 * @ghidraAddress BRGlide: 0x1001d180
 */
int EncodeCombinerSelectorWide(int nInput);

/**
 * Encode an input of the colour combiner for a field of 3 bits: 0 for zero becomes selector 7, 1
 * for one becomes selector 6, 1013 becomes selector 0, and 1000 more than a selector becomes the
 * selector.
 *
 * @param nInput The input.
 * @return The selector.
 * @ghidraAddress BRD3D: 0x1002fac0
 * @ghidraAddress BRGlide: 0x1001d150
 */
int EncodeCombinerSelectorNarrow(int nInput);

/**
 * Build the command that sets the combine mode of two cycles from the inputs of the colour
 * combiner, as EncodeCombinerSelectorWide() and EncodeCombinerSelectorNarrow() take them, packed as
 * the console packs them.
 *
 * @param pCommand Receives the command.
 * @param nColorA0 The input a of the colour of the first cycle.
 * @param nColorB0 The input b of the colour of the first cycle.
 * @param nColorC0 The input c of the colour of the first cycle.
 * @param nColorD0 The input d of the colour of the first cycle.
 * @param nAlphaA0 The input a of the alpha of the first cycle.
 * @param nAlphaB0 The input b of the alpha of the first cycle.
 * @param nAlphaC0 The input c of the alpha of the first cycle.
 * @param nAlphaD0 The input d of the alpha of the first cycle.
 * @param nColorA1 The input a of the colour of the second cycle.
 * @param nColorB1 The input b of the colour of the second cycle.
 * @param nColorC1 The input c of the colour of the second cycle.
 * @param nColorD1 The input d of the colour of the second cycle.
 * @param nAlphaA1 The input a of the alpha of the second cycle.
 * @param nAlphaB1 The input b of the alpha of the second cycle.
 * @param nAlphaC1 The input c of the alpha of the second cycle.
 * @param nAlphaD1 The input d of the alpha of the second cycle.
 * @ghidraAddress BRD3D: 0x1002f900
 * @ghidraAddress BRGlide: 0x1001cf90
 */
void PackCombineMode(GbiCommand *pCommand,
                     int nColorA0,
                     int nColorB0,
                     int nColorC0,
                     int nColorD0,
                     int nAlphaA0,
                     int nAlphaB0,
                     int nAlphaC0,
                     int nAlphaD0,
                     int nColorA1,
                     int nColorB1,
                     int nColorC1,
                     int nColorD1,
                     int nAlphaA1,
                     int nAlphaB1,
                     int nAlphaC1,
                     int nAlphaD1);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_COMBINE_H */
