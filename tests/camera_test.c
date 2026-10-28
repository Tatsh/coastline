/* Host check of the frustum and projections in src/n64/camera.c. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "camera.c"

Gfx *gDisplayListPtr;
static Gfx gfx[16];
s32 g_dwNumViewports = 1, g_dwKeepCustomClipPlanes;
Vec3f g_avFrustumPoints[FRUSTUM_POINTS], g_afFrustumCenter, g_vFrustumHalfRight, g_vFrustumHalfUp;
f32 gFrustumFar, g_fFrustumFovAngle, g_fCameraNearScale, g_fCameraFarDistance;
f32 g_ViewLookAtMtx[4][4], g_afPerspectiveMatrix[4][4], g_afScratchMatrix[4][4];
f32 g_afViewProjMatrix[4][4];
u16 g_wPerspNorm;
Mtx *g_pProjectionMtx;
static DisplayListBlock blocks[4];
static s32 blocksUsed;
DisplayListBlock *AllocDisplayListBlock(void) {
    return &blocks[blocksUsed++];
}

static s32 Near(f32 a, f32 b) {
    return fabsf(a - b) < 0.001f * (1.0f + fabsf(b));
}

static void AssertPoint(Vec3f *v, f32 x, f32 y, f32 z) {
    assert(Near(v->x, x) && Near(v->y, y) && Near(v->z, z));
}

/* Transform a point by a matrix of row vectors, as the RSP does. */
static void Transform(f32 m[4][4], f32 x, f32 y, f32 z, f32 out[4]) {
    s32 i;

    for (i = 0; i < 4; i++) {
        out[i] = x * m[0][i] + y * m[1][i] + z * m[2][i] + m[3][i];
    }
}

static void AssertMatrix(f32 a[4][4], f32 b[4][4]) {
    s32 i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            assert(Near(a[i][j], b[i][j]));
        }
    }
}

static void AssertPerspective(f32 m[4][4], f32 fovy, f32 aspect, f32 near, f32 far) {
    f32 expected[4][4];
    u16 norm;

    guPerspectiveF(expected, &norm, fovy, aspect, near, far, 1.0f);
    AssertMatrix(m, expected);
    assert(g_wPerspNorm == norm);
}

static void AssertProjectionCommands(void) {
    assert(gDisplayListPtr == &gfx[2]);
    assert(gfx[0].words.w0 == 0xBC00000E && gfx[0].words.w1 == g_wPerspNorm);
    assert(gfx[1].words.w0 == 0x01030040);
    assert(gfx[1].words.w1 == (u32)(unsigned long)g_pProjectionMtx - K0BASE);
}

static CameraFrame MakeCamera(void) {
    CameraFrame camera = {0};

    camera.forward.y = 1.0f;
    camera.side.x = 1.0f;
    camera.up.z = 1.0f;
    camera.position.x = 10.0f;
    camera.position.y = 20.0f;
    camera.position.z = 30.0f;
    return camera;
}

static void TestFrustumCorners(void) {
    CameraFrame camera = MakeCamera();

    /* sinf(pi / 6) is a half, so the far rectangle is 50 across and 37.5 up. */
    SetupFrustumCorners(&camera, 0.52359878f, 100.0f, 320.0f, 240.0f);
    AssertPoint(&g_avFrustumPoints[FRUSTUM_CAMERA], 10.0f, 20.0f, 30.0f);
    AssertPoint(&g_avFrustumPoints[FRUSTUM_FAR], 10.0f, 120.0f, 30.0f);
    AssertPoint(&g_afFrustumCenter, 10.0f, 120.0f, 30.0f);
    AssertPoint(&g_vFrustumHalfRight, 50.0f, 0.0f, 0.0f);
    AssertPoint(&g_vFrustumHalfUp, 0.0f, 0.0f, 37.5f);
    /* Each corner sits three quarters of the way from the camera. */
    AssertPoint(&g_avFrustumPoints[FRUSTUM_RIGHT_TOP], 47.5f, 95.0f, 58.125f);
    AssertPoint(&g_avFrustumPoints[FRUSTUM_LEFT_TOP], -27.5f, 95.0f, 58.125f);
    AssertPoint(&g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM], -27.5f, 95.0f, 1.875f);
    AssertPoint(&g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM], 47.5f, 95.0f, 1.875f);
    assert(gFrustumFar == 100.0f && g_fFrustumFovAngle == 0.52359878f);

    /* Two views halve the height. */
    g_dwNumViewports = 2;
    SetupFrustumCorners(&camera, 0.52359878f, 100.0f, 320.0f, 240.0f);
    AssertPoint(&g_vFrustumHalfUp, 0.0f, 0.0f, 18.75f);
    AssertPoint(&g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM], -27.5f, 95.0f, 15.9375f);
    g_dwNumViewports = 1;
}

