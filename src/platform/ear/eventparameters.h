#ifndef COASTLINE_EAR_EVENTPARAMETERS_H
#define COASTLINE_EAR_EVENTPARAMETERS_H

#include <stdint.h>

/** The number of distance bands of an event. */
#define EAR_EVENT_DISTANCE_BAND_COUNT 6

/** A flag of EarEventParams::flags. */
enum EarEventParamsFlag {
    EAR_EVENT_DISTANCE_ATTENUATION = 0x2, /*!< The gain falls between the two distances. */
    EAR_EVENT_DIRECTIONAL = 0x10,         /*!< The direction comes from the position. */
};

/** The parameters of an event, as the game passes them to MixEvent and MoveEvent. */
typedef struct [[gnu::packed]] EarEventParams {
    uint16_t size;              /*!< The size of the parameters, 0x80. */
    uint16_t sourceType;        /*!< The type of the source in the low byte, its flags above. */
    const void *source;         /*!< The source of the samples. */
    uint32_t eventHandle;       /*!< The event. */
    uint16_t channelHandle;     /*!< The channel the event plays on. */
    uint16_t matrixHandle;      /*!< The directional matrix. */
    uint16_t minimumGain;       /*!< The gain at the maximum distance and beyond. */
    uint16_t maximumGain;       /*!< The gain at the minimum distance and nearer. */
    uint32_t maximumDistance;   /*!< The distance of the minimum gain. */
    uint32_t minimumDistance;   /*!< The distance of the maximum gain. */
    uint32_t flags;             /*!< EarEventParamsFlag values. */
    uint8_t unknown20[8];       /* +0x20 */
    uint16_t direction;         /*!< The direction of an event that is not directional. */
    uint8_t unknown2a[14];      /* +0x2a */
    uint16_t environmentHandle; /*!< The environment. */
    uint8_t unknown3a[0x26];    /* +0x3a */
    uint32_t distanceUnit;      /*!< The entry of g_adDistanceUnitToCentimeters. */
    int32_t positionX;          /*!< The position across, from the listener. */
    int32_t positionY;          /*!< The position up, from the listener. */
    int32_t positionZ;          /*!< The position ahead, from the listener. */
    /** The upper ends of the bands, in ten-thousandths of the maximum distance. */
    uint16_t distanceBands[EAR_EVENT_DISTANCE_BAND_COUNT];
    uint8_t unknown7c[4]; /* +0x7c */
} EarEventParams;

#endif /* COASTLINE_EAR_EVENTPARAMETERS_H */
