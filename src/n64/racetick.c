/* clang-format off */
#include "n64/racetick-aliases.h"
#include "n64/reference/racetick.c"
/* IDO needs local definitions to match, while the original data assembly supplies their address. */
#pragma weak g_dwAirplaneInstance
#pragma weak g_fStartLightsDrop
/* clang-format on */
