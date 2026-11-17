#pragma once

/**
 * @file
 * The loader of the models, textures, and vertices of the race engine, and the dents of the cars.
 */

#include <array>
#include <memory>
#include <span>
#include <vector>

#include "common/trackbackend.h"
#include "common/vehiclemodel.h"

namespace coastline {

class ModelLoader;

/** Model relocation, texture lookup, converted vertices, and dents for one race engine. */
class EngineModels {
public:
    /** Register the exclusive race model loader before loading models or track textures. */
    EngineModels();
    /** Unregister the loader and release model storage after race users have been destroyed. */
    ~EngineModels();
    /** Prevent copying registered model storage and relocation tables. */
    EngineModels(const EngineModels &) = delete;
    /**
     * Prevent assignment of registered model storage and relocation tables.
     *
     * @return This loader; the operator is deleted and never called.
     */
    EngineModels &operator=(const EngineModels &) = delete;

private:
    std::unique_ptr<ModelLoader> m_loader; // The vertex pool and relocation tables.
};

/**
 * Report the size of the texture map of the shared model loader.
 *
 * @return The entries of the texture map LoadTrackTextureMap() read last.
 */
int LoadedTextureMapEntries();

/**
 * Report the vertex count of the shared model loader.
 *
 * @return The vertices converted into the pool since ResetModelDataPool().
 */
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
