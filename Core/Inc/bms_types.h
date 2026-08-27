#ifndef BMS_TYPES_H
#define BMS_TYPES_H

#include <stdint.h>

#define BMS_CELL_COUNT         4U
#define BMS_TEMPERATURE_COUNT  2U
#define BMS_ADC_CHANNEL_COUNT  (BMS_CELL_COUNT + 1U + BMS_TEMPERATURE_COUNT)

/* All physical values use fixed-point integer units.
 * Voltage: mV, current: mA, temperature: 0.01 degC.
 */
typedef struct
{
    uint16_t adc_raw[BMS_ADC_CHANNEL_COUNT];
    uint16_t cell_voltage_mv[BMS_CELL_COUNT];
    int32_t pack_current_ma;
    int16_t temperature_cdeg[BMS_TEMPERATURE_COUNT];
    uint32_t timestamp_ms;
    uint8_t valid;
    uint8_t calibrated;
} BmsData;

typedef struct
{
    uint16_t over_voltage_mv;
    uint16_t under_voltage_mv;
    int32_t over_current_ma;
    int16_t over_temperature_cdeg;
    int16_t under_temperature_cdeg;
    uint16_t fault_confirm_ms;
    uint16_t recovery_ms;
} BmsConfig;

typedef enum
{
    BMS_FAULT_NONE = 0U,
    BMS_FAULT_CELL_OV,
    BMS_FAULT_CELL_UV,
    BMS_FAULT_PACK_OC,
    BMS_FAULT_OVER_TEMPERATURE,
    BMS_FAULT_UNDER_TEMPERATURE,
    BMS_FAULT_COMM_TIMEOUT
} BmsFaultCode;

#define BMS_FAULT_FLAG_CELL_OV       (1UL << 0)
#define BMS_FAULT_FLAG_CELL_UV       (1UL << 1)
#define BMS_FAULT_FLAG_PACK_OC       (1UL << 2)
#define BMS_FAULT_FLAG_OVER_TEMP     (1UL << 3)
#define BMS_FAULT_FLAG_UNDER_TEMP    (1UL << 4)
#define BMS_FAULT_FLAG_COMM_TIMEOUT  (1UL << 5)

typedef struct
{
    uint32_t flags;
    BmsFaultCode active_code;
    uint8_t latched;
    uint32_t first_timestamp_ms;
    uint32_t last_timestamp_ms;
} BmsFault;

#endif /* BMS_TYPES_H */
