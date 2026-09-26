#include "bms_injection.h"

void BmsInjection_Init(BmsInjection *injection)
{
    if (injection != 0) {
        injection->cell0_mv = 0U;
        injection->set_at_ms = 0U;
        injection->enabled = 0U;
    }
}

uint8_t BmsInjection_Set(BmsInjection *injection,
                         uint16_t cell0_mv,
                         uint32_t now_ms)
{
    if ((injection == 0) || (cell0_mv > BMS_INJECTION_MAX_MV)) {
        return 0U;
    }

    injection->cell0_mv = cell0_mv;
    injection->set_at_ms = now_ms;
    injection->enabled = 1U;
    return 1U;
}

void BmsInjection_Clear(BmsInjection *injection)
{
    if (injection != 0) {
        injection->enabled = 0U;
    }
}

uint8_t BmsInjection_Apply(BmsInjection *injection,
                           const BmsData *measured,
                           uint32_t now_ms,
                           BmsData *effective)
{
    if ((measured == 0) || (effective == 0)) {
        return 0U;
    }

    *effective = *measured;
    if (injection == 0) {
        return 0U;
    }

    if ((injection->enabled != 0U)
        && ((uint32_t)(now_ms - injection->set_at_ms)
            >= BMS_INJECTION_TIMEOUT_MS)) {
        injection->enabled = 0U;
    }

    if (injection->enabled != 0U) {
        effective->cell_voltage_mv[0] = injection->cell0_mv;
        return 1U;
    }
    return 0U;
}
