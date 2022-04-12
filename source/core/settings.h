/* 
 * File:   settings.h
 * Author: morgan
 *
 * Created on June 5, 2017, 8:36 PM
 */

#ifndef SETTINGS_H
#define	SETTINGS_H

#ifdef	__cplusplus
extern "C" {
#endif

extern struct menu_t backlight_m[];
extern struct menu_t rotate_m[];
extern struct menu_t LEDlight_m[];
extern struct menu_t buzzer_m[];
extern struct menu_t screen_lock_m[];

// Use badge system data to init settings menu position.
void setup_settings_menus(void);


#ifdef	__cplusplus
}
#endif

#endif	/* SETTINGS_H */