static void TestCameraMatrix(void) {
    CameraFrame camera = MakeCamera();
    f32 expected[4][4], point[4];

    blocksUsed = 0;
    gDisplayListPtr = gfx;
    SetupCameraMatrix(&camera, 1.0f, 900.0f, 320.0f, 160.0f);
    assert(g_fCameraNearScale == 2.4f && g_fCameraFarDistance == 900.0f);
    /* The camera is at the origin of the view, looking down -z. */
    Transform(g_ViewLookAtMtx, 10.0f, 20.0f, 30.0f, point);
    assert(Near(point[0], 0.0f) && Near(point[1], 0.0f) && Near(point[2], 0.0f));
    Transform(g_ViewLookAtMtx, 10.0f, 21.0f, 30.0f, point);
    assert(Near(point[2], -1.0f));
    Transform(g_ViewLookAtMtx, 10.0f, 20.0f, 31.0f, point);
    assert(Near(point[1], 1.0f));
    /* The field of view in degrees is 4/3 of the angle, times the height over the width. */
    AssertPerspective(g_afPerspectiveMatrix, 1.3333334f * 0.5f * 57.295776f, 2.0f, 2.4f, 900.0f);
    guMtxCatF(g_ViewLookAtMtx, g_afPerspectiveMatrix, expected);
    AssertMatrix(g_afViewProjMatrix, expected);
    assert(g_pProjectionMtx == (Mtx *)&blocks[0] && blocksUsed == 1);
    assert(blocks[0].commands[0] != 0);
    /* No commands are written. */
    assert(gDisplayListPtr == gfx);
}

static void TestScreenProjections(void) {
    f32 point[4];

    blocksUsed = 0;
    gDisplayListPtr = gfx;
    SetupProjectionMatrix(320.0f, 240.0f);
    Transform(g_ViewLookAtMtx, 512.0f, 384.0f, 1000.0f, point);
    assert(Near(point[0], 0.0f) && Near(point[1], 0.0f) && Near(point[2], 0.0f));
    Transform(g_ViewLookAtMtx, 512.0f, 384.0f, 0.0f, point);
    assert(Near(point[2], -1000.0f));
    AssertPerspective(g_afScratchMatrix, 45.0f, 1.3333334f, 10.0f, 2000.0f);
    assert(g_pProjectionMtx == (Mtx *)&blocks[0]);
    AssertProjectionCommands();

    /* The menu uses its default planes unless asked once to keep the planes set before it. */
    blocksUsed = 0;
    gDisplayListPtr = gfx;
    g_dwKeepCustomClipPlanes = 1;
    g_fCameraNearScale = 5.0f;
    g_fCameraFarDistance = 50.0f;
    SetupMenuProjection(320.0f, 240.0f);
    assert(!g_dwKeepCustomClipPlanes && g_fCameraNearScale == 5.0f);
    AssertPerspective(g_afPerspectiveMatrix, 45.0f, 1.3333334f, 5.0f, 50.0f);
    Transform(g_ViewLookAtMtx, 160.0f, 120.0f, 290.0f, point);
    assert(Near(point[0], 0.0f) && Near(point[1], 0.0f) && Near(point[2], 0.0f));
    AssertProjectionCommands();

    gDisplayListPtr = gfx;
    SetupMenuProjection(320.0f, 240.0f);
    assert(g_fCameraNearScale == 1.0f && g_fCameraFarDistance == 500.0f);
    AssertPerspective(g_afPerspectiveMatrix, 45.0f, 1.3333334f, 1.0f, 500.0f);
    assert(g_pProjectionMtx == (Mtx *)&blocks[1]);
    AssertProjectionCommands();
}

int main(void) {
    TestFrustumCorners();
    TestCameraMatrix();
    TestScreenProjections();
    puts("camera ok");
    return 0;
}
