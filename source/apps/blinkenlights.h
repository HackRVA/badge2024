/* 
 * File:   blinkenlights.h
 * Author: morgan
 *
 * Created on June 2, 2015, 11:12 AM
 */

#ifndef BLINKENLIGHTS_H
#define	BLINKENLIGHTS_H

#define BL_INCR_AMNT 5
void set_red();
void set_blue();
void set_green();

void bl_clear_colors();

void set_bl_mode();
void set_bl_go();
void set_bl_exit();

void set_local_leds();
void bl_populate_menu();

void blinkenlights_cb();
#endif	/* BLINKENLIGHTS_H */

