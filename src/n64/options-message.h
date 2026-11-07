#ifndef COASTLINE_N64_OPTIONS_MESSAGE_H
#define COASTLINE_N64_OPTIONS_MESSAGE_H

/* The message that asks the player to swap the Rumble Pak for the Controller Pak, with its OK and
 * CANCEL buttons. */
#define DRAW_SWAP_FOR_CONTROLLER_PAK_MESSAGE(boxY, textY, buttonY, okText, cancelText)             \
    {                                                                                              \
        DRAW_MESSAGE_BOX(0x79, (boxY), 0x18D, 0xA0);                                               \
        DrawText("PLEASE REMOVE THE RUMBLE PAK", 0x44, (textY));                                   \
        DrawText("AND INSERT THE CONTROLLER PAK", 0x44, (textY) + 0xD);                            \
        DrawText("INTO THE CONTROLLER.  PRESS", 0x44, (textY) + 0x1A);                             \
        DrawText("THE A BUTTON WHEN READY.", 0x44, (textY) + 0x27);                                \
        SetTextAlignLeft();                                                                        \
        SetTextSize(10);                                                                           \
        DrawText((okText), (u32)(g_texButtonA.width + 0xE3) >> 1, ((buttonY) + 0x12) >> 1);        \
        DrawText((cancelText), (u32)(g_texButtonB.width + 0x144) >> 1, ((buttonY) + 0x12) >> 1);   \
        DrawTexturedSprite(&g_texButtonA, 0xDD, (buttonY));                                        \
        DrawTexturedSprite(&g_texButtonB, 0x13E, (buttonY));                                       \
    }

/* The message that asks the player to put the Rumble Pak back. */
#define DRAW_SWAP_FOR_RUMBLE_PAK_MESSAGE(boxX, boxY, boxWidth, textY)                              \
    {                                                                                              \
        DRAW_MESSAGE_BOX((boxX), (boxY), (boxWidth), 0x98);                                        \
        DRAW_SWAP_FOR_RUMBLE_PAK_TEXT(textY);                                                      \
    }

/* The text of the message to swap the Controller Pak for the Rumble Pak. */
#define DRAW_SWAP_FOR_RUMBLE_PAK_TEXT(textY)                                                       \
    {                                                                                              \
        DrawText("IF THE RUMBLE PAK IS TO BE USED,", 0x41, (textY));                               \
        DrawText("REMOVE THE CONTROLLER PAK AND", 0x41, (textY) + 0xD);                            \
        DrawText("INSERT THE RUMBLE PAK INTO THE", 0x41, (textY) + 0x1A);                          \
        DrawText("CONTROLLER.  PRESS THE A BUTTON", 0x41, (textY) + 0x27);                         \
        DrawText("TO CONTINUE.", 0x41, (textY) + 0x34);                                            \
    }

/* The message that the Controller Pak lacks the room for a note. */
#define DRAW_INSUFFICIENT_PAGES_MESSAGE(textY, pages, data)                                        \
    {                                                                                              \
        DrawText("INSUFFICIENT FREE PAGES", 0x5A, (textY));                                        \
        DrawText("OR FREE NOTES IN THE", 0x5A, (textY) + 0xD);                                     \
        DrawText("CONTROLLER PAK.", 0x5A, (textY) + 0x1A);                                         \
        DrawText((pages), 0x5A, (textY) + 0x2D);                                                   \
        DrawText("ARE NEEDED TO SAVE THE", 0x5A, (textY) + 0x3A);                                  \
        DrawText((data), 0x5A, (textY) + 0x47);                                                    \
    }

/* A message of two lines in a box of the usual height. */
#define DRAW_SHORT_MESSAGE(boxX, boxWidth, textX, line1, line2)                                    \
    {                                                                                              \
        DRAW_MESSAGE_BOX((boxX), 0xD9, (boxWidth), 0x4A);                                          \
        DrawText((line1), (textX), 0x7C);                                                          \
        DrawText((line2), (textX), 0x8A);                                                          \
    }

/* The message that the data was loaded or saved, in a box whose top depends on the screen that
 * opened the save and load menu. */
#define DRAW_DONE_MESSAGE(boxX, boxWidth, top, line1, line2)                                       \
    {                                                                                              \
        DRAW_MESSAGE_BOX((boxX), (top), (boxWidth), 0x4A);                                         \
        DrawText((line1), 0x9F, ((top) + 0x20) >> 1);                                              \
        DrawText((line2), 0x9F, (((top) + 0x20) >> 1) + 0xE);                                      \
    }

#endif
