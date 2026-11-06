#include <dryos.h>
#include <property.h>
#include <cfn-generic.h>

// on 5D4 these are not CFn, but in main menus
GENERIC_GET_ALO
GENERIC_GET_HTP
GENERIC_GET_MLU
//GENERIC_SET_MLU

// SJE FIXME need to fill in more stuff here

static int8_t some_cfn[0x2f];
/*
PROP_HANDLER(0x80010007)
{
    ASSERT(len == 0x2f);
    memcpy(some_cfn, buf, 0x2f);
}
*/

int cfn_get_af_button_assignment() { return some_cfn[9]; }
void cfn_set_af_button(int value)
{
    some_cfn[9] = COERCE(value, 0, 2);
//    prop_request_change(0x80010007, some_cfn, 0x2f);
}


/*
 * SHOOT03-R093-BULB-OBSERVER-READONLY
 *
 * Canon EOS 5D Mark IV FW 1.3.3
 *
 * Experimental, read-only native Bulb timer observer.
 * Property: 0x80040060
 * Payload: enable, hours, minutes, seconds (4 bytes).
 *
 * No Canon property writes.
 * No blocking or drawing inside PropMgr.
 * Do not promote without hardware validation.
 */

#include "property.h"
#include "menu.h"
#include "shoot.h"
#include "lens.h"
#include "tasks.h"
#include <stdint.h>

static volatile uint32_t bulb_observer_value = 0;
static volatile uint32_t bulb_observer_seen = 0;
static volatile uint32_t bulb_observer_bad_length = 0;

/*
 * SHOOT03-R109-R3
 * Experimental staged native Canon Bulb Timer.
 * Property 0x80040060, payload 4 bytes.
 */
static volatile uint32_t bulb_r109_valid = 0;
static volatile uint32_t bulb_r109_ready = 0;
static volatile uint32_t bulb_r109_draft = 0;
static volatile uint32_t bulb_r109_dirty = 0;
static volatile uint32_t bulb_r109_pending = 0;
static volatile uint32_t bulb_r109_requested = 0;
static volatile uint32_t bulb_r109_status = 0;
static volatile uint32_t bulb_r109_count = 0;

/* Static payload remains valid during Canon processing. */
static uint8_t bulb_r109_payload[4] = {0, 0, 0, 0};

#define BULB_R109_MAX_WRITES 8

PROP_HANDLER(0x80040060)
{
    (void)token;

    if (property != 0x80040060 || !buf || len != 4)
    {
        bulb_observer_bad_length++;
        return;
    }

    const uint8_t *p = (const uint8_t *)buf;

    uint32_t packed =
          ((uint32_t)p[0])
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);


    bulb_observer_value = packed;
    bulb_observer_seen = 1;

    uint32_t enabled = packed & 0xFFu;
    uint32_t hours = (packed >> 8) & 0xFFu;
    uint32_t minutes = (packed >> 16) & 0xFFu;
    uint32_t seconds = (packed >> 24) & 0xFFu;

    if (enabled > 1 || hours > 99 ||
        minutes > 59 || seconds > 59)
    {
        bulb_r109_valid = 0;
        return;
    }

    bulb_r109_valid = 1;

    if (bulb_r109_pending)
    {
        if (packed == bulb_r109_requested)
        {
            bulb_r109_pending = 0;
            bulb_r109_status = 2;
            bulb_r109_draft = packed;
            bulb_r109_ready = 1;
            bulb_r109_dirty = 0;
        }
    }
    else if (!bulb_r109_dirty)
    {
        bulb_r109_draft = packed;
        bulb_r109_ready = 1;
    }
}

static MENU_UPDATE_FUNC(bulb_observer_update)
{
    if (!bulb_observer_seen)
    {
        if (bulb_observer_bad_length)
            MENU_SET_VALUE("Invalid length");
        else
            MENU_SET_VALUE("Awaiting Canon");

        return;
    }

    uint32_t value = bulb_observer_value;

    int enabled = (value      ) & 0xFF;
    int hours   = (value >>  8) & 0xFF;
    int minutes = (value >> 16) & 0xFF;
    int seconds = (value >> 24) & 0xFF;

    if (enabled > 1 || minutes > 59 || seconds > 59)
    {
        MENU_SET_VALUE("Invalid Canon data");
        return;
    }

    MENU_SET_VALUE(
        "%s %02d:%02d:%02d",
        enabled ? "ON" : "OFF",
        hours, minutes, seconds
    );
}


