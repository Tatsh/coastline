#ifndef COASTLINE_EAR_EVENTPARAMETERS_H
#define COASTLINE_EAR_EVENTPARAMETERS_H

#include <stdint.h>

/** The number of distance bands of an event. */
#define EAR_EVENT_DISTANCE_BAND_COUNT 6

/** A flag of EarEventParams::flags. */
enum EarEventParamsFlag {
    EAR_EVENT_DELAY_BY_DISTANCE = 0x1,       /*!< The sound reaches the listener at its speed. */
    EAR_EVENT_DISTANCE_ATTENUATION = 0x2,    /*!< The gain falls between the two distances. */
    EAR_EVENT_START_DELAY = 0x4,             /*!< EarEventParams::startDelay applies. */
    EAR_EVENT_LIFETIME = 0x8,                /*!< EarEventParams::lifetime applies. */
    EAR_EVENT_DIRECTIONAL = 0x10,            /*!< The direction comes from the position. */
    EAR_EVENT_LOOPING = 0x20,                /*!< The event loops. */
    EAR_EVENT_NOT_STREAMING = 0x40,          /*!< The event is read whole rather than streamed. */
    EAR_EVENT_SUPPRESS_LIFETIME = 0x80,      /*!< The lifetime does not count down. */
    EAR_EVENT_NOTIFY_STOP = 0x100,           /*!< The window is told when the event stops. */
    EAR_EVENT_DOPPLER = 0x200,               /*!< The frequency follows the speed of the source. */
    EAR_EVENT_TWIN = 0x400,                  /*!< A second event plays the same source alongside. */
    EAR_EVENT_ATTENUATION_REQUIRED = 0x800,  /*!< The event plays even beyond its distance. */
    EAR_EVENT_TRACK_ELAPSED = 0x1000,        /*!< The time the event has played is kept. */
    EAR_EVENT_DISTANCE_LIMIT = 0x2000,       /*!< EarEventParams::limitDistance applies. */
    EAR_EVENT_PRIORITY = 0x10000,            /*!< EarEventParams::priority applies. */
    EAR_EVENT_PAUSED = 0x20000,              /*!< The event starts or stays paused. */
    EAR_EVENT_MUTED = 0x40000,               /*!< The event starts or stays muted. */
    EAR_EVENT_ACTIVE_TIME = 0x80000,         /*!< EarEventParams::activeTime applies. */
    EAR_EVENT_FREQUENCY_SCALE = 0x100000,    /*!< EarEventParams::frequencyScale applies. */
    EAR_EVENT_BASE_FREQUENCY = 0x200000,     /*!< EarEventParams::baseFrequencyScale applies. */
    EAR_EVENT_NOTIFY_START = 0x400000,       /*!< The window is told when the event starts. */
    EAR_EVENT_OMNIDIRECTIONAL = 0x800000,    /*!< The event plays through matrix 1, direction 4. */
    EAR_EVENT_START_POSITION = 0x1000000,    /*!< EarEventParams::startPosition applies. */
    EAR_EVENT_PITCH_VARIATION = 0x2000000,   /*!< The base frequency varies at random. */
    EAR_EVENT_BACKEND_PARAMETER = 0x4000000, /*!< EarEventParams::backendParameter applies. */
};

/** The low word of EarEventParams::queueControl, how a new event joins its channel. */
enum EarEventQueueMode {
    EAR_EVENT_QUEUE_TAIL = 0,         /*!< The event joins after every other. */
    EAR_EVENT_QUEUE_NEXT = 1,         /*!< The event plays after the current one. */
    EAR_EVENT_QUEUE_REPLACE = 2,      /*!< The event plays next and the current one stops. */
    EAR_EVENT_QUEUE_BEFORE_EVENT = 3, /*!< The event plays before the event of the high word. */
    EAR_EVENT_QUEUE_AFTER_EVENT = 4,  /*!< The event plays after the event of the high word. */
};

/** The parameters of an event, as the game passes them to MixEvent and MoveEvent. */
typedef struct [[gnu::packed]] EarEventParams {
    uint16_t size;            /*!< The size of the parameters, 0x80. */
    uint16_t sourceType;      /*!< The type of the source in the low byte, its flags above. */
    const void *source;       /*!< The source of the samples. */
    uint32_t eventHandle;     /*!< The event. */
    uint16_t channelHandle;   /*!< The channel the event plays on. */
    uint16_t matrixHandle;    /*!< The directional matrix. */
    uint16_t minimumGain;     /*!< The gain at the maximum distance and beyond. */
    uint16_t maximumGain;     /*!< The gain at the minimum distance and nearer. */
    uint32_t maximumDistance; /*!< The distance of the minimum gain. */
    uint32_t minimumDistance; /*!< The distance of the maximum gain. */
    uint32_t flags;           /*!< EarEventParamsFlag values. */
    uint8_t unknown20[4];     /* +0x20 */
    uint32_t reservedControl; /*!< The reserved control of an event of source type 6. */
    uint16_t direction;       /*!< The direction of an event that is not directional. */
    uint16_t startDelay;      /*!< The delay before the event starts, in milliseconds. */
    int32_t lifetime;         /*!< The time the event lives after its delay, in milliseconds. */
    int32_t activeTime;       /*!< The time the event may play, in milliseconds. */
    /** The EarEventQueueMode in the low word, and the event it refers to in the high word. */
    uint32_t queueControl;
    uint16_t environmentHandle;  /*!< The environment. */
    uint8_t unknown3a[2];        /* +0x3a */
    uint16_t priority;           /*!< The priority of the event. */
    uint16_t frequencyScale;     /*!< The scale of the frequency, in ten-thousandths. */
    uint16_t baseFrequencyScale; /*!< The scale of the base frequency, in ten-thousandths. */
    uint16_t startPosition;      /*!< The position the playback starts at, in ten-thousandths. */
    uint16_t limitDistance;      /*!< The distance beyond which the event stops. */
    uint8_t unknown46[6];        /* +0x46 */
    uint16_t pitchVariation;     /*!< The range of the variation of the pitch, 0 for the default. */
    /** The parameter of the backend in ten-thousandths, 0xffff for -1. */
    uint16_t backendParameter;
    uint8_t unknown50[0x10]; /* +0x50 */
    uint32_t distanceUnit;   /*!< The entry of g_adDistanceUnitToCentimeters. */
    int32_t positionX;       /*!< The position across, from the listener. */
    int32_t positionY;       /*!< The position up, from the listener. */
    int32_t positionZ;       /*!< The position ahead, from the listener. */
    /** The upper ends of the bands, in ten-thousandths of the maximum distance. */
    uint16_t distanceBands[EAR_EVENT_DISTANCE_BAND_COUNT];
    uint8_t unknown7c[4]; /* +0x7c */
} EarEventParams;

#endif /* COASTLINE_EAR_EVENTPARAMETERS_H */
