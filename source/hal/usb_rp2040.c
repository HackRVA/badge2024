//
// Created by Samuel Jones on 11/9/21.
//

#include "tusb.h"

bool usb_is_connected(void) {
    return tud_cdc_connected();
}