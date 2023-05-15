/* 
 * File:   blinkenlights.h
 * Author: morgan
 *
 * Created on June 2, 2015, 11:12 AM
 */

#ifndef BLINKENLIGHTS_H
#define	BLINKENLIGHTS_H

#define BL_INCR_AMNT 5
void set_red(void);
void set_blue(void);
void set_green(void);

void bl_clear_colors(void);

void set_bl_mode(void);
void set_bl_go(void);
void set_bl_exit(void);

void set_local_leds(void);
void bl_populate_menu(void);

void blinkenlights_cb(struct menu_t *m);
#endif	/* BLINKENLIGHTS_H */

