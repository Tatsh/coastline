#ifndef COASTLINE_EAR_BANK_H
#define COASTLINE_EAR_BANK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @param bank The bank.
 * @param data The data of the bank.
 * @return 0.
 * @ghidraAddress EAR: 0x1000de50
 */
int __stdcall EarRegisterBankExportWrapper(uint32_t bank, uint32_t data);

/**
 * Reject the registration of a bank, which this library does not support.
 *
 * @param bank The bank.
 * @param data The data of the bank.
 * @return 0.
 * @ghidraAddress EAR: 0x1000de64
 */
int __stdcall EAR_DLL_RegisterBank(uint32_t bank, uint32_t data);

/**
 * @param bank The bank.
 * @return 0.
 * @ghidraAddress EAR: 0x1000f070
 */
int __stdcall EarShutDownBankExportWrapper(uint32_t bank);

/**
 * Reject the shutdown of a bank, which this library does not support.
 *
 * @param bank The bank.
 * @return 0.
 * @ghidraAddress EAR: 0x1000f080
 */
int __stdcall EAR_DLL_ShutDownBank(uint32_t bank);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_BANK_H */
