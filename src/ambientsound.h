#ifndef COASTLINE_AMBIENT_SOUND_H
#define COASTLINE_AMBIENT_SOUND_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Play the nearest sound of the track on its channel, panned and pitched for the camera.
 *
 * @ghidraAddress USA: 0x8022b534
 * @ghidraAddress EUR: 0x8022ca44
 * @ghidraAddress JPN: 0x8022c1fc
 * @ghidraAddress BRD3D: 0x10067ed0
 */
void UpdateAmbientSoundChannel(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_AMBIENT_SOUND_H */
