#include "bms_protection.h"

static BmsFaultCode DetectVoltageFault(const BmsProtection *protection,
                                       const BmsData *data,
                                       const BmsConfig *config)
{
    uint16_t cell_mv;
    uint16_t ov_recovery_mv;
    uint16_t uv_recovery_mv;

    if ((data->valid == 0U) || (config->monitored_cell_count == 0U)) {
        return BMS_FAULT_NONE;
    }

    cell_mv = data->cell_voltage_mv[0];
    ov_recovery_mv = (config->over_voltage_mv > config->voltage_hysteresis_mv)
                         ? (uint16_t)(config->over_voltage_mv
                                      - config->voltage_hysteresis_mv)
                         : 0U;
    uv_recovery_mv = (uint16_t)(config->under_voltage_mv
                                + config->voltage_hysteresis_mv);

    if ((protection->state == BMS_PROTECTION_STATE_FAULT_ACTIVE)
        || (protection->state == BMS_PROTECTION_STATE_RECOVERING)) {
        if ((protection->pending_code == BMS_FAULT_CELL_OV)
            && (cell_mv > ov_recovery_mv)) {
            return BMS_FAULT_CELL_OV;
        }
        if ((protection->pending_code == BMS_FAULT_CELL_UV)
            && (cell_mv < uv_recovery_mv)) {
            return BMS_FAULT_CELL_UV;
        }
    }

    if (cell_mv >= config->over_voltage_mv) {
        return BMS_FAULT_CELL_OV;
    }
    if (cell_mv <= config->under_voltage_mv) {
        return BMS_FAULT_CELL_UV;
    }

    return BMS_FAULT_NONE;
}

static uint32_t FaultCodeToFlag(BmsFaultCode code)
{
    if (code == BMS_FAULT_CELL_OV) {
        return BMS_FAULT_FLAG_CELL_OV;
    }
    if (code == BMS_FAULT_CELL_UV) {
        return BMS_FAULT_FLAG_CELL_UV;
    }
    return 0U;
}

static uint16_t AddElapsedMs(uint16_t elapsed_ms, uint16_t period_ms)
{
    if ((uint16_t)(UINT16_MAX - elapsed_ms) < period_ms) {
        return UINT16_MAX;
    }
    return (uint16_t)(elapsed_ms + period_ms);
}

void BmsProtection_Init(BmsProtection *protection, BmsFault *fault)
{
    protection->state = BMS_PROTECTION_STATE_NORMAL;
    protection->pending_code = BMS_FAULT_NONE;
    protection->state_elapsed_ms = 0U;

    fault->flags = 0U;
    fault->active_code = BMS_FAULT_NONE;
    fault->latched = 0U;
    fault->first_timestamp_ms = 0U;
    fault->last_timestamp_ms = 0U;
}

void BmsProtection_Update(BmsProtection *protection,
                          const BmsData *data,
                          const BmsConfig *config,
                          BmsFault *fault,
                          uint16_t period_ms)
{
    BmsFaultCode detected_code;
    uint32_t active_flag;

    /* Invalid or disabled measurements must not be interpreted as recovery. */
    if ((data->valid == 0U) || (config->monitored_cell_count == 0U)) {
        return;
    }

    detected_code = DetectVoltageFault(protection, data, config);

    switch (protection->state) {
    case BMS_PROTECTION_STATE_NORMAL:
        if (detected_code != BMS_FAULT_NONE) {
            protection->state = BMS_PROTECTION_STATE_CONFIRMING;
            protection->pending_code = detected_code;
            protection->state_elapsed_ms = period_ms;
        }
        break;

    case BMS_PROTECTION_STATE_CONFIRMING:
        if (detected_code == BMS_FAULT_NONE) {
            protection->state = BMS_PROTECTION_STATE_NORMAL;
            protection->pending_code = BMS_FAULT_NONE;
            protection->state_elapsed_ms = 0U;
        } else if (detected_code != protection->pending_code) {
            protection->pending_code = detected_code;
            protection->state_elapsed_ms = period_ms;
        } else {
            protection->state_elapsed_ms =
                AddElapsedMs(protection->state_elapsed_ms, period_ms);
        }

        if ((protection->state == BMS_PROTECTION_STATE_CONFIRMING)
            && (protection->state_elapsed_ms >= config->fault_confirm_ms)) {
            active_flag = FaultCodeToFlag(protection->pending_code);
            fault->flags |= active_flag;
            fault->active_code = protection->pending_code;
            fault->latched = 1U;
            if (fault->first_timestamp_ms == 0U) {
                fault->first_timestamp_ms = data->timestamp_ms;
            }
            fault->last_timestamp_ms = data->timestamp_ms;
            protection->state = BMS_PROTECTION_STATE_FAULT_ACTIVE;
            protection->state_elapsed_ms = 0U;
        }
        break;

    case BMS_PROTECTION_STATE_FAULT_ACTIVE:
        if (detected_code == protection->pending_code) {
            fault->last_timestamp_ms = data->timestamp_ms;
        } else if (detected_code == BMS_FAULT_NONE) {
            protection->state = BMS_PROTECTION_STATE_RECOVERING;
            protection->state_elapsed_ms = period_ms;
        } else {
            fault->flags &= ~FaultCodeToFlag(protection->pending_code);
            fault->active_code = BMS_FAULT_NONE;
            protection->state = BMS_PROTECTION_STATE_CONFIRMING;
            protection->pending_code = detected_code;
            protection->state_elapsed_ms = period_ms;
        }
        break;

    case BMS_PROTECTION_STATE_RECOVERING:
        if (detected_code == protection->pending_code) {
            protection->state = BMS_PROTECTION_STATE_FAULT_ACTIVE;
            protection->state_elapsed_ms = 0U;
        } else if (detected_code != BMS_FAULT_NONE) {
            fault->flags &= ~FaultCodeToFlag(protection->pending_code);
            fault->active_code = BMS_FAULT_NONE;
            protection->state = BMS_PROTECTION_STATE_CONFIRMING;
            protection->pending_code = detected_code;
            protection->state_elapsed_ms = period_ms;
        } else {
            protection->state_elapsed_ms =
                AddElapsedMs(protection->state_elapsed_ms, period_ms);
            if (protection->state_elapsed_ms >= config->recovery_ms) {
                fault->flags &= ~FaultCodeToFlag(protection->pending_code);
                fault->active_code = BMS_FAULT_NONE;
                fault->last_timestamp_ms = data->timestamp_ms;
                protection->state = BMS_PROTECTION_STATE_NORMAL;
                protection->pending_code = BMS_FAULT_NONE;
                protection->state_elapsed_ms = 0U;
            }
        }
        break;

    default:
        BmsProtection_Init(protection, fault);
        break;
    }
}
