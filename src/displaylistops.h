#ifndef COASTLINE_DISPLAYLISTOPS_H
#define COASTLINE_DISPLAYLISTOPS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Patch the render modes of vehicle display lists and, on N64, track display lists.
 *
 * @ghidraAddress USA: 0x8021b97c
 * @ghidraAddress EUR: 0x8021cab8
 * @ghidraAddress JPN: 0x8021c388
 * @ghidraAddress BRD3D: 0x100344d7
 * @stub{BRD3D}
 */
void ProcessAllDisplayLists(void);

#ifdef __cplusplus
}
#endif

#endif
