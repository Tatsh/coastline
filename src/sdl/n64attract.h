#pragma once

/**
 * @file
 * The attract demo of a Nintendo 64 release, recorded at import by running the code of its
 * cartridge in the interpreter of mipscpu.h from power on, with no controller input, until the demo
 * after the splash screens ends.
 */

#include <array>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace coastline {

/** A release of the cartridge whose demo is recorded. */
enum class N64DemoRelease {
    Usa, /*!< North America. */
    Jpn, /*!< Japan. */
    Eur, /*!< Europe, whose race steps at 25 Hz. */
};

/** The first vehicle of the demo after a step of its race. */
struct N64DemoFrame {
    float matrix[4][4];           /*!< The orientation and the position of the body, row by row. */
    float bodyRoll;               /*!< The roll angle of the body. */
    float steerAngle;             /*!< The steering angle of the front wheels, in radians. */
    float wheelRoll[4];           /*!< How far each wheel has turned, in degrees. */
    std::int32_t wheelContact[4]; /*!< The frames each wheel has touched the ground. */
    std::uint8_t wheelSurface[4]; /*!< The surface under each wheel. */
    std::uint8_t skidding;        /*!< Set while the tyres skid. */
    std::uint8_t landed;          /*!< Set when a wheel lands. */
    std::uint8_t impactVolume;    /*!< The loudest hit of the track since the sounds played. */
    std::uint8_t vehicleImpactVolume; /*!< The loudest hit of another vehicle. */
    std::uint8_t effect;              /*!< The effect byte the vehicle sounds clear. */
    std::uint32_t buttons;            /*!< The buttons and the race actions of the input. */
    float throttle;                   /*!< The throttle, below 0 in reverse. */
    float engineRpm;                  /*!< The revolutions of the engine a minute. */
    float distance;                   /*!< The distance travelled along the route. */
    float totalTime;                  /*!< The time of the race so far, in seconds. */
    std::int32_t lap;                 /*!< The laps finished. */
};

/** The recorded demo. */
struct N64Demo {
    std::int32_t track;                  /*!< The track, as the race numbers it. */
    std::int32_t car;                    /*!< The car. */
    std::array<std::uint8_t, 5> options; /*!< The handling, transmission, tyres, suspension, and
                                              controller layout of the recording. */
    std::array<std::uint8_t, 3> color;   /*!< The colour of the car. */
    float stepSeconds;                   /*!< The time of a step of the race. */
    std::vector<N64DemoFrame> frames;    /*!< The vehicle after each step. */
    std::uint64_t instructions;          /*!< The instructions run, for the log. */
};

/**
 * Record the attract demo of a cartridge.
 *
 * @param rom The image, in the byte order of the console.
 * @param release The release of the image.
 * @return The demo, or the reason it could not be recorded.
 */
std::expected<N64Demo, std::string> RecordN64AttractDemo(std::span<const std::uint8_t> rom,
                                                         N64DemoRelease release);

} // namespace coastline
