#ifndef LV_CONTROLLER_H
#define LV_CONTROLLER_H

#include <stdbool.h>
#include "pn532.h"

bool readPassiveTargetID(pn532_t *p, uint8_t *uid, uint8_t *uidLength);
bool readPassiveTargetID2(pn532_t *p, uint8_t *uid, uint8_t *uidLength);
void app_main(void);

#endif