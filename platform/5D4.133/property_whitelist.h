#ifndef _property_whitelist_h_
#define _property_whitelist_h_

#include "property.h"

// By default, we allow "reads" (property_handler, ML hooking) for any property.
//
// Initial support for Digic 6, 7, 8 cams found some prop handlers triggered
// bad behaviour (crashes, hangs).  These are blacklisted for both read and write
// until investigated and fixed.
//
// For Digic 4 and 5 cams, "writes" (prop_request_change()) are controlled
// by a global flag, CONFIG_PROP_REQUEST_CHANGE.  This is all or nothing.
//
// For D678, we disallow all "writes" via the same flag, so,
// CONFIG_PROP_REQUEST_CHANGE should be #undef for new ports in features.h.
// Writing to props can brick cams if mistakes are made.
//
// Additionally, even when defined, we now have a per property check (on D678),
// so if prop_write_allow[] is empty, no writes will be attempted even
// with CONFIG_PROP_REQUEST_CHANGE.  This allows devs to enable properties
// one at a time after checking correctness via reversing, tests, etc.

// DO NOT put the same property in both lists;
// that would allow writes but not reads, which is untested
// (and unnecessary as far as I know).

// deny reads / do not register property handlers for these
const uint32_t prop_handler_deny[] =
{
    PROP_LV_AFFRAME, // ML asserts
};

// allow writes / allow prop_request_change() for these:
const uint32_t prop_write_allow [] =
{
    /* EXPO01A: White Balance / Kelvin only */
    PROP_WB_MODE_LV,
    PROP_WB_KELVIN_LV,
    PROP_WB_MODE_PH,
    PROP_WB_KELVIN_PH,

    /* EXPO02B: validated WBShift G/M candidate */
    PROP_WBS_GM,
    /* EXPO02C: WBShift B/A candidate */
    PROP_WBS_BA,

    /* EXPO03A: custom WB gains candidate */
    PROP_CUSTOM_WB,

    /* ISO */
    PROP_ISO,

    /* Shutter */
    PROP_SHUTTER,

    /* Aperture */
    PROP_APERTURE,

    /* Picture Style selection and editable settings */
    PROP_PICTURE_STYLE,
    PROP_PICSTYLE_SETTINGS_STANDARD,
    PROP_PICSTYLE_SETTINGS_PORTRAIT,
    PROP_PICSTYLE_SETTINGS_LANDSCAPE,
    PROP_PICSTYLE_SETTINGS_FINEDETAIL,
    PROP_PICSTYLE_SETTINGS_NEUTRAL,
    PROP_PICSTYLE_SETTINGS_FAITHFUL,
    PROP_PICSTYLE_SETTINGS_MONOCHROME,
    PROP_PICSTYLE_SETTINGS_USERDEF1,
    PROP_PICSTYLE_SETTINGS_USERDEF2,
    PROP_PICSTYLE_SETTINGS_USERDEF3,
};

// anything not listed above will allow reads but not writes

#endif
