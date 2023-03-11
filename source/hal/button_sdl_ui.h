#ifndef BUTTON_UI_H_
#define BUTTON_UI_H_

/* There are functions/vars in button_sim.c meant to be accessed only from the UI
 * side of the simulator.
 */

extern int time_to_quit;
extern int key_press_cb();
extern int key_release_cb();

/* This is used for showing UI inputs on the badge simulator on screen.
 * They are counters that count down to zero when a button is pressed.
 * This is so the UI has time to actually display the button presses
 * instead of the sim completely missing that a button was pressed at
 * all.
 */
struct sim_button_status {
	int button_a;
	int button_b;
	int left_rotary_button;
	int right_rotary_button;
	int dpad_up;
	int dpad_down;
	int dpad_right;
	int dpad_left;
};

struct sim_button_status get_sim_button_status(void);
void sim_button_status_countdown(void);
int sim_get_rotary_angle(int which_rotary);

#endif
