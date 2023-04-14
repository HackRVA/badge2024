//
// Created by Samuel Jones on 2/21/22.
//
#include <SDL.h>
#include "button.h"
#include "rtc.h"
#include "sim_lcd_params.h"
#include "button_sdl_ui.h"
#include "button_coords.h"

#define UNUSED __attribute__((unused))

int time_to_quit = 0;
static int down_latches = 0;
static int up_latches = 0;
static int button_states = 0;
static int rotation_count[2] = { 0 };
static int rotary_angle[2] = { 0 };
static uint64_t last_change = 0;
static user_gpio_callback callback = NULL;
static int control_key_pressed = 0;

static struct sim_button_status sim_button_status = { 0 };
#define BUTTON_DISPLAY_DURATION 15 /* frames */

struct sim_button_status get_sim_button_status(void)
{
	return sim_button_status;
}

static void countdown_to_zero(int *x)
{
	if (*x > 0)
		(*x)--;
}

static void rotary_angle_delta(int which_rotary, int amount)
{
	int new_angle = rotary_angle[which_rotary] + amount * 8;
	if (new_angle < 0)
		new_angle += 128;
	if (new_angle > 127)
		new_angle -= 128;
	rotary_angle[which_rotary] = new_angle;
}

void sim_button_status_countdown(void)
{
	countdown_to_zero(&sim_button_status.button_a);
	countdown_to_zero(&sim_button_status.button_b);
	countdown_to_zero(&sim_button_status.left_rotary_button);
	countdown_to_zero(&sim_button_status.right_rotary_button);
	countdown_to_zero(&sim_button_status.dpad_up);
	countdown_to_zero(&sim_button_status.dpad_down);
	countdown_to_zero(&sim_button_status.dpad_right);
	countdown_to_zero(&sim_button_status.dpad_left);
}

static void zoom(float factor)
{
	int cx, cy, x1, y1, x2, y2;

	struct sim_lcd_params slp = get_sim_lcd_params();

	cx = slp.xoffset + slp.width / 2;
	cy = slp.yoffset + slp.height / 2;

	x1 = cx - (int) (factor * (cx - slp.xoffset));
	x2 = cx + (int) (factor * (cx - slp.xoffset));
	y1 = cy - (int) (factor * (cy - slp.yoffset));
	y2 = cy + (int) (factor * (cy - slp.yoffset));

	if (x2 - x1 < 100) /* limit zooming out */
		return;

	if (x1 < 1 || y1 < 1) { /* limit zooming in */
		set_sim_lcd_params_default();
		return;
	}
	slp.xoffset = x1;
	slp.yoffset = y1;
	slp.width = x2 - x1;
	slp.height = y2 - y1;
	set_sim_lcd_params(&slp);
}

static void zoom_in(void)
{
	zoom(1.1);
}

static void zoom_out(void)
{
	zoom(0.9);
}

static void rotate_simulator(void)
{
	struct sim_lcd_params slp = get_sim_lcd_params();
	if (slp.orientation == SIM_LCD_ORIENTATION_LANDSCAPE)
		set_sim_lcd_params_portrait();
	else
		set_sim_lcd_params_landscape();
}

static int mouse_close_enough(int x, int y, struct button_coord *b)
{
	return ((x - b->x) * (x - b->x) + (y - b->y) * (y - b->y)) < (40 * 40);
}

static int last_mouse_pressed_button = BADGE_BUTTON_MAX;

