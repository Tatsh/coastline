#pragma once

/**
 * @file
 * The data of the Nintendo 64 release, read from the USA cartridge at start-up for its front end.
 * The cartridge stores the data big-endian with 32-bit pointers. Each datum is read field by
 * field into its definition on the host, and each pointer is moved to the definition on the host
 * of the symbol it addresses, or into a copy of the segment for bytes such as text and pixels.
 */

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include <boost/pfr.hpp>

#include "ptr32.h"

namespace coastline {

/** The main segment of the USA cartridge, its code and data, which the console runs from RAM. */
class CartridgeData {
public:
    /**
     * Copy the main segment of the cartridge and read the symbols of the release.
     *
     * @param rom The USA cartridge image, big-endian.
     * @return Whether the image holds the main segment.
     */
    static bool Load(std::span<const std::uint8_t> rom);

    /**
     * @param address An address of the console in the main segment.
     * @return The byte of the copy of the segment at the address, or null outside it.
     */
    static const std::uint8_t *Bytes(std::uint32_t address);

    /** @return The whole cartridge image of Load(), big-endian. */
    static std::span<const std::uint8_t> Rom();

    /**
     * @param name The name of a symbol of the release.
     * @return The address of the symbol on the console, or 0 when it is not known.
     */
    static std::uint32_t AddressOf(std::string_view name);

    /**
     * Record the definition on the host of a symbol, so that pointers to it are moved there.
     *
     * @param address The address of the symbol on the console.
     * @param host The definition on the host.
     * @param consoleElementSize The bytes of an element of the symbol on the console.
     * @param hostElementSize The bytes of an element of the definition on the host.
     * @param count The elements, 1 for an object.
     */
    static void Register(std::uint32_t address,
                         void *host,
                         std::size_t consoleElementSize,
                         std::size_t hostElementSize,
                         std::size_t count);

    /**
     * @param address An address of the console stored in the data.
     * @param bytes Whether the pointer addresses bytes, which are read from the copy of the
     * segment when no definition on the host is recorded.
     * @return The address on the host, or null for 0 and for an address with no host definition.
     */
    static void *Relocate(std::uint32_t address, bool bytes);

    /**
     * @param address An address of the console.
     * @return Whether a definition on the host covers the address.
     */
    static bool IsDefined(std::uint32_t address);

    /**
     * @param address An address of the console in the main segment.
     * @return The bytes from the address to the next symbol of the release, or 0 for an address
     * that is not the start of a symbol.
     */
    static std::size_t SymbolLength(std::uint32_t address);

    /**
     * Record the routine on the host of a routine of the release, for the pointers to routines of
     * the data.
     *
     * @param name The name of the routine.
     * @param host The routine on the host.
     */
    static void RegisterRoutine(std::string_view name, void (*host)());

