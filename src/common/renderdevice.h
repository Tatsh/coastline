#ifndef COASTLINE_RENDERDEVICE_H
#define COASTLINE_RENDERDEVICE_H

/**
 * @file
 * The device the renderer of the PC release draws with: render states by the numbers of
 * D3DRENDERSTATETYPE, and lists of transformed and lit triangles. The Windows libraries draw with
 * Direct3D, and the SDL game with its own renderer.
 */

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A vertex the game transformed and lit, laid out as D3DTLVERTEX. */
typedef struct DeviceVertex {
    float sx;     /*!< The screen x in pixels. */
    float sy;     /*!< The screen y in pixels. */
    float sz;     /*!< The depth, from 0 to 1. */
    float rhw;    /*!< The reciprocal of the homogeneous w. */
    u32 color;    /*!< The diffuse colour, 0xAARRGGBB. */
    u32 specular; /*!< The specular colour, with the fog share in its alpha. */
    float tu;     /*!< The texture u. */
    float tv;     /*!< The texture v. */
} DeviceVertex;

/**
 * Set a render state of the device.
 *
 * @param state The state, a value of D3DRENDERSTATETYPE.
 * @param value The value, in the units of Direct3D.
 */
void SetDeviceRenderState(u32 state, u32 value);

/**
 * Draw a list of triangles.
 *
 * @param vertices The vertices, three for each triangle.
 * @param count The number of vertices.
 */
void DrawDeviceTriangles(DeviceVertex *vertices, u32 count);

/**
 * Draw a list of indexed triangles.
 *
 * @param vertices The vertices.
 * @param vertexCount The number of vertices.
 * @param indices The indices, three for each triangle.
 * @param indexCount The number of indices.
 */
void DrawDeviceIndexedTriangles(DeviceVertex *vertices,
                                u32 vertexCount,
                                u16 *indices,
                                u32 indexCount);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RENDERDEVICE_H */
