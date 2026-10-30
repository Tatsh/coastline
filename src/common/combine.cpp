#include "common/combine.h"

// The inputs of the colour combiner that are not selectors, and the base of those that are.
#define COMBINER_INPUT_ZERO 0
#define COMBINER_INPUT_ONE 1
#define COMBINER_INPUT_SELECTOR_BASE 1000
// The input the narrow fields take as selector 0.
#define COMBINER_INPUT_NARROW_SELECTOR_0 1013

int EncodeCombinerSelectorNarrow(int nInput) {
    switch (nInput) {
    case COMBINER_INPUT_ZERO:
        return 7;
    case COMBINER_INPUT_ONE:
        return 6;
    case COMBINER_INPUT_NARROW_SELECTOR_0:
        return 0;
    default:
        return nInput - COMBINER_INPUT_SELECTOR_BASE;
    }
}

int EncodeCombinerSelectorWide(int nInput) {
    switch (nInput) {
    case COMBINER_INPUT_ZERO:
        return 31;
    case COMBINER_INPUT_ONE:
        return 6;
    default:
        return nInput - COMBINER_INPUT_SELECTOR_BASE;
    }
}

// The opcode of the command that sets the combine mode.
#define GBI_SET_COMBINE_OPCODE 0xfcu

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
                     int nAlphaD1) {
    unsigned dwColorA0 = EncodeCombinerSelectorWide(nColorA0);
    unsigned dwColorB0 = EncodeCombinerSelectorWide(nColorB0);
    unsigned dwColorC0 = EncodeCombinerSelectorWide(nColorC0);
    unsigned dwColorD0 = EncodeCombinerSelectorWide(nColorD0);
    unsigned dwAlphaA0 = EncodeCombinerSelectorNarrow(nAlphaA0);
    unsigned dwAlphaB0 = EncodeCombinerSelectorNarrow(nAlphaB0);
    unsigned dwAlphaC0 = EncodeCombinerSelectorNarrow(nAlphaC0);
    unsigned dwAlphaD0 = EncodeCombinerSelectorNarrow(nAlphaD0);
    unsigned dwColorA1 = EncodeCombinerSelectorWide(nColorA1);
    unsigned dwColorB1 = EncodeCombinerSelectorWide(nColorB1);
    unsigned dwColorC1 = EncodeCombinerSelectorWide(nColorC1);
    unsigned dwColorD1 = EncodeCombinerSelectorWide(nColorD1);
    unsigned dwAlphaA1 = EncodeCombinerSelectorNarrow(nAlphaA1);
    unsigned dwAlphaB1 = EncodeCombinerSelectorNarrow(nAlphaB1);
    unsigned dwAlphaC1 = EncodeCombinerSelectorNarrow(nAlphaC1);
    unsigned dwAlphaD1 = EncodeCombinerSelectorNarrow(nAlphaD1);

    pCommand->w0 = GBI_SET_COMBINE_OPCODE << 24 | (dwColorA0 & 0xf) << 20 |
                   (dwColorC0 & 0x1f) << 15 | (dwAlphaA0 & 7) << 12 | (dwAlphaC0 & 7) << 9 |
                   (dwColorA1 & 0xf) << 5 | (dwColorC1 & 0x1f);
    pCommand->w1 = (dwColorB0 & 0xf) << 28 | (dwColorB1 & 0xf) << 24 | (dwAlphaA1 & 7) << 21 |
                   (dwAlphaC1 & 7) << 18 | (dwColorD0 & 7) << 15 | (dwAlphaB0 & 7) << 12 |
                   (dwAlphaD0 & 7) << 9 | (dwColorD1 & 7) << 6 | (dwAlphaB1 & 7) << 3 |
                   (dwAlphaD1 & 7);
}