int mouse_button_down_cb(SDL_MouseButtonEvent *event, struct button_coord_list *bcl)
{
	int x, y;
	BADGE_BUTTON button = BADGE_BUTTON_MAX;

	if (event->button < 1 || event->button > 3)
		return 1;

#if 0
	/* for now, all the moust buttons do the same thing */
	switch (event->button) {
	case 0:
	case 1:
	case 2:
	default:
	}
#endif

	x = event->x;
	y = event->y;

	if (mouse_close_enough(x, y, &bcl->a_button)) {
            button = BADGE_BUTTON_A;
            sim_button_status.button_a = BUTTON_DISPLAY_DURATION;
	} else if (mouse_close_enough(x, y, &bcl->b_button)) {
            button = BADGE_BUTTON_B;
            sim_button_status.button_b = BUTTON_DISPLAY_DURATION;
	} else if (mouse_close_enough(x, y, &bcl->left_rotary)) {
            button = BADGE_BUTTON_ENCODER_2_SW;
            sim_button_status.left_rotary_button = BUTTON_DISPLAY_DURATION;
	} else if (mouse_close_enough(x, y, &bcl->right_rotary)) {
            button = BADGE_BUTTON_ENCODER_SW;
            sim_button_status.right_rotary_button = BUTTON_DISPLAY_DURATION;
	} else if (mouse_close_enough(x, y, &bcl->dpad_up)) {
                button = BADGE_BUTTON_UP;
                sim_button_status.dpad_up = BUTTON_DISPLAY_DURATION;
	} else if (mouse_close_enough(x, y, &bcl->dpad_down)) {
                button = BADGE_BUTTON_DOWN;
                sim_button_status.dpad_down = BUTTON_DISPLAY_DURATION;
	} else if (mouse_close_enough(x, y, &bcl->dpad_left)) {
                button = BADGE_BUTTON_LEFT;
                sim_button_status.dpad_left = BUTTON_DISPLAY_DURATION;
	} else if (mouse_close_enough(x, y, &bcl->dpad_right)) {
                button = BADGE_BUTTON_RIGHT;
                sim_button_status.dpad_right = BUTTON_DISPLAY_DURATION;
	}
	if (button != BADGE_BUTTON_MAX) {
                last_mouse_pressed_button = button;
		down_latches |= 1<<button;
		button_states |= 1<<button;
		if (callback) {
			callback(button, true);
		}
		last_change = rtc_get_ms_since_boot();
	}
	return 1;
}

int mouse_button_up_cb(__attribute__((unused)) SDL_MouseButtonEvent *event)
{
	if (quit_confirm_active) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		if (quit_confirmed(x, y))
			time_to_quit = 1;
		else
			quit_confirm_active = 0;
		return 1;
	}
	/* Release whichever mouse-pressed simulated button was last pressed */
	if (last_mouse_pressed_button >= 0 && last_mouse_pressed_button < BADGE_BUTTON_MAX) {
		down_latches &= ~(1 << last_mouse_pressed_button);
		button_states &= ~(1 << last_mouse_pressed_button);
		last_change = rtc_get_ms_since_boot();
		last_mouse_pressed_button = BADGE_BUTTON_MAX;
	}
	return 1;
}

int mouse_scroll_cb(SDL_MouseWheelEvent *event, struct button_coord_list *bcl)
{
	int x, y, amount;

	SDL_GetMouseState(&x, &y);
	if (event->y < 0)
		amount = 1;
	else
		amount = -1;

	if (control_key_pressed) {
		if (amount == 1)
			zoom_in();
		else
			zoom_out();
		return 1;
	}

	if (mouse_close_enough(x, y, &bcl->left_rotary)) {
		rotary_angle_delta(1, amount);
		rotation_count[1] += amount;
	} else if (mouse_close_enough(x, y, &bcl->right_rotary)) {
		rotary_angle_delta(0, amount);
		rotation_count[0] += amount;
	}
	return 1;
}

