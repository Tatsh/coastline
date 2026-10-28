#include "bank.h"

#include "error.h"

int __stdcall EarRegisterBankExportWrapper(uint32_t bank, uint32_t data) {
    return EAR_DLL_RegisterBank(bank, data);
}

int __stdcall EAR_DLL_RegisterBank(uint32_t bank, uint32_t data) {
    SetEarLastError(0);
    SetEarLastError(0x17);
    return 0;
}

int __stdcall EarShutDownBankExportWrapper(uint32_t bank) {
    return EAR_DLL_ShutDownBank(bank);
}

int __stdcall EAR_DLL_ShutDownBank(uint32_t bank) {
    SetEarLastError(0);
    SetEarLastError(0x17);
    return 0;
}
