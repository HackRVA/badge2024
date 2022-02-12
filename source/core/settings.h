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

extern struct menu_t myBadgeid_m[];
extern struct menu_t peerBadgeid_m[];
extern struct menu_t backlight_m[];
extern struct menu_t rotate_m[];
extern struct menu_t LEDlight_m[];
extern struct menu_t buzzer_m[];
extern struct menu_t ping_m[];
extern struct menu_t screen_lock_m[];
void screen_save_lock_cb(struct menu_t *h);
void rotate_cb(struct menu_t *h);
void setRotate(int yes);
int getRotate(void);


#ifdef	__cplusplus
}
#endif

#endif	/* SETTINGS_H */

