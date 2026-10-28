#pragma once

/**
 * @file
 * The layouts on the console of the types of the front end of the Nintendo 64 release that
 * Boost.PFR cannot read field by field, for the data read from the cartridge.
 */

#include <PR/gbi.h>
#include <PR/os_cont.h>

#include "carphysics.h"
#include "model.h"
#include "n64/decaleditor.h"
#include "sdl/cartridgedata.h"
#include "seasons.h"
#include "vehicle.h"

namespace coastline {

/** A model, which the menus load from the cartridge later, so their data addresses none. */
template <>
struct CartridgeOnDemand<Model> : std::false_type {};

/**
 * A command of a display list: two words, the second of which is an address for the commands that
 * call a display list or load memory. A display list it calls is defined on the host, and its
 * address becomes a Ptr32 offset as GBI_WORD1() writes one. The other memory is read from the copy
 * of the segment.
 * ponytail: textures, vertices, matrices, and lights remain big-endian bytes of the segment until
 * the renderer converts them.
 */
template <>
struct CartridgeLayout<Gfx> {
    static constexpr std::size_t size = 8;
    static constexpr std::size_t align = 8;

    static void Read(BigEndianReader &reader, Gfx &value) {
        value.words.w0 = static_cast<u32>(reader.Word(4));
        auto address = static_cast<u32>(reader.Word(4));
        value.words.w1 = address;
        switch (value.words.w0 >> 24) {
        case G_DL:
            if (address != 0 && !CartridgeData::IsDefined(address)) {
                BigEndianReader::DefineOnDemand<Gfx>(address);
            }
            value.words.w1 = EncodePtr32(CartridgeData::Relocate(address, false));
            break;
        case G_SETTIMG:
        case G_VTX:
        case G_MTX:
        case G_MOVEMEM:
            value.words.w1 = EncodePtr32(CartridgeData::Relocate(address, true));
            break;
        default:
            break;
        }
    }
};

/** The physics of a car, all 32-bit numbers. */
template <>
struct CartridgeWords<CarPhysicsSpec> : std::true_type {};

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

/** A season of the championship: its names, its races, and the cars it unlocks. */
template <>
struct CartridgeLayout<Season> {
    static constexpr std::size_t align = 4;
    static constexpr std::size_t size = detail::AlignUp(20 + 2 * SEASON_RACES, align);

    static void Read(BigEndianReader &reader, Season &value) {
        reader.Read(value.name);
        reader.Read(value.subtitle);
        reader.Read(value.raceCount);
        reader.Read(value.pointsNeeded);
        reader.Read(value.cars);
        reader.Read(value.unlocks);
        reader.Read(value.races);
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
