#pragma once

/**
 * @file
 * The archives of the game, the pod files that store its data.
 */

/** A pod file the game reads. The layout is not recovered yet. */
class PodArchive {
public:
    /**
     * Open the file of the archive.
     *
     * @ghidraAddress BRD3D: 0x100088d0
     * @ghidraAddress BRGlide: 0x10008ab0
     */
    void OpenPodFile();

    /**
     * Close the file of the archive.
     *
     * @ghidraAddress BRD3D: 0x10008970
     */
    void ClosePodFile();

    /**
     * Set the path of the file of the archive.
     *
     * @param pszPath The path.
     * @ghidraAddress BRD3D: 0x10008b40
     * @ghidraAddress BRGlide: 0x10008d20
     */
    void SetFilePath(const char *pszPath);
};

/**
 * The archive of the data of the game.
 *
 * @ghidraAddress BRD3D: 0x10a99780
 * @ghidraAddress BRGlide: 0x10ac0810
 */
extern PodArchive g_BossRallyPodArchive;
