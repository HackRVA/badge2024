#ifndef BUTTON_UI_H_
#define BUTTON_UI_H_

/* There are functions/vars in button_sim.c meant to be accessed only from the UI
 * side of the simulator.
 */

extern int time_to_quit;
extern int key_press_cb();
extern int key_release_cb();

#endif
