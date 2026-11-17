# Game options

The Options menu writes the settings below to the `[Game]` section of `BossRally.ini`.
[Display](display.md) covers the graphics settings and presets.

## Auto-save

`Auto-save race progress` in Options saves championship progress, records, and improved Time Attack
ghosts after each completed race. Autosave is on by default (`AutoSave=1` under `[Game]` in
`BossRally.ini`). Championship saves resume at the next round. Automatic saves replace the primary
season and ghost saves. The option takes effect immediately; manual saving remains available with
autosave off. Preferences save independently of race progress. Failed automatic saves display a
warning on the results screen.

## Units

Until Units is changed, `--region jpn` and `--region eur` select KPH, and `--region usa` selects
MPH. Without an explicit region, system locales for MPH jurisdictions use MPH and other country
locales use KPH. MPH country codes are AG, AI, AS, BS, BZ, DM, FK, FM, GB, GD, GG, GU, IM, IO, JE,
KN, KY, LC, MH, MP, MS, PR, PW, SH, TC, US, VC, VG, and VI. Liberia defaults to KPH.
If the locale has no country, MPH is the fallback. Changing Units records `Units=mph`
or `Units=kph` under `[Game]` and overrides automatic selection on subsequent launches. Existing
settings without a valid Units preference use automatic selection. Explicitly loading an options
note saves its units as a manual choice. Set `Units=auto` to restore automatic selection.

## Paint shop

Paint Shop supports Milk Truck, Cupra, Helmet, and Beach Ball. Cupra, Helmet, and Beach Ball use
numbered texture buttons in place of body-panel buttons. Tall textures shrink to fit the canvas.
Painting preserves the imported image beneath the decal; clearing the decal restores the image
with the current body tint. Bonus-car decals save beneath `controller-pak/custom-decals` in the
save directory, independently of the console Controller Pak capacity. Console decal notes retain
their native format when imported from or exported to a Controller Pak image.

## Title screen

The `Title screen` option selects the Japanese or international opening independently of language
and regional models and textures. `[Game] TitleScreen` accepts `regional` (default), `japanese`,
or `international`. Changing the title reinitialises the game after Options closes and returns
to the logos. Alternate titles require corresponding imported ROM assets. The menu offers available
titles and marks a saved title as missing if its assets are
absent. If the requested title cannot load, the game logs a warning and uses the selected
model region's opening. Asset-pack overrides also apply to the chosen intro.
An explicit `--region` overrides the saved title choice for the run and locks Title screen in
Options to the active region's opening after fallback.

## Music

The `Music` option selects native N64 XM modules or PC CD audio (`Music=XM` or `Music=CD` under
`[Game]`). The selection applies as Options closes. CD selection also plays the PC menu track
in the front end. The BGM volume controls both sources. Missing CD audio falls back to XM with a
warning. F12 advances the selected source's
tracks; restarting a race retains a manually selected XM module.
`PlayMusic=0` silences music in menus and races while sound effects remain independent.

The music of the cartridge's XM modules is scaled against the sound effects by the gain of the
asset pack, 1.5 in the default pack and 1 for the level of the console. `XmMusicGain` in the
`[Game]` section replaces the gain of the pack when the user adds it. The music clips rather than
wrapping when the gain raises it past the range of a sample.

## Sound effects

`Sound effects` selects PC or N64 samples (`SoundEffects=PC` or `SoundEffects=N64` under
`[Game]`, with PC as the default). New installs use volume 4/10 for PC or 10/10 for N64 until the
SFX volume is adjusted. A manual volume persists across sample changes and restarts as `SfxVolume=0`
through `SfxVolume=10` under `[Game]`; `SfxVolume=auto` restores the source-dependent default.
Changing samples reinitialises the game after Options closes and returns to the logos. Race sounds
use the selected pack's converted WAV files. N64 samples
retain their duration and match the RMS level
of their PC counterparts, limited to 98% peak amplitude. Missing N64 samples fall back to PC
with a warning; PC taunts are silent in N64 mode. Menu navigation and adjustments use their
distinct native sounds with either race sound source. Intro sounds remain specific to the selected
title.
`PlaySFX=0` silences menu feedback, intro sounds, and race sound effects independently of music.

## Volume

Setting BGM or SFX volume to zero suspends processing for the corresponding category. BGM stops XM
sequencing and mixing or pauses CD playback and FLAC decoding. SFX pauses native mixing and EAR
playback updates. Raising a volume resumes its category independently; setting both to zero also
pauses the native audio device.

## Race cameras

Each player's selected race camera persists across races and application restarts. `[Player1]`
and `[Player2]` store `Camera` as `0` (cockpit), `1` (chase), `2` (bumper), or `3` (trackside).
Replays and automatic finish cameras preserve the player's selection.

## Visual effects

`Visual effects` selects PC or N64 rendering (`Effects=PC` or `Effects=N64` under `[Game]`,
with PC as the default). The selection applies as Options closes. N64 effects use the imported
smoke and wheel-spray PNGs, with ordered opacity and depth writes for particles. Persistent
skid marks appear only in PC mode, as in the original releases. Particle motion follows the
race simulation in both modes. Import ROM images again to add the effect PNGs to older asset
directories; missing images produce a warning and use PC effects.

## Window reflections

`Window reflections` selects the original PC projection or N64 angular reflection mapping.
`WindowReflections=auto` under `[Game]` follows Visual effects by default; `PC` and `N64` override
the mapping independently. Changes apply as Options closes. Both versions use the same reflection
images and switch to the night image inside tunnels; their generated texture coordinates differ.
