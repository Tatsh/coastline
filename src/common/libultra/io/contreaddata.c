#include <PR/os_cont.h>

#include "common/inputpoll.h"
#include "common/types.h"

void osContGetReadData(OSContPad *data) {
    int steering;
    int acceleration;
    unsigned buttons = ReadInputDevicesButtonMask(&steering, &acceleration);

    data->stick_x = steering;
    data->stick_y = acceleration;
    data->button = 0;
    if (buttons & 0x10) {
        data->button = A_BUTTON | D_JPAD;
    }
    if (buttons & 0x4) {
        data->button |= A_BUTTON | U_JPAD;
    }
    if (buttons & 0x1) {
        data->button |= L_JPAD;
    }
    if (buttons & 0x2) {
        data->button |= R_JPAD;
    }
    if (buttons & 0x8) {
        data->button |= B_BUTTON;
    }
    if (buttons & 0x100) {
        data->button |= U_CBUTTONS;
    }
    if (buttons & 0x200) {
        data->button |= L_CBUTTONS;
    }
    if (buttons & 0x400) {
        data->button |= D_CBUTTONS;
    }
    if (buttons & 0x8000) {
        data->button |= START_BUTTON;
    }
    if (buttons & 0x20) {
        data->button |= R_TRIG;
    }
    if (buttons & 0x40) {
        data->button |= L_TRIG;
    }
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    /* The PC release reads one player, and the SDL game reads the second from its controller. */
    data[1].CONT_ERRNO =
        ReadSecondControllerPad(&data[1].button, &data[1].stick_x, &data[1].stick_y) ?
            0 :
            CONT_NO_RESPONSE_ERROR;
#endif
}