    /**
     * @param address The address of a routine of the console stored in the data.
     * @return The routine on the host, or null for 0 and for a routine not recorded.
     */
    static void (*RelocateRoutine(std::uint32_t address))();
};

/**
 * Whether a type is read as 32-bit words at its size on the host, for the types Boost.PFR cannot
 * read field by field, such as a structure with an anonymous union. Unions are read as words.
 */
template <typename T>
struct CartridgeWords : std::is_union<T> {};

class BigEndianReader;

/**
 * Whether the data a pointer to T addresses is defined on the host when no definition covers it.
 * A type the data only addresses with null pointers, such as a model loaded later, is not.
 */
template <typename T>
struct CartridgeOnDemand : std::true_type {};

/**
 * The layout of a type on the console, for the types Boost.PFR cannot read field by field, such as
 * a structure with an array among its fields. A specialization has the members
 * `static constexpr std::size_t size` and `align`, the bytes and the alignment on the console, and
 * `static void Read(BigEndianReader &reader, T &value)`.
 */
template <typename T>
struct CartridgeLayout;

namespace detail {

template <typename T>
concept CustomLayout = requires { CartridgeLayout<T>::size; };

template <typename T>
concept OffsetPointer = requires(T value) { value.SetRaw(0u); };

template <typename T>
concept WordType = CartridgeWords<T>::value;

// The alignment of a type on the console, where pointers have 4 bytes.
template <typename T>
constexpr std::size_t ConsoleAlignOf();

constexpr std::size_t AlignUp(std::size_t value, std::size_t align) {
    return (value + align - 1) / align * align;
}

template <typename T, std::size_t... I>
constexpr std::size_t FieldsAlignOf(std::index_sequence<I...>) {
    std::size_t align = 1;
    ((align = std::max(align, ConsoleAlignOf<boost::pfr::tuple_element_t<I, T>>())), ...);
    return align;
}

template <typename T>
constexpr std::size_t ConsoleAlignOf() {
    if constexpr (CustomLayout<T>) {
        return CartridgeLayout<T>::align;
    } else if constexpr (std::is_pointer_v<T> || std::is_enum_v<T> || OffsetPointer<T> ||
                         WordType<T>) {
        return 4;
    } else if constexpr (std::is_arithmetic_v<T>) {
        return sizeof(T);
    } else if constexpr (std::is_array_v<T>) {
        return ConsoleAlignOf<std::remove_extent_t<T>>();
    } else {
        return FieldsAlignOf<T>(std::make_index_sequence<boost::pfr::tuple_size_v<T>>());
    }
}

} // namespace detail

/** @return The bytes of a value of T on the console, with 32-bit pointers. */
template <typename T>
constexpr std::size_t ConsoleSizeOf();

namespace detail {

template <typename T, std::size_t... I>
constexpr std::size_t FieldsSizeOf(std::index_sequence<I...>) {
    std::size_t size = 0;
    ((size = AlignUp(size, ConsoleAlignOf<boost::pfr::tuple_element_t<I, T>>()) +
             ConsoleSizeOf<boost::pfr::tuple_element_t<I, T>>()),
     ...);
    return AlignUp(size, ConsoleAlignOf<T>());
}

} // namespace detail

template <typename T>
constexpr std::size_t ConsoleSizeOf() {
    if constexpr (detail::CustomLayout<T>) {
        return CartridgeLayout<T>::size;
    } else if constexpr (std::is_pointer_v<T> || std::is_enum_v<T> || detail::OffsetPointer<T>) {
        return 4;
    } else if constexpr (std::is_arithmetic_v<T> || detail::WordType<T>) {
        return sizeof(T);
    } else if constexpr (std::is_array_v<T>) {
        return std::extent_v<T> * ConsoleSizeOf<std::remove_extent_t<T>>();
    } else {
        return detail::FieldsSizeOf<T>(std::make_index_sequence<boost::pfr::tuple_size_v<T>>());
    }
}

/** Reads big-endian data of the console in the layout of the console. */
class BigEndianReader {
public:
    /** @param address The address of the console of the first byte. */
    explicit BigEndianReader(std::uint32_t address) : m_address(address) {
    }

    /**
     * Read a value of the console into a value of the host, its fields in order with the padding
     * of the console.
     *
     * @param value The value.
     */
    template <typename T>
    void Read(T &value) {
        if constexpr (detail::CustomLayout<T>) {
            CartridgeLayout<T>::Read(*this, value);
        } else if constexpr (std::is_enum_v<T>) {
            value = static_cast<T>(Word(4));
        } else if constexpr (std::is_floating_point_v<T>) {
            static_assert(sizeof(T) == 4, "The console stores floats in 32 bits.");
            value = std::bit_cast<T>(static_cast<std::uint32_t>(Word(4)));
        } else if constexpr (std::is_integral_v<T>) {
            value = static_cast<T>(Word(sizeof(T)));
        } else if constexpr (std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>) {
            value = reinterpret_cast<T>(
                CartridgeData::RelocateRoutine(static_cast<std::uint32_t>(Word(4))));
        } else if constexpr (std::is_pointer_v<T>) {
            using Pointee = std::remove_cv_t<std::remove_pointer_t<T>>;
            constexpr bool bytes = std::is_void_v<Pointee> || std::is_same_v<Pointee, char> ||
                                   std::is_same_v<Pointee, signed char> ||
                                   std::is_same_v<Pointee, unsigned char>;
            auto address = static_cast<std::uint32_t>(Word(4));
            if constexpr (!bytes && requires {
                              sizeof(Pointee);
                              Pointee{};
                          } && CartridgeOnDemand<Pointee>::value) {
                if (address != 0 && !CartridgeData::IsDefined(address)) {
                    DefineOnDemand<Pointee>(address);
                }
            }
            value = reinterpret_cast<T>(CartridgeData::Relocate(address, bytes));
        } else if constexpr (std::is_array_v<T>) {
            for (auto &element : value) {
                Read(element);
            }
        } else if constexpr (detail::OffsetPointer<T>) {
            using Pointee = std::remove_cv_t<std::remove_pointer_t<decltype(value.operator->())>>;
            constexpr bool bytes = sizeof(Pointee) == 1;
            auto address = static_cast<std::uint32_t>(Word(4));
            if constexpr (!bytes && requires { Pointee{}; } && CartridgeOnDemand<Pointee>::value) {
                if (address != 0 && !CartridgeData::IsDefined(address)) {
                    DefineOnDemand<Pointee>(address);
                }
            }
            value.SetRaw(coastline::EncodePtr32(CartridgeData::Relocate(address, bytes)));
        } else if constexpr (detail::WordType<T>) {
            static_assert(sizeof(T) % 4 == 0, "A type read as words has whole words.");
            std::uint32_t words[sizeof(T) / 4];
            for (auto &word : words) {
                word = static_cast<std::uint32_t>(Word(4));
            }
            std::memcpy(&value, words, sizeof(T));
        } else {
            static_assert(std::is_aggregate_v<T>, "Only aggregates are read field by field.");
            boost::pfr::for_each_field(value, [this](auto &field) {
                Align(detail::ConsoleAlignOf<std::remove_cvref_t<decltype(field)>>());
                Read(field);
            });
            Align(detail::ConsoleAlignOf<T>());
        }
    }

