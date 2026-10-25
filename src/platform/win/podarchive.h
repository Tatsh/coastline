#pragma once

/**
 * @file
 * The archives of the game, the pod files that store its data.
 */

/** A pod file the game reads. The layout is not recovered yet. */
class PodArchive {
public:
    /**
     * Close the file of the archive.
     *
     * @ghidraAddress BRD3D: 0x10008970
     */
    void ClosePodFile();
};

/**
 * The archive of the data of the game.
 *
 * @ghidraAddress BRD3D: 0x10a99780
 */
extern PodArchive g_BossRallyPodArchive;
