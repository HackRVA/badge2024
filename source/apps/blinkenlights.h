/* 
 * File:   blinkenlights.h
 * Author: morgan
 *
 * Created on June 2, 2015, 11:12 AM
 */

#ifndef BLINKENLIGHTS_H
#define	BLINKENLIGHTS_H

#define BL_INCR_AMNT 5
void set_red(struct menu_t *m);
void set_blue(struct menu_t *m);
void set_green(struct menu_t *m);

void bl_clear_colors(struct menu_t *m);

void set_bl_mode(struct menu_t *m);
void set_bl_go(struct menu_t *m);
void set_bl_exit(struct menu_t *m);

void set_local_leds(void);
void bl_populate_menu(struct menu_t *m);

void blinkenlights_cb(struct menu_t *m);
#endif	/* BLINKENLIGHTS_H */