    /**
     * Move to the next multiple of an alignment, as the console pads a field.
     *
     * @param align The alignment in bytes.
     */
    void Align(std::size_t align) {
        m_address = static_cast<std::uint32_t>(detail::AlignUp(m_address, align));
    }

    /**
     * Skip bytes the host does not read.
     *
     * @param size The bytes.
     */
    void Skip(std::size_t size) {
        m_address += static_cast<std::uint32_t>(size);
    }

    /**
     * Define on the host the data at an address no definition covers, which only other data
     * addresses, as elements of T up to the next symbol of the release. The definition remains
     * for the rest of the program.
     *
     * @param address The address of the console of the data.
     */
    template <typename T>
    static void DefineOnDemand(std::uint32_t address) {
        std::size_t consoleSize = ConsoleSizeOf<T>();
        std::size_t count =
            std::max<std::size_t>(1, CartridgeData::SymbolLength(address) / consoleSize);
        auto *objects = new T[count]{};
        CartridgeData::Register(address, objects, consoleSize, sizeof(T), count);
        BigEndianReader reader(address);
        for (std::size_t i = 0; i < count; ++i) {
            reader.Read(objects[i]);
        }
    }

    /**
     * @param size The bytes of the word, from 1 to 8.
     * @return The next big-endian word.
     */
    std::uint64_t Word(std::size_t size);

private:
    std::uint32_t m_address;
};

/**
 * Record the definition on the host of a symbol of the release for the pointers of other data,
 * without reading it.
 *
 * @param name The name of the symbol.
 * @param value The definition on the host, an object or an array.
 * @return Whether the symbol is known.
 */
template <typename T>
bool RegisterCartridgeSymbol(std::string_view name, T &value) {
    std::uint32_t address = CartridgeData::AddressOf(name);
    if (address == 0) {
        return false;
    }
    using Element = std::remove_all_extents_t<T>;
    CartridgeData::Register(
        address, &value, ConsoleSizeOf<Element>(), sizeof(Element), sizeof(T) / sizeof(Element));
    return true;
}

/**
 * Read a symbol of the release into its definition on the host, and record the definition for the
 * pointers of other data. Record every symbol a datum addresses before the datum is read.
 *
 * @param name The name of the symbol.
 * @param value The definition on the host, an object or an array.
 * @return Whether the symbol is known.
 */
template <typename T>
bool ReadCartridgeSymbol(std::string_view name, T &value) {
    std::uint32_t address = CartridgeData::AddressOf(name);
    if (address == 0) {
        return false;
    }
    using Element = std::remove_all_extents_t<T>;
    CartridgeData::Register(
        address, &value, ConsoleSizeOf<Element>(), sizeof(Element), sizeof(T) / sizeof(Element));
    BigEndianReader reader(address);
    reader.Read(value);
    return true;
}

} // namespace coastline
