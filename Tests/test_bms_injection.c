#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "bms_injection.h"
#include "bms_protection.h"

static void Step(BmsInjection *injection,
                 BmsProtection *protection,
                 BmsData *measured,
                 BmsData *effective,
                 const BmsConfig *config,
                 BmsFault *fault,
                 uint32_t now_ms)
{
    measured->timestamp_ms = now_ms;
    (void)BmsInjection_Apply(injection, measured, now_ms, effective);
    BmsProtection_Update(protection, effective, config, fault, 100U);
}

int main(void)
{
    BmsInjection injection;
    BmsProtection protection;
    BmsConfig config = {0};
    BmsData measured = {0};
    BmsData effective = {0};
    BmsFault fault = {0};
    uint32_t now_ms;

    measured.valid = 1U;
    measured.cell_voltage_mv[0] = 1500U;
    measured.cell_voltage_mv[1] = 1800U;
    config.monitored_cell_count = 1U;
    config.over_voltage_mv = 3000U;
    config.under_voltage_mv = 300U;
    config.voltage_hysteresis_mv = 100U;
    config.fault_confirm_ms = 300U;
    config.recovery_ms = 500U;

    BmsInjection_Init(&injection);
    BmsProtection_Init(&protection, &fault);
    assert(BmsInjection_Apply(&injection, &measured, 0U, &effective) == 0U);
    assert(effective.cell_voltage_mv[0] == 1500U);
    assert(BmsInjection_Set(&injection, 3200U, 0U) == 1U);
    assert(BmsInjection_Set(&injection, 3301U, 0U) == 0U);

    for (now_ms = 100U; now_ms <= 300U; now_ms += 100U) {
        Step(&injection, &protection, &measured, &effective,
             &config, &fault, now_ms);
    }
    assert(measured.cell_voltage_mv[0] == 1500U);
    assert(effective.cell_voltage_mv[0] == 3200U);
    assert(effective.cell_voltage_mv[1] == 1800U);
    assert(protection.state == BMS_PROTECTION_STATE_FAULT_ACTIVE);
    assert(fault.flags == BMS_FAULT_FLAG_CELL_OV);
    assert(fault.active_code == BMS_FAULT_CELL_OV);

    assert(BmsInjection_Set(&injection, 1500U, 300U) == 1U);
    for (now_ms = 400U; now_ms <= 800U; now_ms += 100U) {
        Step(&injection, &protection, &measured, &effective,
             &config, &fault, now_ms);
    }
    assert(protection.state == BMS_PROTECTION_STATE_NORMAL);
    assert(fault.flags == 0U);
    assert(fault.active_code == BMS_FAULT_NONE);
    assert(fault.latched == 1U);

    measured.valid = 0U;
    assert(BmsInjection_Apply(&injection, &measured, 900U, &effective) == 1U);
    assert(effective.valid == 0U);
    measured.valid = 1U;
    assert(BmsInjection_Set(&injection, 3200U, 1000U) == 1U);
    assert(BmsInjection_Apply(&injection, &measured, 30999U, &effective) == 1U);
    assert(BmsInjection_Apply(&injection, &measured, 31000U, &effective) == 0U);
    assert(effective.cell_voltage_mv[0] == 1500U);

    assert(BmsInjection_Set(&injection, 3200U, 0xFFFFFFF0UL) == 1U);
    assert(BmsInjection_Apply(&injection, &measured, 5U, &effective) == 1U);
    assert(BmsInjection_Apply(&injection, &measured, 0x00007520UL,
                              &effective) == 0U);
    BmsInjection_Clear(&injection);
    assert(BmsInjection_Apply(&injection, &measured, 0x00007521UL,
                              &effective) == 0U);

    puts("bms_injection tests passed");
    return 0;
}
