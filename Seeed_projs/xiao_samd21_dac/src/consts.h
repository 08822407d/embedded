#include "config.h"


#define VOLT_TO_DAC_VALUE(x) ((x) / VDDA * (1 << DAC_RESOL_BITS))    // x between 0 ~ 10.0V