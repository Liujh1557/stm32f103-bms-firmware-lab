#ifndef BMS_PROTECTION_H
#define BMS_PROTECTION_H

#include "bms_types.h"

typedef enum
{
    BMS_PROTECTION_STATE_NORMAL = 0U,
    BMS_PROTECTION_STATE_CONFIRMING,
    BMS_PROTECTION_STATE_FAULT_ACTIVE,
    BMS_PROTECTION_STATE_RECOVERING
} BmsProtectionState;

typedef struct
{
    BmsProtectionState state;
    BmsFaultCode pending_code;
    uint16_t state_elapsed_ms;
} BmsProtection;

void BmsProtection_Init(BmsProtection *protection, BmsFault *fault);
void BmsProtection_Update(BmsProtection *protection,
                          const BmsData *data,
                          const BmsConfig *config,
                          BmsFault *fault,
                          uint16_t period_ms);

#endif /* BMS_PROTECTION_H */