int key_press_cb(SDL_Keysym *keysym)
{
    struct sim_lcd_params slp = get_sim_lcd_params();
    BADGE_BUTTON button = BADGE_BUTTON_MAX;
    switch (keysym->sym) {
	case SDLK_r:
		rotate_simulator();
		break;
	case SDLK_EQUALS:
		zoom_in();
		break;
	case SDLK_MINUS:
		zoom_out();
		break;
	case SDLK_k:
        case SDLK_w:
        case SDLK_UP:
	    if (slp.orientation == SIM_LCD_ORIENTATION_PORTRAIT) {
                button = BADGE_BUTTON_UP;
                sim_button_status.dpad_up = BUTTON_DISPLAY_DURATION;
            } else {
                button = BADGE_BUTTON_RIGHT;
                sim_button_status.dpad_right = BUTTON_DISPLAY_DURATION;
            }
        break;
	case SDLK_j:
        case SDLK_s:
        case SDLK_DOWN:
	    if (slp.orientation == SIM_LCD_ORIENTATION_PORTRAIT) {
                button = BADGE_BUTTON_DOWN;
                sim_button_status.dpad_down = BUTTON_DISPLAY_DURATION;
            } else {
                button = BADGE_BUTTON_LEFT;
                sim_button_status.dpad_left = BUTTON_DISPLAY_DURATION;
            }
        break;
	case SDLK_h:
        case SDLK_a:
        case SDLK_LEFT:
	    if (slp.orientation == SIM_LCD_ORIENTATION_PORTRAIT) {
                button = BADGE_BUTTON_LEFT;
                sim_button_status.dpad_left = BUTTON_DISPLAY_DURATION;
            } else {
                button = BADGE_BUTTON_UP;
                sim_button_status.dpad_up = BUTTON_DISPLAY_DURATION;
            }
        break;
	case SDLK_l:
        case SDLK_d:
        case SDLK_RIGHT:
	    if (slp.orientation == SIM_LCD_ORIENTATION_PORTRAIT) {
                button = BADGE_BUTTON_RIGHT;
                sim_button_status.dpad_right = BUTTON_DISPLAY_DURATION;
            } else {
                button = BADGE_BUTTON_DOWN;
                sim_button_status.dpad_down = BUTTON_DISPLAY_DURATION;
            }
        break;
        case SDLK_SPACE:
        case SDLK_RETURN:
            if (!quit_confirm_active) {
                button = BADGE_BUTTON_A;
                sim_button_status.button_a = BUTTON_DISPLAY_DURATION;
            }
        break;
        case SDLK_b:
            button = BADGE_BUTTON_B;
            sim_button_status.button_b = BUTTON_DISPLAY_DURATION;
        break;
        case SDLK_v:
            button = BADGE_BUTTON_ENCODER_2_SW; /* left encoder switch */
            sim_button_status.left_rotary_button = BUTTON_DISPLAY_DURATION;
        break;
        case SDLK_n:
            button = BADGE_BUTTON_ENCODER_SW; /* right encoder switch */
            sim_button_status.right_rotary_button = BUTTON_DISPLAY_DURATION;
        break;
        case SDLK_q:
        case SDLK_ESCAPE:
		quit_confirm_active = !quit_confirm_active;
        break;
        case SDLK_COMMA:
        case SDLK_LESS:
            rotation_count[0] -= 1;
            rotary_angle_delta(0, -1);
        break;
        case SDLK_PERIOD:
        case SDLK_GREATER:
            rotation_count[0] += 1;
            rotary_angle_delta(0, 1);
        break;
	case SDLK_z:
            rotation_count[1] -= 1;
            rotary_angle_delta(1, -1);
        break;
	case SDLK_x:
            rotation_count[1] += 1;
            rotary_angle_delta(1, 1);
        break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		control_key_pressed = 1;
	break;
	case SDLK_F11:
		toggle_fullscreen_mode();
		break;
        default:
            break;
    }
    if (button != BADGE_BUTTON_MAX) {
        down_latches |= 1<<button;
        button_states |= 1<<button;
        if (callback) {
            callback(button, true);
        }
        last_change = rtc_get_ms_since_boot();
    }
    return 1;
}

