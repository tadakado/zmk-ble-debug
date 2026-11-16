// SPDX-License-Identifier: MIT
// Keymap behavior that logs the BLE profile / bond list to the console.

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

// BLE host profiles only exist on an endpoint (central or non-split); on a
// split peripheral the profile API isn't available, so compile to a no-op.
#if !defined(CONFIG_ZMK_SPLIT) || defined(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#define DUMP_ENABLED 1
#else
#define DUMP_ENABLED 0
#endif

#if DUMP_ENABLED
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zephyr/bluetooth/addr.h>
#if IS_ENABLED(CONFIG_ZMK_USB)
#include <zmk/usb.h>
#endif
#endif

LOG_MODULE_REGISTER(ble_dump, CONFIG_ZMK_LOG_LEVEL);

static void ble_dump_profiles(void) {
#if DUMP_ENABLED
    LOG_INF("=== BLE profiles (active=%d) ===", zmk_ble_active_profile_index());
    for (uint8_t i = 0; i < ZMK_BLE_PROFILE_COUNT; i++) {
        if (zmk_ble_profile_is_open(i)) {
            LOG_INF("  profile %d: (open)", i);
        } else {
            char s[BT_ADDR_LE_STR_LEN];
            bt_addr_le_to_str(zmk_ble_profile_address(i), s, sizeof(s));
            LOG_INF("  profile %d: %s%s%s", i, s,
                    (i == zmk_ble_active_profile_index()) ? " [active]" : "",
                    zmk_ble_profile_is_connected(i) ? " [connected]" : "");
        }
    }
#else
    LOG_INF("ble_dump: peripheral has no host profiles");
#endif
}

#if DUMP_ENABLED
static const char *transport_str(enum zmk_transport t) {
    switch (t) {
    case ZMK_TRANSPORT_USB:
        return "USB";
    case ZMK_TRANSPORT_BLE:
        return "BLE";
    default:
        return "none";
    }
}
#endif

// Where keystrokes go right now. "selected" is the endpoint reports are sent to:
// the preferred transport if it is ready, otherwise the other one as a fallback.
static void dump_endpoint(void) {
#if DUMP_ENABLED
    char sel[ZMK_ENDPOINT_STR_LEN];
    struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();

    if (selected.transport == ZMK_TRANSPORT_NONE) {
        snprintf(sel, sizeof(sel), "none");
    } else {
        zmk_endpoint_instance_to_str(selected, sel, sizeof(sel));
    }
    LOG_INF("=== Output: sending to %s (preferred %s) ===", sel,
            transport_str(zmk_endpoint_get_preferred_transport()));
#if IS_ENABLED(CONFIG_ZMK_USB)
    LOG_INF("  USB: %s", zmk_usb_is_hid_ready() ? "HID ready"
                         : zmk_usb_is_powered() ? "powered, not ready"
                                                : "not connected");
#endif
    LOG_INF("  BLE: active profile %d %s", zmk_ble_active_profile_index(),
            zmk_ble_active_profile_is_connected() ? "connected" : "not connected");
#endif
}

static int on_pressed(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    ble_dump_profiles();
    dump_endpoint();
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_released(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_ble_dump_api = {
    .binding_pressed = on_pressed,
    .binding_released = on_released,
};

static int behavior_ble_dump_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

#define DT_DRV_COMPAT zmk_behavior_ble_dump

BEHAVIOR_DT_INST_DEFINE(0, behavior_ble_dump_init, NULL, NULL, NULL,
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_ble_dump_api);