/* Local editing never writes a Canon property. */
static void bulb_r109_edit(int field, int delta)
{
    if (field < 0 || field > 3 ||
        !bulb_observer_seen ||
        !bulb_r109_valid ||
        !bulb_r109_ready ||
        bulb_observer_bad_length ||
        bulb_r109_pending ||
        delta == 0)
        return;

    uint32_t value = bulb_r109_draft;
    int shift = field * 8;
    int current = (value >> shift) & 0xFF;

    int maximum = field == 0 ? 1 :
                  field == 1 ? 99 : 59;

    int next;

    if (field == 0)
        next = current ? 0 : 1;
    else
        next = current + (delta > 0 ? 1 : -1);

    if (next < 0)
        next = 0;

    if (next > maximum)
        next = maximum;

    uint32_t mask = 0xFFu << shift;

    bulb_r109_draft = (value & ~mask) |
                      ((uint32_t)next << shift);

    bulb_r109_dirty =
        bulb_r109_draft != bulb_observer_value;

    bulb_r109_status = 0;
}

static MENU_SELECT_FUNC(bulb_r109_enabled_select)
{
    (void)priv;
    bulb_r109_edit(0, delta);
}

static MENU_SELECT_FUNC(bulb_r109_hours_select)
{
    (void)priv;
    bulb_r109_edit(1, delta);
}

static MENU_SELECT_FUNC(bulb_r109_minutes_select)
{
    (void)priv;
    bulb_r109_edit(2, delta);
}

static MENU_SELECT_FUNC(bulb_r109_seconds_select)
{
    (void)priv;
    bulb_r109_edit(3, delta);
}

static MENU_UPDATE_FUNC(bulb_r109_enabled_update)
{
    if (!bulb_r109_ready)
    {
        MENU_SET_VALUE("Awaiting Canon");
        return;
    }

    MENU_SET_VALUE(
        "%s%s",
        (bulb_r109_draft & 0xFFu) ? "ON" : "OFF",
        bulb_r109_dirty ? " *" : ""
    );
}

static MENU_UPDATE_FUNC(bulb_r109_hours_update)
{
    MENU_SET_VALUE(
        "%02d",
        (int)((bulb_r109_draft >> 8) & 0xFFu)
    );
}

static MENU_UPDATE_FUNC(bulb_r109_minutes_update)
{
    MENU_SET_VALUE(
        "%02d",
        (int)((bulb_r109_draft >> 16) & 0xFFu)
    );
}

static MENU_UPDATE_FUNC(bulb_r109_seconds_update)
{
    MENU_SET_VALUE(
        "%02d",
        (int)((bulb_r109_draft >> 24) & 0xFFu)
    );
}

/* Q on Apply is the only place that sends a Canon write. */
static MENU_SELECT_FUNC(bulb_r109_apply_Q)
{
    (void)priv;
    (void)delta;

    if (!bulb_observer_seen ||
        !bulb_r109_valid ||
        !bulb_r109_ready ||
        bulb_observer_bad_length ||
        !bulb_r109_dirty ||
        bulb_r109_pending ||
        bulb_r109_count >= BULB_R109_MAX_WRITES)
        return;

    uint32_t request = bulb_r109_draft;

    uint32_t enabled = request & 0xFFu;
    uint32_t hours = (request >> 8) & 0xFFu;
    uint32_t minutes = (request >> 16) & 0xFFu;
    uint32_t seconds = (request >> 24) & 0xFFu;

    if (enabled > 1 || hours > 99 ||
        minutes > 59 || seconds > 59)
        return;

    if (enabled && !hours && !minutes && !seconds)
        return;

    if (request == bulb_observer_value)
    {
        bulb_r109_dirty = 0;
        return;
    }

    bulb_r109_payload[0] = (uint8_t)enabled;
    bulb_r109_payload[1] = (uint8_t)hours;
    bulb_r109_payload[2] = (uint8_t)minutes;
    bulb_r109_payload[3] = (uint8_t)seconds;

    bulb_r109_requested = request;
    bulb_r109_status = 1;
    bulb_r109_pending = 1;
    bulb_r109_count++;

    prop_request_change(
        0x80040060,
        bulb_r109_payload,
        sizeof(bulb_r109_payload)
    );
}

