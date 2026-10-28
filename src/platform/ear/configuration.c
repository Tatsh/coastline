#include "configuration.h"

#include "error.h"

int __stdcall EAR_DLL_AAA_Validate(uint32_t code) {
    SetEarLastError(0);
    if (!ValidateEarNumericCode(code)) {
        SetEarLastError(1);
        return 0;
    }
    LoadEarConfiguration(code);
    return 1;
}
