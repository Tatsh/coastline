#ifndef COASTLINE_RACER_SLOT_ACCESS_H
#define COASTLINE_RACER_SLOT_ACCESS_H

#include "racerslot.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define RACER_SLOT_GET(slot, field) ((slot)->Get##field())
#define RACER_SLOT_SET(slot, field, value) ((slot)->Set##field(value))
#else
#define RACER_SLOT_GET(slot, field) RACER_SLOT_GET_##field(slot)
#define RACER_SLOT_SET(slot, field, value) RACER_SLOT_SET_##field(slot, value)

#define RACER_SLOT_GET_Position(slot) ((slot)->position)
#define RACER_SLOT_GET_PreviousPosition(slot) ((slot)->previousPosition)
#define RACER_SLOT_GET_Velocity(slot) ((slot)->velocity)
#define RACER_SLOT_GET_Color(slot) ((slot)->color)
#define RACER_SLOT_GET_SpeedKph(slot) ((slot)->speedKph)
#define RACER_SLOT_SET_SpeedKph(slot, value) ((slot)->speedKph = (value))
#define RACER_SLOT_GET_Section(slot) ((slot)->section)
#define RACER_SLOT_SET_Section(slot, value) ((slot)->section = (value))
#define RACER_SLOT_GET_PointIndex(slot) ((slot)->pointIndex)
#define RACER_SLOT_SET_PointIndex(slot, value) ((slot)->pointIndex = (value))
#define RACER_SLOT_GET_LapTime(slot) ((slot)->lapTime)
#define RACER_SLOT_SET_LapTime(slot, value) ((slot)->lapTime = (value))
#define RACER_SLOT_GET_BestLapTime(slot) ((slot)->bestLapTime)
#define RACER_SLOT_SET_BestLapTime(slot, value) ((slot)->bestLapTime = (value))
#define RACER_SLOT_GET_TotalTime(slot) ((slot)->totalTime)
#define RACER_SLOT_SET_TotalTime(slot, value) ((slot)->totalTime = (value))
#define RACER_SLOT_GET_TimeLeft(slot) ((slot)->timeLeft)
#define RACER_SLOT_SET_TimeLeft(slot, value) ((slot)->timeLeft = (value))
#define RACER_SLOT_GET_Lap(slot) ((slot)->lap)
#define RACER_SLOT_SET_Lap(slot, value) ((slot)->lap = (value))
#define RACER_SLOT_GET_DistanceLaps(slot) ((slot)->distanceLaps)
#define RACER_SLOT_SET_DistanceLaps(slot, value) ((slot)->distanceLaps = (value))
#define RACER_SLOT_GET_MaxGate(slot) ((slot)->maxGate)
#define RACER_SLOT_SET_MaxGate(slot, value) ((slot)->maxGate = (value))
#define RACER_SLOT_GET_Gate(slot) ((slot)->gate)
#define RACER_SLOT_SET_Gate(slot, value) ((slot)->gate = (value))
#define RACER_SLOT_GET_Distance(slot) ((slot)->distance)
#define RACER_SLOT_SET_Distance(slot, value) ((slot)->distance = (value))
#define RACER_SLOT_GET_RacePosition(slot) ((slot)->racePosition)
#define RACER_SLOT_SET_RacePosition(slot, value) ((slot)->racePosition = (value))
#define RACER_SLOT_GET_Vehicle(slot) ((slot)->vehicle)
#define RACER_SLOT_SET_Vehicle(slot, value) ((slot)->vehicle = (value))
#define RACER_SLOT_GET_Index(slot) ((slot)->index)
#define RACER_SLOT_SET_Index(slot, value) ((slot)->index = (value))
#define RACER_SLOT_GET_Status(slot) ((slot)->status)
#define RACER_SLOT_SET_Status(slot, value) ((slot)->status = (value))
#define RACER_SLOT_GET_Straggler(slot) ((slot)->straggler)
#define RACER_SLOT_SET_Straggler(slot, value) ((slot)->straggler = (value))
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#endif /* COASTLINE_RACER_SLOT_ACCESS_H */
