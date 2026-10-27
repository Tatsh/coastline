#ifndef COASTLINE_VOLUME_CONTROLS_H
#define COASTLINE_VOLUME_CONTROLS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Raise the music a level, and apply it.
 *
 * @ghidraAddress USA: 0x80211840
 * @ghidraAddress EUR: 0x80211730
 * @ghidraAddress JPN: 0x80211620
 * @ghidraAddress BRD3D: 0x10060d50
 */
void IncreaseBgmVolume(void);

/**
 * Lower the music a level, and apply it.
 *
 * @ghidraAddress USA: 0x8021187c
 * @ghidraAddress EUR: 0x8021176c
 * @ghidraAddress JPN: 0x8021165c
 * @ghidraAddress BRD3D: 0x10060d70
 */
void DecreaseBgmVolume(void);

/**
 * Raise the effects a level, and apply it.
 *
 * @ghidraAddress USA: 0x802118f8
 * @ghidraAddress EUR: 0x802117e8
 * @ghidraAddress JPN: 0x802116d8
 * @ghidraAddress BRD3D: 0x10060dc0
 */
void IncreaseSfxVolume(void);

/**
 * Lower the effects a level, and apply it.
 *
 * @ghidraAddress USA: 0x80211934
 * @ghidraAddress EUR: 0x80211824
 * @ghidraAddress JPN: 0x80211714
 * @ghidraAddress BRD3D: 0x10060de0
 */
void DecreaseSfxVolume(void);

#ifdef __cplusplus
}
#endif

#endif
