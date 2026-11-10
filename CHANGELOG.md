# Changelog

## [unreleased]

### Added

- Import the Asian Nintendo 64 release as a source of USA game assets.
- Verify the Asian release's reconstructed code against its ROM during matching builds.
- Constrain paint shop lines and shapes with Shift or controller Z in SDL and patched N64 builds.
- Support the Nintendo 64 Mouse and Randnet keyboard in the patched Nintendo 64 paint shop.
- Undo and redo SDL paint shop edits with Ctrl+Z and Ctrl+Shift+Z, without a fixed history limit.
- Undo and redo patched Nintendo 64 paint shop edits with controller L/L+R or Randnet keyboard
  shortcuts.
- Build a Boss Rally installer with both Windows renderer libraries that copies data from the
  original disc during installation.
- Include version and attribution metadata in Windows installers, uninstallers, and Boss Rally
  binaries.
- Include a numeric build version, tag or revision, build date, and attribution in the macOS app
  bundle metadata.
- Use HDR display headroom for CRT highlights when the window supports HDR output.

### Changed

- Use `boss-rally`, `coastline`, and `tgr-<region>` as the public CMake build presets.
- Convert the Boss Rally intro to MP4 during import, using libav on Linux and temporary FFmpeg on
  macOS and Windows. Play it with native media APIs on macOS and Windows.

### Fixed

- Apply CRT scanlines and masks in linear light and compensate for their average brightness loss.
- Bound Boss Rally file-error messages, release EAR allocations consistently, and validate CD track
  counts on current Windows compilers.
- Treat localised Boss Rally HUD and race messages as text on current Windows compilers.
- Avoid closing unopened WAV files when a header is invalid on current Windows compilers.
- Align the speedometer needle with its dashboard in 640×480 Nintendo 64 races.
- Read packed WAV format headers at their correct size in Windows SDL builds.
- Reject oversized paint shop bitmaps before allocating or copying pixels.
- Protect Controller Pak access from Rumble Pak updates in patched USA Nintendo 64 builds.
