#ifndef BMS_INJECTION_H
#define BMS_INJECTION_H

#include "bms_types.h"

/* Teaching-only virtual PA0 input. Never substitute this for a real BMS sensor. */
#define BMS_INJECTION_MAX_MV     3300U
#define BMS_INJECTION_TIMEOUT_MS 30000U

typedef struct
{
    uint16_t cell0_mv;
    uint32_t set_at_ms;
    uint8_t enabled;
} BmsInjection;

void BmsInjection_Init(BmsInjection *injection);
uint8_t BmsInjection_Set(BmsInjection *injection,
                         uint16_t cell0_mv,
                         uint32_t now_ms);
void BmsInjection_Clear(BmsInjection *injection);
uint8_t BmsInjection_Apply(BmsInjection *injection,
                           const BmsData *measured,
                           uint32_t now_ms,
                           BmsData *effective);

#endif /* BMS_INJECTION_H */