static MENU_UPDATE_FUNC(bulb_r109_apply_update)
{
    if (bulb_r109_pending)
    {
        MENU_SET_VALUE("Awaiting Canon");
        return;
    }

    if (bulb_r109_status == 2)
    {
        MENU_SET_VALUE("Canon confirmed");
        return;
    }

    if (!bulb_observer_seen || !bulb_r109_ready)
    {
        MENU_SET_VALUE("Awaiting Canon");
        return;
    }

    if (!bulb_r109_valid || bulb_observer_bad_length)
    {
        MENU_SET_VALUE("Invalid Canon data");
        return;
    }

    if (bulb_r109_count >= BULB_R109_MAX_WRITES)
    {
        MENU_SET_VALUE("Limit reached");
        return;
    }

    if (!bulb_r109_dirty)
    {
        MENU_SET_VALUE("No changes");
        return;
    }

    uint32_t request = bulb_r109_draft;

    if ((request & 0xFFu) &&
        (request & 0xFFFFFF00u) == 0)
    {
        MENU_SET_VALUE("Set duration > 0");
        return;
    }

    MENU_SET_VALUE("Press Q to apply");
}

static MENU_SELECT_FUNC(bulb_r109_discard_Q)
{
    (void)priv;
    (void)delta;

    if (!bulb_observer_seen ||
        !bulb_r109_valid ||
        bulb_r109_pending)
        return;

    bulb_r109_draft = bulb_observer_value;
    bulb_r109_ready = 1;
    bulb_r109_dirty = 0;
    bulb_r109_status = 0;
}

static MENU_UPDATE_FUNC(bulb_r109_discard_update)
{
    if (bulb_r109_pending)
        MENU_SET_VALUE("Write pending");
    else
        MENU_SET_VALUE(
            bulb_r109_dirty ? "Press Q to reset" : "No draft"
        );
}


/* SHOOT03-R115-A: local duration editing only. */
static int bulb_r115_step = 0; /* 0=1s, 1=1m, 2=1h */

static MENU_SELECT_FUNC(bulb_r115_duration_select)
{
    (void)priv;

    if (!delta ||
        !bulb_observer_seen ||
        !bulb_r109_valid ||
        !bulb_r109_ready ||
        bulb_observer_bad_length ||
        bulb_r109_pending)
        return;

    uint32_t current = bulb_r109_draft;

    int h = (current >> 8) & 0xFF;
    int m = (current >> 16) & 0xFF;
    int s = (current >> 24) & 0xFF;

    int total = h * 3600 + m * 60 + s;

    int step = bulb_r115_step == 0 ? 1 :
               bulb_r115_step == 1 ? 60 : 3600;

    int next = total + (delta > 0 ? step : -step);
    int minimum = (current & 0xFFu) ? 1 : 0;

    if (next < minimum)
        next = minimum;

    if (next > 359999)
        next = 359999;

    int nh = next / 3600;
    int nm = (next / 60) % 60;
    int ns = next % 60;

    bulb_r109_draft =
        (current & 0xFFu) |
        ((uint32_t)nh << 8) |
        ((uint32_t)nm << 16) |
        ((uint32_t)ns << 24);

    bulb_r109_dirty =
        (bulb_r109_draft != bulb_observer_value);

    bulb_r109_status = 0;
}

/* Q on duration changes step; it never writes Canon. */
static MENU_SELECT_FUNC(bulb_r115_duration_step_Q)
{
    (void)priv;
    (void)delta;

    if (bulb_r109_pending)
        return;

    bulb_r115_step = (bulb_r115_step + 1) % 3;
}