int key_release_cb(SDL_Keysym *keysym)
{
    BADGE_BUTTON button = BADGE_BUTTON_MAX;
    switch (keysym->sym) {
        case SDLK_k:
        case SDLK_w:
        case SDLK_UP:
            button = BADGE_BUTTON_UP;
            break;
        case SDLK_j:
        case SDLK_s:
        case SDLK_DOWN:
            button = BADGE_BUTTON_DOWN;
            break;
        case SDLK_h:
        case SDLK_a:
        case SDLK_LEFT:
            button = BADGE_BUTTON_LEFT;
            break;
        case SDLK_l:
        case SDLK_d:
        case SDLK_RIGHT:
            button = BADGE_BUTTON_RIGHT;
            break;
        case SDLK_SPACE:
        case SDLK_RETURN:
            if (quit_confirm_active)
                time_to_quit = 1;
            else
                button = BADGE_BUTTON_A;
            break;
        case SDLK_q:
        case SDLK_ESCAPE:
		break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		control_key_pressed = 0;
	break;
        case SDLK_b:
            button = BADGE_BUTTON_B;
        break;
        case SDLK_n:
            button = BADGE_BUTTON_ENCODER_SW;
        break;
        case SDLK_v:
            button = BADGE_BUTTON_ENCODER_2_SW;
        break;
        default:
            break;
    }
    if (button != BADGE_BUTTON_MAX) {
        up_latches |= 1<<button;
        button_states &= ~(1<<button);
        if (callback) {
            callback(button, false);
        }
        last_change = rtc_get_ms_since_boot();
    }
    return 1;
}

