# Changelog

## [unreleased]

### Added

- Import the Asian Nintendo 64 release as a source of USA game assets.
- Verify the Asian release's reconstructed code against its ROM during matching builds.
- Constrain paint shop lines and shapes with Shift or controller Z in SDL and patched N64 builds.
- Support the Nintendo 64 Mouse and Randnet keyboard in the patched Nintendo 64 paint shop.

### Fixed

- Reject oversized paint shop bitmaps before allocating or copying pixels.
- Protect Controller Pak access from Rumble Pak updates in patched USA Nintendo 64 builds.