static MENU_UPDATE_FUNC(bulb_r115_duration_update)
{
    (void)entry;

    if (!bulb_observer_seen || !bulb_r109_ready)
    {
        MENU_SET_VALUE("Awaiting Canon");
        return;
    }

    uint32_t v = bulb_r109_draft;

    MENU_SET_VALUE(
        "%02d:%02d:%02d %s%s",
        (int)((v >> 8) & 0xFFu),
        (int)((v >> 16) & 0xFFu),
        (int)((v >> 24) & 0xFFu),
        bulb_r115_step == 0 ? "1s" :
        bulb_r115_step == 1 ? "1m" : "1h",
        bulb_r109_dirty ? "*" : ""
    );
}


/*
 * SHOOT03-R116-B
 * Experimental one-shot, Canon native Bulb Timer.
 *
 * A separate task watches the menu transition.
 * No second exposure timer. Property edits use the existing writer.
 *
 * First hardware validation restricted to 1-10 sec,
 * B mode, Single drive, manual AF disabled.
 */

#define BULB_R116_IDLE       0
#define BULB_R116_ARMED      1
#define BULB_R116_COUNTDOWN  2
#define BULB_R116_FIRING     3
#define BULB_R116_CANCELED   4
#define BULB_R116_TRIGGERED  5

static volatile int bulb_r116_state = BULB_R116_IDLE;
static int bulb_r116_delay = 2;
static int bulb_r116_delay_snapshot = 2;
static uint32_t bulb_r116_expected = 0;

static int bulb_r116_ready(void)
{
    if (!bulb_observer_seen ||
        !bulb_r109_valid ||
        !bulb_r109_ready ||
        bulb_observer_bad_length ||
        bulb_r109_dirty ||
        bulb_r109_pending)
        return 0;

    uint32_t v = bulb_observer_value;

    if ((v & 0xFFu) != 1u)
        return 0;

    int h = (v >> 8) & 0xFF;
    int m = (v >> 16) & 0xFF;
    int s = (v >> 24) & 0xFF;
    int total = h * 3600 + m * 60 + s;

    /* Experimental short-exposure guard. */
    if (total < 1 || total > 60)
        return 0;

    if (shooting_mode != SHOOTMODE_BULB)
        return 0;

    if (is_movie_mode() ||
        drive_mode != DRIVE_SINGLE ||
        is_intervalometer_running() ||
        get_halfshutter_pressed() ||
        !job_state_ready_to_take_pic() ||
        ml_shutdown_requested)
        return 0;

    return 1;
}

static MENU_SELECT_FUNC(bulb_r116_delay_select)
{
    (void)priv;

    if (bulb_r116_state == BULB_R116_ARMED ||
        bulb_r116_state == BULB_R116_COUNTDOWN)
        return;

    if (delta > 0 && bulb_r116_delay < 10)
        bulb_r116_delay++;
    else if (delta < 0 && bulb_r116_delay > 0)
        bulb_r116_delay--;
}

static MENU_UPDATE_FUNC(bulb_r116_delay_update)
{
    MENU_SET_VALUE("%d sec", bulb_r116_delay);
}

/* Q arms once. Q again cancels. */
static MENU_SELECT_FUNC(bulb_r116_arm_Q)
{
    (void)priv;
    (void)delta;

    if (bulb_r116_state == BULB_R116_ARMED ||
        bulb_r116_state == BULB_R116_COUNTDOWN)
    {
        bulb_r116_state = BULB_R116_CANCELED;
        return;
    }

    if (!gui_menu_shown() || !bulb_r116_ready())
    {
        bulb_r116_state = BULB_R116_CANCELED;
        return;
    }

    bulb_r116_expected = bulb_observer_value;
    bulb_r116_delay_snapshot = bulb_r116_delay;
    bulb_r116_state = BULB_R116_ARMED;
}

