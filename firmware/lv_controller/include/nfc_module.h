#ifndef NFC_MODULE_H
#define NFC_MODULE_H

#include "pn532.h"

// NFC reader global pointer
extern pn532_t *nfc_reader;

void init_nfc_reader(void);
void read_single_nfc_tag(void *);

#endif