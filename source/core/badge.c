#include "colors.h"
#include "assetList.h"
#include "menu.h"
#include "button.h"
#include "screensavers.h"
#include "framebuffer.h"
#include "badge.h"
#include "random.h"
#include "led_pwm.h"
#include "display.h"
#include "ir.h"
#include "rtc.h"
#include "key_value_storage.h"
#include "settings.h"
#include "uid.h"
#include "xorshift.h"
#include "mic_pdm.h"
#include "default_menu_app.h"
#include "screensaver_app.h"

/*
  inital system data, will be save/restored from flash
*/

SYSTEM_DATA G_sysData = {
	.name={"               "}, 
	.badgeId=0, 
	.sekrits={ 0 }, 
	.achievements={ 0 },
	.ledBrightness=255,
	.backlight=192,
	.mute=0
};

unsigned char  NextUSBOut=0;


SYSTEM_DATA* badge_system_data(void) {
    return &G_sysData;
}

/* UserInit is very first thing called by main() */
void UserInit(void)
{
    flash_kv_init();
    FbInit();
    FbClear();

    flash_kv_get_binary("sysdata", badge_system_data(), sizeof(SYSTEM_DATA));
    
    G_sysData.badgeId = uid_get();

    led_pwm_enable(BADGE_LED_DISPLAY_BACKLIGHT, G_sysData.backlight);
    if (G_sysData.backlight < 5) {
        G_sysData.backlight = 5;
    }
    led_pwm_set_scale(G_sysData.ledBrightness);
    display_set_rotation(G_sysData.display_rotated);
    if (G_sysData.display_inverted) {
        display_set_display_mode_inverted();
    }

    setup_settings_menus();
}


// dormant returns 1 if touch/buttons and IR messages are dormant for 60 seconds, otherwise returns 0
unsigned char dormant(void)
{
	if (mic_running())
		return 0; // Going dormant with the mic running seems to cause problems -PMW

	uint32_t timestamp = (uint32_t)rtc_get_ms_since_boot();
	if (timestamp < (button_last_input_timestamp() + 1000 * 60))
		return 0;

        if (!ir_messages_seen(false /* don't reset */ ))
            return 1;

	// wake on IR receive and reset timer
	button_reset_last_input_timestamp();
	return 0;
}


unsigned char brightScreen = 1;

unsigned char is_dormant = 0;

static struct badge_app app_stack[MAX_APP_STACK_DEPTH];
static int app_stack_idx = -1;

void pop_app(void)
{
	if (app_stack_idx > 0)
		app_stack_idx--;
	app_stack[app_stack_idx].wake_up = 1;
}

void push_app(struct badge_app app)
{
	if (app_stack_idx >= MAX_APP_STACK_DEPTH)
		return;
	app_stack_idx++;
	app_stack[app_stack_idx] = app;
	app_stack[app_stack_idx].wake_up = 1;
}

static void maybe_start_screensaver(void)
{
	if (app_stack[app_stack_idx].app_func == screensaver_cb) /* screensaver already running? */
		return;

	if (badge_system_data()->screensaver_disabled)
		return;

	if (!dormant())
		return;

	int now = rtc_get_ms_since_boot();
	if (now - button_last_input_timestamp() > 30000) { /* time to start screensaver? */
		struct badge_app app;

		app.app_func = screensaver_cb;
		app.app_context = 0;
		app.wake_up = 1;
		push_app(app); /* start screensaver */
	}
}

extern const struct menu_t main_m[];
extern void QC_cb(struct badge_app *app);
extern void rvasec_splash_cb(struct badge_app *app);
#define INITIAL_BADGE_APP rvasec_splash_cb

uint64_t ProcessIO(void) // 30 fps
{
    static const uint64_t frame_interval_us_default = 1000000/30;
    static struct default_menu_app_context menu_context;

    if (app_stack_idx == -1) { /* No apps at all yet? */

	/* Push main menu app first, then initial badge app on top of that */
	/* When the initial badge app exits, it will pop off, leaving the menu */

	init_default_menu_app_context(&menu_context, (void *) &main_m[0]);
	default_menu_app.app_context = &menu_context;
	push_app(default_menu_app);
	push_app((struct badge_app) { .app_func = INITIAL_BADGE_APP, .app_context = 0, .wake_up = 1 });
    }
    maybe_start_screensaver();
    app_stack[app_stack_idx].app_func(&app_stack[app_stack_idx]);

    return frame_interval_us_default;
}
