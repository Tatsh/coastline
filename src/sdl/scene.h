#ifndef COASTLINE_SDL_SCENE_H
#define COASTLINE_SDL_SCENE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The race the SDL game starts, as the menus would choose it. The defaults are the race of the
 * benchmark of the PC release.
 */
typedef struct SceneSelection {
    int gameMode;     /*!< A GAME_MODE_ value. */
    int track;        /*!< The track. */
    int car;          /*!< The car. */
    int weather;      /*!< The weather. */
    int tires;        /*!< The tyres. */
    int handling;     /*!< The handling. */
    int suspension;   /*!< The suspension. */
    int transmission; /*!< The transmission. */
} SceneSelection;

/** The race the SDL game starts. */
extern SceneSelection g_SdlScene;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_SCENE_H */
