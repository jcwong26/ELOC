#ifndef NFC_MODULE_H
#define NFC_MODULE_H

#include "pn532.h"

// NFC reader global pointer
extern pn532_t *nfc_reader;

int init_nfc_reader(void);
void read_single_nfc_tag(void *);
void nfc_state_machine(uint8_t uid[], uint8_t uidLength);
int new_tag(uint8_t uid[], uint8_t uidLength);
int check_tag(uint8_t uid[], uint8_t uidLength);

#endif