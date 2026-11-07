# Changelog

## [unreleased]

### Added

- Import the Asian Nintendo 64 release as a source of USA game assets.
- Verify the Asian release's reconstructed code against its ROM during matching builds.
- Constrain paint shop lines and shapes with Shift or controller Z in SDL and patched N64 builds.
- Support the Nintendo 64 Mouse and Randnet keyboard in the patched Nintendo 64 paint shop.
- Undo and redo SDL paint shop edits with Ctrl+Z and Ctrl+Shift+Z, without a fixed history limit.
- Undo and redo patched Nintendo 64 paint shop edits with controller L/L+R or Randnet keyboard
  shortcuts, using Expansion Pak memory for deeper history when available.
- Render patched Nintendo 64 races at 640×480 when an Expansion Pak is present and
  `EXPANSION_PAK_SUPPORT` is enabled.

### Fixed

- Align the speedometer needle with its dashboard in 640×480 Nintendo 64 races.
- Read packed WAV format headers at their correct size in Windows SDL builds.
- Reject oversized paint shop bitmaps before allocating or copying pixels.
- Protect Controller Pak access from Rumble Pak updates in patched USA Nintendo 64 builds.