static MENU_UPDATE_FUNC(bulb_r116_arm_update)
{
    switch (bulb_r116_state)
    {
        case BULB_R116_ARMED:
            MENU_SET_VALUE("ARMED - leave menu");
            break;
        case BULB_R116_COUNTDOWN:
            MENU_SET_VALUE("Countdown");
            break;
        case BULB_R116_FIRING:
            MENU_SET_VALUE("Triggering");
            break;
        case BULB_R116_TRIGGERED:
            MENU_SET_VALUE("Triggered once");
            break;
        case BULB_R116_CANCELED:
            MENU_SET_VALUE("Canceled / blocked");
            break;
        default:
            MENU_SET_VALUE("Press Q to arm");
            break;
    }

    if (!bulb_r116_ready() &&
        bulb_r116_state != BULB_R116_ARMED &&
        bulb_r116_state != BULB_R116_COUNTDOWN)
    {
        MENU_SET_WARNING(
            MENU_WARN_ADVICE,
            "Requires confirmed Canon Bulb 1-60s, B and Single drive."
        );
    }
}

static int bulb_r116_still_safe(void)
{
    return bulb_r116_state == BULB_R116_COUNTDOWN &&
           !gui_menu_shown() &&
           bulb_r116_expected == bulb_observer_value &&
           bulb_r116_ready();
}

/*
 * R120-B: coalesce local edits into a single
 * asynchronous Canon property request.
 *
 * Called only by the existing bulb_r116_task.
 * No Canon property write occurs in menu redraw.
 *
 * Keep the experimental limit of 8 writes.
 */
static void bulb_r120_auto_commit_tick(void)
{
    static uint32_t last_draft = 0;
    static int stable_ticks = 0;

    if (bulb_r116_state == BULB_R116_ARMED ||
        bulb_r116_state == BULB_R116_COUNTDOWN ||
        bulb_r116_state == BULB_R116_FIRING)
    {
        stable_ticks = 0;
        return;
    }

    if (!bulb_observer_seen ||
        !bulb_r109_valid ||
        !bulb_r109_ready ||
        bulb_observer_bad_length ||
        bulb_r109_pending ||
        !bulb_r109_dirty ||
        bulb_r109_count >= BULB_R109_MAX_WRITES)
    {
        stable_ticks = 0;
        return;
    }

    uint32_t draft = bulb_r109_draft;

    if (draft != last_draft)
    {
        last_draft = draft;
        stable_ticks = 0;
        return;
    }

    /* R121-A: five stable 100 ms polling intervals. */
    if (++stable_ticks >= 5)
    {
        stable_ticks = 0;

        /*
         * Reuse the already hardware-tested writer.
         * It performs input validation and sets
         * pending before issuing the Canon request.
         */
        bulb_r109_apply_Q(0, 0);
    }
}

static void bulb_r116_task(void *unused)
{
    (void)unused;

    while (!ml_shutdown_requested)
    {
        if (bulb_r116_state != BULB_R116_ARMED)
        {
            bulb_r120_auto_commit_tick();
            msleep(100);
            continue;
        }

        /* Q was pressed inside the ML menu.
         * Wait until the menu is fully closed. */
        while (!ml_shutdown_requested &&
               bulb_r116_state == BULB_R116_ARMED &&
               gui_menu_shown())
        {
            msleep(100);
        }

        if (ml_shutdown_requested ||
            bulb_r116_state != BULB_R116_ARMED)
            continue;

        bulb_r116_state = BULB_R116_COUNTDOWN;

        /*
         * R121-A: 500 ms of post-menu settling, followed
         * by the user-configured start delay.
         * Poll every 100 ms to allow cancellation.
         */
        int ticks = 5 + bulb_r116_delay_snapshot * 10;

        for (int i = 0; i < ticks; i++)
        {
            if (!bulb_r116_still_safe())
            {
                bulb_r116_state = BULB_R116_CANCELED;
                break;
            }

            msleep(100);
        }

        if (!bulb_r116_still_safe())
        {
            if (bulb_r116_state == BULB_R116_COUNTDOWN)
                bulb_r116_state = BULB_R116_CANCELED;
            continue;
        }

        /* Disarm BEFORE issuing the shutter request.
         * Never retry automatically. */
        bulb_r116_state = BULB_R116_FIRING;

        /*
         * Same single-picture path underlying
         * the intervalometer. On this port,
         * CONFIG_BULB is not enabled.
         * Canon remains responsible for exposure.
         */
        take_a_pic(AF_DISABLE);

        bulb_r116_state = BULB_R116_TRIGGERED;
    }
}