int joystick_event_cb(__attribute__((unused)) SDL_Window *window, SDL_Event event)
{
	/* This is a bit janky... need to figure something better to throttle axis inputs */
	static int axistimer[10] = { 0 };

	for (int i = 0; i < 10; i++) {
		if (axistimer[i] > 0)
			axistimer[i]--;
	}
#if 0
	/* Print out what the joystick is doing. */
	switch (event.type) {
	case SDL_JOYAXISMOTION: {
			SDL_JoyAxisEvent e = event.jaxis;
			fprintf(stderr, "js:%02d axis: %02d value: %06d\n",
					e.which, e.axis, e.value);
			break;
		}
	case SDL_JOYBALLMOTION: {
			SDL_JoyBallEvent e = event.jball;
			fprintf(stderr, "JS:%02d Ball: %02d x,y: %06d,%06d\n",
				e.which, e.ball, e.xrel, e.yrel);
			break;
		}
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP: {
			SDL_JoyButtonEvent e = event.jbutton;
			fprintf(stderr, "JS:%02d Button: %02d %s\n", e.which, e.button,
				e.state == SDL_PRESSED ? "PRESSED" : "RELEASED");
			break;
		}
	case SDL_JOYHATMOTION: {
			SDL_JoyHatEvent e = event.jhat;
			fprintf(stderr, "JS:%02d Hat: %02d ", e.which, e.hat);
			switch(e.value) {
			case SDL_HAT_LEFTUP:
				fprintf(stderr, "Left-Up\n");
				break;
			case SDL_HAT_UP:
				fprintf(stderr, "Up\n");
				break;
			case SDL_HAT_RIGHTUP:
				fprintf(stderr, "Right-Up\n");
				break;
			case SDL_HAT_LEFT:
				fprintf(stderr, "Left\n");
				break;
			case SDL_HAT_CENTERED:
				fprintf(stderr, "Centered\n");
				break;
			case SDL_HAT_RIGHT:
				fprintf(stderr, "Right\n");
				break;
			case SDL_HAT_LEFTDOWN:
				fprintf(stderr, "Left-Down\n");
				break;
			case SDL_HAT_DOWN:
				fprintf(stderr, "Down\n");
				break;
			case SDL_HAT_RIGHTDOWN:
				fprintf(stderr, "Right-Down\n");
				break;
			}
		}
		break;
	}
#endif

	/* For now, just arrange thing suitably for XBox-360 controllers
	 * since that is what I have.
	 */
	BADGE_BUTTON button = BADGE_BUTTON_MAX;
	int button_pressed = 0;
	switch (event.type) {
	case SDL_JOYAXISMOTION: {
			SDL_JoyAxisEvent e = event.jaxis;
			if (e.axis == 0 && axistimer[0] == 0) {
				if (e.value < -20000) {
					axistimer[0] = 5;
					rotation_count[1] -= 1;
					rotary_angle_delta(1, -1);
				} else {
					if (e.value > 20000)  {
						axistimer[0] = 5;
						rotation_count[1] += 1;
						rotary_angle_delta(1, +1);
					}
				}
			}
			if (e.axis == 3 && axistimer[3] == 0) {
				if (e.value < -20000) {
					axistimer[3] = 5;
					rotation_count[0] -= 1;
					rotary_angle_delta(0, -1);
				} else {
					if (e.value > 20000)  {
						axistimer[3] = 5;
						rotation_count[0] += 1;
						rotary_angle_delta(0, +1);
					}
				}
			}
		}
		break;
	case SDL_JOYBUTTONUP: {
			SDL_JoyButtonEvent e = event.jbutton;
			if (e.button == 0) {
				sim_button_status.button_a = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_ENCODER_SW;
				down_latches &= ~(1 << BADGE_BUTTON_ENCODER_SW);
				button_states &= ~(1 << BADGE_BUTTON_ENCODER_SW);
				button_pressed = 1;
			} else if (e.button == 1) {
				/* TODO: fill this in */
			}
		}
		break;
	case SDL_JOYBUTTONDOWN: {
			SDL_JoyButtonEvent e = event.jbutton;
			if (e.button == 0) {
				sim_button_status.button_a = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_ENCODER_SW;
				down_latches |= (1 << BADGE_BUTTON_ENCODER_SW);
				button_states |= (1 << BADGE_BUTTON_ENCODER_SW);
				button_pressed = 1;
			} else if (e.button == 1) {
				/* TODO: fill this in */
			}
		}
		break;
	case SDL_JOYHATMOTION: {
			SDL_JoyHatEvent e = event.jhat;
			switch(e.value) {
			case SDL_HAT_LEFTUP:
				sim_button_status.dpad_up = BUTTON_DISPLAY_DURATION;
				sim_button_status.dpad_left = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_UP | BADGE_BUTTON_LEFT;
				down_latches |= (1 << BADGE_BUTTON_UP) | (1 << BADGE_BUTTON_LEFT);
				button_states |= (1 << BADGE_BUTTON_UP) | (1 << BADGE_BUTTON_LEFT);
				button_pressed = 1;
				break;
			case SDL_HAT_RIGHTUP:
				sim_button_status.dpad_up = BUTTON_DISPLAY_DURATION;
				sim_button_status.dpad_right = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_UP | BADGE_BUTTON_RIGHT;
				down_latches |= (1 << BADGE_BUTTON_UP) | (1 << BADGE_BUTTON_RIGHT);
				button_states |= (1 << BADGE_BUTTON_UP) | (1 << BADGE_BUTTON_RIGHT);
				button_pressed = 1;
				break;
			case SDL_HAT_UP:
				sim_button_status.dpad_up = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_UP;
				down_latches |= (1 << BADGE_BUTTON_UP);
				button_states |= (1 << BADGE_BUTTON_UP);
				button_pressed = 1;
				break;
			case SDL_HAT_LEFTDOWN:
				sim_button_status.dpad_down = BUTTON_DISPLAY_DURATION;
				sim_button_status.dpad_left = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_DOWN | BADGE_BUTTON_LEFT;
				down_latches |= (1 << BADGE_BUTTON_DOWN) | (1 << BADGE_BUTTON_LEFT);
				button_states |= (1 << BADGE_BUTTON_DOWN) | (1 << BADGE_BUTTON_LEFT);
				button_pressed = 1;
				break;
			case SDL_HAT_LEFT:
				sim_button_status.dpad_left = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_LEFT;
				down_latches |= (1 << BADGE_BUTTON_LEFT);
				button_states |= (1 << BADGE_BUTTON_LEFT);
				button_pressed = 1;
				break;
			case SDL_HAT_CENTERED:
				down_latches &= ~((1 << BADGE_BUTTON_LEFT) |
							(1 << BADGE_BUTTON_RIGHT) |
							(1 << BADGE_BUTTON_UP) |
							(1 << BADGE_BUTTON_DOWN));
				button_states &= ~((1 << BADGE_BUTTON_LEFT) |
							(1 << BADGE_BUTTON_RIGHT) |
							(1 << BADGE_BUTTON_UP) |
							(1 << BADGE_BUTTON_DOWN));
				button_pressed = 0;
				break;
			case SDL_HAT_RIGHT:
				sim_button_status.dpad_right = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_RIGHT;
				down_latches |= (1 << BADGE_BUTTON_RIGHT);
				button_states |= (1 << BADGE_BUTTON_RIGHT);
				button_pressed = 1;
				break;
			case SDL_HAT_RIGHTDOWN:
				sim_button_status.dpad_down = BUTTON_DISPLAY_DURATION;
				sim_button_status.dpad_right = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_DOWN | BADGE_BUTTON_RIGHT;
				down_latches |= (1 << BADGE_BUTTON_DOWN) | (1 << BADGE_BUTTON_RIGHT);
				button_states |= (1 << BADGE_BUTTON_DOWN) | (1 << BADGE_BUTTON_RIGHT);
				button_pressed = 1;
				break;
			case SDL_HAT_DOWN:
				sim_button_status.dpad_down = BUTTON_DISPLAY_DURATION;
				button |= BADGE_BUTTON_DOWN;
				down_latches |= (1 << BADGE_BUTTON_DOWN);
				button_states |= (1 << BADGE_BUTTON_DOWN);
				button_pressed = 1;
				break;
			}
			if (button_pressed) {
				if (callback) {
					callback(button, true);
				}
				last_change = rtc_get_ms_since_boot();
			}
		}
	}
	return 1;
}

