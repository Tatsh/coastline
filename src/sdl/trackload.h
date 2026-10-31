#pragma once

#include <array>
#include <memory>
#include <span>
#include <vector>

#include "common/vehiclemodel.h"

namespace coastline {

class ModelLoader;

/** Model relocation, texture lookup, converted vertices, and dents for one race engine. */
class EngineModels {
public:
    EngineModels();
    ~EngineModels();
    EngineModels(const EngineModels &) = delete;
    EngineModels &operator=(const EngineModels &) = delete;

private:
    std::unique_ptr<ModelLoader> m_loader;
};

/** Set the texture segment of the loaded track before relocating its textures. */
void SetTrackTextureSource(unsigned char *textures);

/** @return The entries of the texture map LoadTrackTextureMap() read last. */
int LoadedTextureMapEntries();

/** @return The vertices converted into the pool since ResetModelDataPool(). */
unsigned ConvertedVertexCount();

/**
 * How far DeformVehicleMeshByImpact() has moved each vertex of a car from where it rested when the
 * car loaded, by the position it rested at.
 *
 * @param model The model of the car.
 * @param rest The position each vertex rests at, in the units of the model.
 * @return The displacement of each vertex of @p rest, or nothing when the car has no dent.
 */
std::vector<std::array<float, 3>> CarDentOffsets(const VehicleModel *model,
                                                 std::span<const std::array<float, 3>> rest);

} // namespace coastline