TASK_CREATE(
    "bulb_r116_oneshot",
    bulb_r116_task,
    0,
    0x1e,
    0x1400
);

/*
 * R120-B parent entry.
 * Q arms once only after Canon has confirmed
 * the current ON/OFF and exposure duration.
 */
static MENU_SELECT_FUNC(bulb_r120_arm_Q)
{
    if (bulb_r116_state == BULB_R116_ARMED ||
        bulb_r116_state == BULB_R116_COUNTDOWN)
    {
        bulb_r116_arm_Q(priv, delta);
        return;
    }

    if (bulb_r109_dirty || bulb_r109_pending)
        return;

    bulb_r116_arm_Q(priv, delta);
}

static MENU_UPDATE_FUNC(bulb_r120_parent_update)
{
    (void)entry;

    if (bulb_r116_state == BULB_R116_ARMED)
    {
        MENU_SET_VALUE("ARMED - leave ML");
        return;
    }

    if (bulb_r116_state == BULB_R116_COUNTDOWN)
    {
        MENU_SET_VALUE("Starting...");
        return;
    }

    if (bulb_r109_pending)
    {
        MENU_SET_VALUE("Waiting for Canon");
        return;
    }

    if (bulb_r109_dirty)
    {
        if (bulb_r109_count >= BULB_R109_MAX_WRITES)
            MENU_SET_VALUE("Write limit reached");
        else
            MENU_SET_VALUE("Saving settings...");
        return;
    }

    if (!bulb_observer_seen || !bulb_r109_ready)
    {
        MENU_SET_VALUE("Awaiting Canon");
        return;
    }

    uint32_t v = bulb_observer_value;

    MENU_SET_VALUE(
        "%s %02d:%02d:%02d",
        (v & 0xFFu) ? "ON" : "OFF",
        (int)((v >> 8) & 0xFFu),
        (int)((v >> 16) & 0xFFu),
        (int)((v >> 24) & 0xFFu)
    );

    if (!bulb_r116_ready())
    {
        MENU_SET_WARNING(
            MENU_WARN_ADVICE,
            "Auto shot: Canon Bulb 1-60s, B, Single drive."
        );
    }
}

static struct menu_entry bulb_observer_menu[] =
{
    {
        .name = "Canon Bulb Timer",
        .select = menu_open_submenu,
        .select_Q = bulb_r120_arm_Q,
        .update = bulb_r120_parent_update,
        .depends_on = DEP_PHOTO_MODE,
        .submenu_width = 710,
        .help = "SET: settings. Q: arm one automatic shot.",
        .help2 = "Canon must confirm settings before Q can arm.",
        .children = (struct menu_entry[]) {
            {
                .name = "Bulb Timer",
                .select = bulb_r109_enabled_select,
                .update = bulb_r109_enabled_update,
                .help = "Enable or disable Canon native Bulb Timer."
            },
            {
                .name = "Duration",
                .select = bulb_r115_duration_select,
                .select_Q = bulb_r115_duration_step_Q,
                .update = bulb_r115_duration_update,
                .icon_type = IT_ACTION,
                .help = "Wheel: duration. Q: 1s / 1m / 1h step."
            },
            {
                .name = "Delay",
                .select = bulb_r116_delay_select,
                .update = bulb_r116_delay_update,
                .icon_type = IT_ACTION,
                .help = "Auto-shot delay after closing ML: 0-10s."
            },
            MENU_EOL
        },
    },
};

static void bulb_observer_init(void *unused)
{
    (void)unused;

    menu_add(
        "Shoot",
        bulb_observer_menu,
        COUNT(bulb_observer_menu)
    );
}

INIT_FUNC(__FILE__, bulb_observer_init);

/* END SHOOT03-R093-BULB-OBSERVER-READONLY */
