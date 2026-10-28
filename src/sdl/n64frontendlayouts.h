#pragma once

/**
 * @file
 * The layouts on the console of the types of the front end of the Nintendo 64 release that
 * Boost.PFR cannot read field by field, for the data read from the cartridge.
 */

#include <PR/os_cont.h>

#include "n64/decaleditor.h"
#include "sdl/cartridgedata.h"
#include "vehicle.h"

namespace coastline {

/** A swatch of the decal editor: a rectangle of four words and three bytes of color. */
template <>
struct CartridgeLayout<PaletteSwatch> {
    static constexpr std::size_t size = 20;
    static constexpr std::size_t align = 4;

    static void Read(BigEndianReader &reader, PaletteSwatch &value) {
        reader.Read(value.rect);
        reader.Read(value.color);
        reader.Align(align);
    }
};

/** A car: its name, the offsets of its model and dashboard, and the values of its menus. */
template <>
struct CartridgeLayout<CarModel> {
    static constexpr std::size_t size = 0x60;
    static constexpr std::size_t align = 4;

    static void Read(BigEndianReader &reader, CarModel &value) {
        reader.Read(value.name);
        reader.Read(value.unreconstructed4);
        reader.Read(value.modelRom);
        reader.Read(value.modelRomEnd);
        reader.Read(value.modelSize);
        reader.Read(value.dashboardRom);
        reader.Read(value.unreconstructed24);
        reader.Read(value.dashboardWidth);
        reader.Read(value.dashboardHeight);
        reader.Read(value.gearX);
        reader.Read(value.gearY);
        reader.Read(value.gearWidth);
        reader.Read(value.gearHeight);
        reader.Read(value.tachometer);
        reader.Read(value.needleX);
        reader.Read(value.needleY);
        reader.Read(value.unreconstructed31);
        reader.Read(value.needleFullAngle);
        reader.Read(value.needleRestAngle);
        reader.Read(value.driveType);
        reader.Read(value.engineLayout);
        reader.Read(value.acceleration);
        reader.Read(value.maxSpeed);
        reader.Read(value.unreconstructed4C);
        reader.Read(value.gearCount);
        reader.Read(value.shadowLength);
        reader.Read(value.shadowHalfWidth);
    }
};

/** The state of a Controller Pak, which is only in the BSS and is not read from the cartridge. */
template <>
struct CartridgeLayout<OSPfs> {
    static constexpr std::size_t size = sizeof(OSPfs);
    static constexpr std::size_t align = 4;

    static void Read(BigEndianReader &reader, OSPfs &value) {
        reader.Skip(size);
    }
};

/** The state of a file of a Controller Pak, which is only in the BSS. */
template <>
struct CartridgeLayout<OSPfsState> {
    static constexpr std::size_t size = sizeof(OSPfsState);
    static constexpr std::size_t align = 4;

    static void Read(BigEndianReader &reader, OSPfsState &value) {
        reader.Skip(size);
    }
};

} // namespace coastline