void handle_window_event(SDL_Window *window, SDL_Event event)
{
	int width, height;

	switch (event.window.event) {
	case SDL_WINDOWEVENT_RESIZED:
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		SDL_GetWindowSize(window, &width, &height);
		adjust_sim_lcd_params_defaults(width, height);
		struct sim_lcd_params slp = get_sim_lcd_params();
		if (slp.orientation == SIM_LCD_ORIENTATION_LANDSCAPE)
			set_sim_lcd_params_landscape();
		else
			set_sim_lcd_params_portrait();
		break;
	default:
		break;
	}
}

void button_init_gpio(void) {
    last_change = rtc_get_ms_since_boot();
}

// Poll button state. Returns 1 if pressed, 0 if not pressed
int button_poll(BADGE_BUTTON button) {
    return button_states & (1<<button) ? 1 : 0;
}

// Get a bitmask of buttons.
int button_mask() {
    return button_states;
}

int button_down_latches(void) {
    int return_val = down_latches;
    down_latches = 0;
    return return_val;
}

int button_up_latches(void) {
    int return_val = up_latches;
    up_latches = 0;
    return return_val;
}

void clear_latches(void) {
    down_latches = 0;
    up_latches = 0;
}

typedef void (*user_gpio_callback)(BADGE_BUTTON button, bool state);
void button_set_interrupt(user_gpio_callback cb) {
    callback = cb;
}

// TODO maybe we can track this outside of the HAL and in the badge system files somewhere
unsigned int button_last_input_timestamp(void) {
    return last_change;
}

void button_reset_last_input_timestamp(void) {
    last_change = rtc_get_ms_since_boot();
}

int button_get_rotation(unsigned which_rotary) {
	if (which_rotary > (sizeof(rotation_count)/sizeof(rotation_count[0]))) {
		return 0;
	}

    int count = rotation_count[which_rotary];
    rotation_count[which_rotary] = 0;
    return count;
}

int sim_get_rotary_angle(int which_rotary)
{
	return rotary_angle[which_rotary];
}
