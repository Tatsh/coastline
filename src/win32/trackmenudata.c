#include <stddef.h>

#include "trackmenuitem.h"

/* These native segment markers have only descriptor references and are never read. */
/* @ghidraAddress BRD3D: 0x117824f0 */
static u32 g_auUnusedTrackSegmentMarkers[42];

/* @ghidraAddress BRD3D: 0x100b5b28 */
static const SkyTextureFiles g_DesertDaySkyFiles = {"cargfx/skytexdesert.lut4",
                                                    "cargfx/skytexdesert.ci4"};
/* @ghidraAddress BRD3D: 0x100b5b60 */
static const SkyTextureFiles g_MountainDaySkyFiles = {"cargfx/skytexmountain.lut4",
                                                      "cargfx/skytexmountain.ci4"};
/* @ghidraAddress BRD3D: 0x100b5ba0 */
static const SkyTextureFiles g_CoastDaySkyFiles = {"cargfx/skytexcoast.lut4",
                                                   "cargfx/skytexcoast.ci4"};
/* @ghidraAddress BRD3D: 0x100b5bd8 */
static const SkyTextureFiles g_MineDaySkyFiles = {"cargfx/skytexmine.lut4",
                                                  "cargfx/skytexmine.ci4"};
/* @ghidraAddress BRD3D: 0x100b5c10 */
static const SkyTextureFiles g_AmazonDaySkyFiles = {"cargfx/skytexamazon.lut4",
                                                    "cargfx/skytexamazon.ci4"};
/* @ghidraAddress BRD3D: 0x100b5c48 */
static const SkyTextureFiles g_DesertNightSkyFiles = {"cargfx/skytexdesertn.lut4",
                                                      "cargfx/skytexdesertn.ci4"};
/* @ghidraAddress BRD3D: 0x100b5c88 */
static const SkyTextureFiles g_MountainNightSkyFiles = {"cargfx/skytexmountainn.lut4",
                                                        "cargfx/skytexmountainn.ci4"};
/* @ghidraAddress BRD3D: 0x100b5cc8 */
static const SkyTextureFiles g_CoastNightSkyFiles = {"cargfx/skytexcoastn.lut4",
                                                     "cargfx/skytexcoastn.ci4"};
/* @ghidraAddress BRD3D: 0x100b5d00 */
static const SkyTextureFiles g_MineNightSkyFiles = {"cargfx/skytexminen.lut4",
                                                    "cargfx/skytexminen.ci4"};
/* @ghidraAddress BRD3D: 0x100b5d38 */
static const SkyTextureFiles g_AmazonNightSkyFiles = {"cargfx/skytexamazonn.lut4",
                                                      "cargfx/skytexamazonn.ci4"};

