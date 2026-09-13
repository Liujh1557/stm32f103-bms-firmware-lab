#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bms_protection.h"

static void Step(BmsProtection *protection,
                 BmsData *data,
                 const BmsConfig *config,
                 BmsFault *fault,
                 uint16_t cell_mv)
{
    data->cell_voltage_mv[0] = cell_mv;
    data->timestamp_ms += 100U;
    BmsProtection_Update(protection, data, config, fault, 100U);
}

int main(void)
{
    BmsData data;
    BmsConfig config;
    BmsFault fault;
    BmsProtection protection;
    uint8_t i;

    memset(&data, 0, sizeof(data));
    memset(&config, 0, sizeof(config));
    data.valid = 1U;
    config.monitored_cell_count = 1U;
    config.over_voltage_mv = 3000U;
    config.under_voltage_mv = 300U;
    config.voltage_hysteresis_mv = 100U;
    config.fault_confirm_ms = 300U;
    config.recovery_ms = 500U;
    BmsProtection_Init(&protection, &fault);

    Step(&protection, &data, &config, &fault, 1500U);
    assert(protection.state == BMS_PROTECTION_STATE_NORMAL);

    Step(&protection, &data, &config, &fault, 0U);
    Step(&protection, &data, &config, &fault, 0U);
    Step(&protection, &data, &config, &fault, 1500U);
    assert(fault.flags == 0U);
    assert(protection.state == BMS_PROTECTION_STATE_NORMAL);

    for (i = 0U; i < 3U; i++) {
        Step(&protection, &data, &config, &fault, 0U);
    }
    assert(fault.flags == BMS_FAULT_FLAG_CELL_UV);
    assert(fault.active_code == BMS_FAULT_CELL_UV);
    assert(fault.latched == 1U);
    assert(protection.state == BMS_PROTECTION_STATE_FAULT_ACTIVE);

    Step(&protection, &data, &config, &fault, 350U);
    assert(protection.state == BMS_PROTECTION_STATE_FAULT_ACTIVE);

    for (i = 0U; i < 5U; i++) {
        Step(&protection, &data, &config, &fault, 500U);
    }
    assert(fault.flags == 0U);
    assert(fault.active_code == BMS_FAULT_NONE);
    assert(fault.latched == 1U);
    assert(protection.state == BMS_PROTECTION_STATE_NORMAL);

    for (i = 0U; i < 3U; i++) {
        Step(&protection, &data, &config, &fault, 3100U);
    }
    assert(fault.flags == BMS_FAULT_FLAG_CELL_OV);
    assert(fault.active_code == BMS_FAULT_CELL_OV);
    assert(protection.state == BMS_PROTECTION_STATE_FAULT_ACTIVE);

    data.valid = 0U;
    for (i = 0U; i < 10U; i++) {
        Step(&protection, &data, &config, &fault, 1500U);
    }
    assert(fault.flags == BMS_FAULT_FLAG_CELL_OV);
    assert(protection.state == BMS_PROTECTION_STATE_FAULT_ACTIVE);
    data.valid = 1U;

    Step(&protection, &data, &config, &fault, 2950U);
    assert(protection.state == BMS_PROTECTION_STATE_FAULT_ACTIVE);

    puts("bms_protection tests passed");
    return 0;
}