TrackMenuItem g_aTrackDescriptors[16] = {
    {{"Desert", 0x0, NULL, &g_auUnusedTrackSegmentMarkers[0], &g_auUnusedTrackSegmentMarkers[1]},
     &g_auUnusedTrackSegmentMarkers[20],
     &g_auUnusedTrackSegmentMarkers[21],
     &g_DesertDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[32],
     &g_DesertNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[37],
     {
         {
             {{57.0f, 22.0f, 22.0f, 22.0f, 22.0f, 22.0f}, 60.0f},
             {{57.0f, 24.0f, 22.0f, 22.0f, 22.0f, 22.0f}, 60.0f},
             {{57.0f, 25.0f, 23.0f, 23.0f, 23.0f, 23.0f}, 60.0f},
         },
         {
             {{57.0f, 22.0f, 21.0f, 22.0f, 21.0f, 16.0f}, 60.0f},
             {{57.0f, 24.0f, 21.0f, 22.0f, 21.0f, 16.0f}, 60.0f},
             {{57.0f, 25.0f, 22.0f, 23.0f, 22.0f, 17.0f}, 60.0f},
         },
         {
             {{33.0f, 27.0f, 22.0f, 22.0f, 19.0f, 18.0f}, 40.0f},
             {{33.0f, 27.0f, 24.0f, 24.0f, 20.0f, 18.0f}, 40.0f},
             {{33.0f, 30.0f, 27.0f, 27.0f, 22.0f, 19.0f}, 40.0f},
         },
         {
             {{33.0f, 27.0f, 22.0f, 21.0f, 19.0f, 18.0f}, 40.0f},
             {{33.0f, 27.0f, 24.0f, 21.0f, 19.0f, 18.0f}, 40.0f},
             {{33.0f, 30.0f, 27.0f, 24.0f, 22.0f, 21.0f}, 40.0f},
         },
     }},
    {{"Mountain", 0x0, NULL, &g_auUnusedTrackSegmentMarkers[2], &g_auUnusedTrackSegmentMarkers[3]},
     &g_auUnusedTrackSegmentMarkers[22],
     &g_auUnusedTrackSegmentMarkers[23],
     &g_MountainDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[33],
     &g_MountainNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[38],
     {
         {
             {{87.0f, 24.0f, 19.0f, 19.0f, 19.0f, 14.0f}, 90.0f},
             {{87.0f, 26.0f, 21.0f, 21.0f, 21.0f, 14.0f}, 90.0f},
             {{87.0f, 31.0f, 26.0f, 26.0f, 26.0f, 20.0f}, 90.0f},
         },
         {
             {{83.0f, 18.0f, 18.0f, 18.0f, 18.0f, 14.0f}, 90.0f},
             {{83.0f, 20.0f, 20.0f, 19.0f, 19.0f, 15.0f}, 90.0f},
             {{83.0f, 25.0f, 25.0f, 24.0f, 24.0f, 20.0f}, 90.0f},
         },
         {
             {{45.0f, 29.0f, 27.0f, 20.0f, 19.0f, 12.0f}, 50.0f},
             {{45.0f, 30.0f, 28.0f, 21.0f, 20.0f, 11.0f}, 50.0f},
             {{55.0f, 34.0f, 32.0f, 25.0f, 26.0f, 15.0f}, 60.0f},
         },
         {
             {{45.0f, 28.0f, 25.0f, 19.0f, 18.0f, 11.0f}, 50.0f},
             {{45.0f, 28.0f, 27.0f, 19.0f, 17.0f, 10.0f}, 50.0f},
             {{55.0f, 31.0f, 30.0f, 22.0f, 20.0f, 10.0f}, 60.0f},
         },
     }},
    {{"Coastline", 0x0, NULL, &g_auUnusedTrackSegmentMarkers[6], &g_auUnusedTrackSegmentMarkers[7]},
     &g_auUnusedTrackSegmentMarkers[24],
     &g_auUnusedTrackSegmentMarkers[25],
     &g_CoastDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[34],
     &g_CoastNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[39],
     {
         {
             {{47.0f, 17.0f, 17.0f, 17.0f, 17.0f, 17.0f}, 50.0f},
             {{47.0f, 18.0f, 18.0f, 17.0f, 17.0f, 17.0f}, 50.0f},
             {{47.0f, 20.0f, 20.0f, 19.0f, 19.0f, 17.0f}, 50.0f},
         },
         {
             {{45.0f, 14.0f, 13.0f, 13.0f, 13.0f, 13.0f}, 50.0f},
             {{45.0f, 15.0f, 14.0f, 13.0f, 13.0f, 13.0f}, 50.0f},
             {{45.0f, 17.0f, 16.0f, 15.0f, 15.0f, 15.0f}, 50.0f},
         },
         {
             {{30.0f, 14.0f, 13.0f, 13.0f, 12.0f, 11.0f}, 40.0f},
             {{30.0f, 16.0f, 14.0f, 13.0f, 12.0f, 11.0f}, 40.0f},
             {{30.0f, 18.0f, 16.0f, 15.0f, 14.0f, 13.0f}, 40.0f},
         },
         {
             {{30.0f, 13.0f, 13.0f, 12.0f, 12.0f, 11.0f}, 40.0f},
             {{30.0f, 14.0f, 14.0f, 12.0f, 12.0f, 11.0f}, 40.0f},
             {{30.0f, 16.0f, 16.0f, 14.0f, 14.0f, 14.0f}, 40.0f},
         },
     }},
    {{"Strip Mine",
      0x0,
      NULL,
      &g_auUnusedTrackSegmentMarkers[4],
      &g_auUnusedTrackSegmentMarkers[5]},
     &g_auUnusedTrackSegmentMarkers[26],
     &g_auUnusedTrackSegmentMarkers[27],
     &g_MineDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[35],
     &g_MineNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[40],
     {
         {
             {{57.0f, 14.0f, 14.0f, 14.0f, 14.0f, 14.0f}, 60.0f},
             {{57.0f, 16.0f, 16.0f, 16.0f, 15.0f, 14.0f}, 60.0f},
             {{57.0f, 16.0f, 16.0f, 16.0f, 15.0f, 14.0f}, 60.0f},
         },
         {
             {{54.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f}, 58.0f},
             {{54.0f, 14.0f, 14.0f, 14.0f, 13.0f, 12.0f}, 58.0f},
             {{54.0f, 14.0f, 14.0f, 14.0f, 13.0f, 12.0f}, 58.0f},
         },
         {
             {{20.0f, 15.0f, 19.0f, 21.0f, 9.0f, 0.0f}, 25.0f},
             {{20.0f, 17.0f, 21.0f, 20.0f, 10.0f, 0.0f}, 25.0f},
             {{25.0f, 18.0f, 22.0f, 22.0f, 10.0f, 0.0f}, 30.0f},
         },
         {
             {{20.0f, 15.0f, 19.0f, 21.0f, 9.0f, 0.0f}, 25.0f},
             {{20.0f, 17.0f, 21.0f, 22.0f, 10.0f, 0.0f}, 25.0f},
             {{25.0f, 17.0f, 21.0f, 22.0f, 10.0f, 0.0f}, 30.0f},
         },
     }},
    {{"Jungle", 0x0, NULL, &g_auUnusedTrackSegmentMarkers[8], &g_auUnusedTrackSegmentMarkers[9]},
     &g_auUnusedTrackSegmentMarkers[28],
     &g_auUnusedTrackSegmentMarkers[29],
     &g_AmazonDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[36],
     &g_AmazonNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[41],
     {
         {
             {{57.0f, 19.0f, 19.0f, 19.0f, 19.0f, 19.0f}, 60.0f},
             {{57.0f, 21.0f, 19.0f, 19.0f, 19.0f, 19.0f}, 60.0f},
             {{57.0f, 23.0f, 21.0f, 21.0f, 21.0f, 21.0f}, 60.0f},
         },
         {
             {{54.0f, 17.0f, 17.0f, 17.0f, 16.0f, 16.0f}, 60.0f},
             {{54.0f, 19.0f, 17.0f, 17.0f, 16.0f, 16.0f}, 60.0f},
             {{54.0f, 21.0f, 19.0f, 19.0f, 18.0f, 18.0f}, 60.0f},
         },
         {
             {{25.0f, 22.0f, 22.0f, 22.0f, 20.0f, 17.0f}, 30.0f},
             {{27.0f, 24.0f, 23.0f, 22.0f, 20.0f, 17.0f}, 30.0f},
             {{27.0f, 26.0f, 25.0f, 24.0f, 22.0f, 20.0f}, 30.0f},
         },
         {
             {{25.0f, 22.0f, 21.0f, 21.0f, 20.0f, 17.0f}, 30.0f},
             {{27.0f, 24.0f, 23.0f, 21.0f, 20.0f, 17.0f}, 30.0f},
             {{27.0f, 26.0f, 25.0f, 23.0f, 22.0f, 19.0f}, 30.0f},
         },
     }},
    {{"Race Track",
      0x0,
      NULL,
      &g_auUnusedTrackSegmentMarkers[10],
      &g_auUnusedTrackSegmentMarkers[11]},
     &g_auUnusedTrackSegmentMarkers[22],
     &g_auUnusedTrackSegmentMarkers[23],
     &g_MountainDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[33],
     &g_MountainNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[38],
     {
         {
             {{87.0f, 24.0f, 19.0f, 19.0f, 19.0f, 14.0f}, 90.0f},
             {{87.0f, 26.0f, 21.0f, 21.0f, 21.0f, 14.0f}, 90.0f},
             {{87.0f, 31.0f, 26.0f, 26.0f, 26.0f, 20.0f}, 90.0f},
         },
         {
             {{83.0f, 18.0f, 18.0f, 18.0f, 18.0f, 14.0f}, 90.0f},
             {{83.0f, 20.0f, 20.0f, 19.0f, 19.0f, 15.0f}, 90.0f},
             {{83.0f, 25.0f, 25.0f, 24.0f, 24.0f, 20.0f}, 90.0f},
         },
         {
             {{45.0f, 29.0f, 27.0f, 20.0f, 19.0f, 12.0f}, 50.0f},
             {{45.0f, 30.0f, 28.0f, 21.0f, 20.0f, 11.0f}, 50.0f},
             {{55.0f, 34.0f, 32.0f, 25.0f, 26.0f, 15.0f}, 60.0f},
         },
         {
             {{45.0f, 28.0f, 25.0f, 19.0f, 18.0f, 11.0f}, 50.0f},
             {{45.0f, 28.0f, 27.0f, 19.0f, 17.0f, 10.0f}, 50.0f},
             {{55.0f, 31.0f, 30.0f, 22.0f, 20.0f, 10.0f}, 60.0f},
         },
     }},
    {{"Mirror Desert",
      0x10,
      NULL,
      &g_auUnusedTrackSegmentMarkers[0],
      &g_auUnusedTrackSegmentMarkers[1]},
     &g_auUnusedTrackSegmentMarkers[20],
     &g_auUnusedTrackSegmentMarkers[21],
     &g_DesertDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[32],
     &g_DesertNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[37],
     {
         {
             {{57.0f, 22.0f, 22.0f, 22.0f, 22.0f, 22.0f}, 60.0f},
             {{57.0f, 24.0f, 22.0f, 22.0f, 22.0f, 22.0f}, 60.0f},
             {{57.0f, 25.0f, 23.0f, 23.0f, 23.0f, 23.0f}, 60.0f},
         },
         {
             {{57.0f, 22.0f, 21.0f, 22.0f, 21.0f, 16.0f}, 60.0f},
             {{57.0f, 24.0f, 21.0f, 22.0f, 21.0f, 16.0f}, 60.0f},
             {{57.0f, 25.0f, 22.0f, 23.0f, 22.0f, 17.0f}, 60.0f},
         },
         {
             {{33.0f, 27.0f, 22.0f, 22.0f, 19.0f, 18.0f}, 40.0f},
             {{33.0f, 27.0f, 24.0f, 24.0f, 20.0f, 18.0f}, 40.0f},
             {{33.0f, 30.0f, 27.0f, 27.0f, 22.0f, 19.0f}, 40.0f},
         },
         {
             {{33.0f, 27.0f, 22.0f, 21.0f, 19.0f, 18.0f}, 40.0f},
             {{33.0f, 27.0f, 24.0f, 21.0f, 19.0f, 18.0f}, 40.0f},
             {{33.0f, 30.0f, 27.0f, 24.0f, 22.0f, 21.0f}, 40.0f},
         },
     }},
    {{"Mirror Mountain",
      0x10,
      NULL,
      &g_auUnusedTrackSegmentMarkers[2],
      &g_auUnusedTrackSegmentMarkers[3]},
     &g_auUnusedTrackSegmentMarkers[22],
     &g_auUnusedTrackSegmentMarkers[23],
     &g_MountainDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[33],
     &g_MountainNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[38],
     {
         {
             {{87.0f, 24.0f, 19.0f, 19.0f, 19.0f, 14.0f}, 90.0f},
             {{87.0f, 26.0f, 21.0f, 21.0f, 21.0f, 14.0f}, 90.0f},
             {{87.0f, 31.0f, 26.0f, 26.0f, 26.0f, 20.0f}, 90.0f},
         },
         {
             {{83.0f, 18.0f, 18.0f, 18.0f, 18.0f, 14.0f}, 90.0f},
             {{83.0f, 20.0f, 20.0f, 19.0f, 19.0f, 15.0f}, 90.0f},
             {{83.0f, 25.0f, 25.0f, 24.0f, 24.0f, 20.0f}, 90.0f},
         },
         {
             {{45.0f, 29.0f, 27.0f, 20.0f, 19.0f, 12.0f}, 50.0f},
             {{45.0f, 30.0f, 28.0f, 21.0f, 20.0f, 11.0f}, 50.0f},
             {{55.0f, 34.0f, 32.0f, 25.0f, 26.0f, 15.0f}, 60.0f},
         },
         {
             {{45.0f, 28.0f, 25.0f, 19.0f, 18.0f, 11.0f}, 50.0f},
             {{45.0f, 28.0f, 27.0f, 19.0f, 17.0f, 10.0f}, 50.0f},
             {{55.0f, 31.0f, 30.0f, 22.0f, 20.0f, 10.0f}, 60.0f},
         },
     }},
    {{"Mirror Coastline",
      0x10,
      NULL,
      &g_auUnusedTrackSegmentMarkers[6],
      &g_auUnusedTrackSegmentMarkers[7]},
     &g_auUnusedTrackSegmentMarkers[24],
     &g_auUnusedTrackSegmentMarkers[25],
     &g_CoastDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[34],
     &g_CoastNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[39],
     {
         {
             {{47.0f, 17.0f, 17.0f, 17.0f, 17.0f, 17.0f}, 50.0f},
             {{47.0f, 18.0f, 18.0f, 17.0f, 17.0f, 17.0f}, 50.0f},
             {{47.0f, 20.0f, 20.0f, 19.0f, 19.0f, 17.0f}, 50.0f},
         },
         {
             {{45.0f, 14.0f, 13.0f, 13.0f, 13.0f, 13.0f}, 50.0f},
             {{45.0f, 15.0f, 14.0f, 13.0f, 13.0f, 13.0f}, 50.0f},
             {{45.0f, 17.0f, 16.0f, 15.0f, 15.0f, 15.0f}, 50.0f},
         },
         {
             {{30.0f, 14.0f, 13.0f, 13.0f, 12.0f, 11.0f}, 40.0f},
             {{30.0f, 16.0f, 14.0f, 13.0f, 12.0f, 11.0f}, 40.0f},
             {{30.0f, 18.0f, 16.0f, 15.0f, 14.0f, 13.0f}, 40.0f},
         },
         {
             {{30.0f, 13.0f, 13.0f, 12.0f, 12.0f, 11.0f}, 40.0f},
             {{30.0f, 14.0f, 14.0f, 12.0f, 12.0f, 11.0f}, 40.0f},
             {{30.0f, 16.0f, 16.0f, 14.0f, 14.0f, 14.0f}, 40.0f},
         },
     }},
    {{"Mirror Strip Mine",
      0x10,
      NULL,
      &g_auUnusedTrackSegmentMarkers[4],
      &g_auUnusedTrackSegmentMarkers[5]},
     &g_auUnusedTrackSegmentMarkers[26],
     &g_auUnusedTrackSegmentMarkers[27],
     &g_MineDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[35],
     &g_MineNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[40],
     {
         {
             {{57.0f, 14.0f, 14.0f, 14.0f, 14.0f, 14.0f}, 60.0f},
             {{57.0f, 16.0f, 16.0f, 16.0f, 15.0f, 14.0f}, 60.0f},
             {{57.0f, 16.0f, 16.0f, 16.0f, 15.0f, 14.0f}, 60.0f},
         },
         {
             {{54.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f}, 58.0f},
             {{54.0f, 14.0f, 14.0f, 14.0f, 13.0f, 12.0f}, 58.0f},
             {{54.0f, 14.0f, 14.0f, 14.0f, 13.0f, 12.0f}, 58.0f},
         },
         {
             {{20.0f, 15.0f, 19.0f, 21.0f, 9.0f, 0.0f}, 25.0f},
             {{20.0f, 17.0f, 21.0f, 20.0f, 10.0f, 0.0f}, 25.0f},
             {{25.0f, 18.0f, 22.0f, 22.0f, 10.0f, 0.0f}, 30.0f},
         },
         {
             {{20.0f, 15.0f, 19.0f, 21.0f, 9.0f, 0.0f}, 25.0f},
             {{20.0f, 17.0f, 21.0f, 22.0f, 10.0f, 0.0f}, 25.0f},
             {{25.0f, 17.0f, 21.0f, 22.0f, 10.0f, 0.0f}, 30.0f},
         },
     }},
    {{"Mirror Jungle",
      0x10,
      NULL,
      &g_auUnusedTrackSegmentMarkers[8],
      &g_auUnusedTrackSegmentMarkers[9]},
     &g_auUnusedTrackSegmentMarkers[28],
     &g_auUnusedTrackSegmentMarkers[29],
     &g_AmazonDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[36],
     &g_AmazonNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[41],
     {
         {
             {{57.0f, 19.0f, 19.0f, 19.0f, 19.0f, 19.0f}, 60.0f},
             {{57.0f, 21.0f, 19.0f, 19.0f, 19.0f, 19.0f}, 60.0f},
             {{57.0f, 23.0f, 21.0f, 21.0f, 21.0f, 21.0f}, 60.0f},
         },
         {
             {{54.0f, 17.0f, 17.0f, 17.0f, 16.0f, 16.0f}, 60.0f},
             {{54.0f, 19.0f, 17.0f, 17.0f, 16.0f, 16.0f}, 60.0f},
             {{54.0f, 21.0f, 19.0f, 19.0f, 18.0f, 18.0f}, 60.0f},
         },
         {
             {{25.0f, 22.0f, 22.0f, 22.0f, 20.0f, 17.0f}, 30.0f},
             {{27.0f, 24.0f, 23.0f, 22.0f, 20.0f, 17.0f}, 30.0f},
             {{27.0f, 26.0f, 25.0f, 24.0f, 22.0f, 20.0f}, 30.0f},
         },
         {
             {{25.0f, 22.0f, 21.0f, 21.0f, 20.0f, 17.0f}, 30.0f},
             {{27.0f, 24.0f, 23.0f, 21.0f, 20.0f, 17.0f}, 30.0f},
             {{27.0f, 26.0f, 25.0f, 23.0f, 22.0f, 19.0f}, 30.0f},
         },
     }},
    {{"Mirror Race Track",
      0x10,
      NULL,
      &g_auUnusedTrackSegmentMarkers[10],
      &g_auUnusedTrackSegmentMarkers[11]},
     &g_auUnusedTrackSegmentMarkers[22],
     &g_auUnusedTrackSegmentMarkers[23],
     &g_MountainDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[33],
     &g_MountainNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[38],
     {
         {
             {{87.0f, 24.0f, 19.0f, 19.0f, 19.0f, 14.0f}, 90.0f},
             {{87.0f, 26.0f, 21.0f, 21.0f, 21.0f, 14.0f}, 90.0f},
             {{87.0f, 31.0f, 26.0f, 26.0f, 26.0f, 20.0f}, 90.0f},
         },
         {
             {{83.0f, 18.0f, 18.0f, 18.0f, 18.0f, 14.0f}, 90.0f},
             {{83.0f, 20.0f, 20.0f, 19.0f, 19.0f, 15.0f}, 90.0f},
             {{83.0f, 25.0f, 25.0f, 24.0f, 24.0f, 20.0f}, 90.0f},
         },
         {
             {{45.0f, 29.0f, 27.0f, 20.0f, 19.0f, 12.0f}, 50.0f},
             {{45.0f, 30.0f, 28.0f, 21.0f, 20.0f, 11.0f}, 50.0f},
             {{55.0f, 34.0f, 32.0f, 25.0f, 26.0f, 15.0f}, 60.0f},
         },
         {
             {{45.0f, 28.0f, 25.0f, 19.0f, 18.0f, 11.0f}, 50.0f},
             {{45.0f, 28.0f, 27.0f, 19.0f, 17.0f, 10.0f}, 50.0f},
             {{55.0f, 31.0f, 30.0f, 22.0f, 20.0f, 10.0f}, 60.0f},
         },
     }},
    {{"Load and Gamewin",
      0x2,
      NULL,
      &g_auUnusedTrackSegmentMarkers[12],
      &g_auUnusedTrackSegmentMarkers[13]},
     &g_auUnusedTrackSegmentMarkers[30],
     &g_auUnusedTrackSegmentMarkers[31],
     &g_DesertDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[32],
     &g_DesertNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[37],
     {
         {
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
         },
         {
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
         },
         {
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
         },
         {
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
         },
     }},
    {{"Mirror Bonus Track",
      0x10,
      NULL,
      &g_auUnusedTrackSegmentMarkers[10],
      &g_auUnusedTrackSegmentMarkers[11]},
     &g_auUnusedTrackSegmentMarkers[22],
     &g_auUnusedTrackSegmentMarkers[23],
     &g_MountainDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[33],
     &g_MountainNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[38],
     {
         {
             {{87.0f, 24.0f, 19.0f, 19.0f, 19.0f, 14.0f}, 90.0f},
             {{87.0f, 26.0f, 21.0f, 21.0f, 21.0f, 14.0f}, 90.0f},
             {{87.0f, 31.0f, 26.0f, 26.0f, 26.0f, 20.0f}, 90.0f},
         },
         {
             {{83.0f, 18.0f, 18.0f, 18.0f, 18.0f, 14.0f}, 90.0f},
             {{83.0f, 20.0f, 20.0f, 19.0f, 19.0f, 15.0f}, 90.0f},
             {{83.0f, 25.0f, 25.0f, 24.0f, 24.0f, 20.0f}, 90.0f},
         },
         {
             {{45.0f, 29.0f, 27.0f, 20.0f, 19.0f, 12.0f}, 50.0f},
             {{45.0f, 30.0f, 28.0f, 21.0f, 20.0f, 11.0f}, 50.0f},
             {{55.0f, 34.0f, 32.0f, 25.0f, 26.0f, 15.0f}, 60.0f},
         },
         {
             {{45.0f, 28.0f, 25.0f, 19.0f, 18.0f, 11.0f}, 50.0f},
             {{45.0f, 28.0f, 27.0f, 19.0f, 17.0f, 10.0f}, 50.0f},
             {{55.0f, 31.0f, 30.0f, 22.0f, 20.0f, 10.0f}, 60.0f},
         },
     }},
    {{"Bonus Track",
      0x0,
      NULL,
      &g_auUnusedTrackSegmentMarkers[10],
      &g_auUnusedTrackSegmentMarkers[11]},
     &g_auUnusedTrackSegmentMarkers[22],
     &g_auUnusedTrackSegmentMarkers[23],
     &g_MountainDaySkyFiles,
     &g_auUnusedTrackSegmentMarkers[33],
     &g_MountainNightSkyFiles,
     &g_auUnusedTrackSegmentMarkers[38],
     {
         {
             {{87.0f, 24.0f, 19.0f, 19.0f, 19.0f, 14.0f}, 90.0f},
             {{87.0f, 26.0f, 21.0f, 21.0f, 21.0f, 14.0f}, 90.0f},
             {{87.0f, 31.0f, 26.0f, 26.0f, 26.0f, 20.0f}, 90.0f},
         },
         {
             {{83.0f, 18.0f, 18.0f, 18.0f, 18.0f, 14.0f}, 90.0f},
             {{83.0f, 20.0f, 20.0f, 19.0f, 19.0f, 15.0f}, 90.0f},
             {{83.0f, 25.0f, 25.0f, 24.0f, 24.0f, 20.0f}, 90.0f},
         },
         {
             {{45.0f, 29.0f, 27.0f, 20.0f, 19.0f, 12.0f}, 50.0f},
             {{45.0f, 30.0f, 28.0f, 21.0f, 20.0f, 11.0f}, 50.0f},
             {{55.0f, 34.0f, 32.0f, 25.0f, 26.0f, 15.0f}, 60.0f},
         },
         {
             {{45.0f, 28.0f, 25.0f, 19.0f, 18.0f, 11.0f}, 50.0f},
             {{45.0f, 28.0f, 27.0f, 19.0f, 17.0f, 10.0f}, 50.0f},
             {{55.0f, 31.0f, 30.0f, 22.0f, 20.0f, 10.0f}, 60.0f},
         },
     }},
    {{"Reset To First Year",
      0x2,
      NULL,
      &g_auUnusedTrackSegmentMarkers[18],
      &g_auUnusedTrackSegmentMarkers[19]},
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     {
         {
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
         },
         {
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
         },
         {
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
         },
         {
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
             {{30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f}, 120.0f},
         },
     }},
};

TrackMenuItem *g_apTrackMenuItems[16] = {
    &g_aTrackDescriptors[0],
    &g_aTrackDescriptors[1],
    &g_aTrackDescriptors[2],
    &g_aTrackDescriptors[3],
    &g_aTrackDescriptors[4],
    &g_aTrackDescriptors[5],
    &g_aTrackDescriptors[6],
    &g_aTrackDescriptors[7],
    &g_aTrackDescriptors[8],
    &g_aTrackDescriptors[9],
    &g_aTrackDescriptors[10],
    &g_aTrackDescriptors[11],
    &g_aTrackDescriptors[12],
    &g_aTrackDescriptors[13],
    &g_aTrackDescriptors[14],
    &g_aTrackDescriptors[15],
};
