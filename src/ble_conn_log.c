// SPDX-License-Identifier: MIT
// Log BLE connection events with the HCI disconnect reason at INF level.
// ZMK itself logs the reason only at DBG, so with the usual INF console a host
// drop shows up merely as "Endpoint changed: USB". The reason tells whether the
// link died on the radio (0x08 supervision timeout, 0x22 LL timeout) or the
// other side hung up on purpose (0x13 remote user terminated, ...).

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/addr.h>

LOG_MODULE_REGISTER(ble_conn, CONFIG_ZMK_LOG_LEVEL);

// HCI error codes (Bluetooth Core Spec Vol 1, Part F), as numbers so this does
// not depend on the exact BT_HCI_ERR_* macro names of a Zephyr version.
static const char *reason_str(uint8_t reason) {
    switch (reason) {
    case 0x05: return "auth failure";
    case 0x06: return "key missing";
    case 0x08: return "supervision timeout";
    case 0x13: return "remote user terminated";
    case 0x14: return "remote low resources";
    case 0x15: return "remote power off";
    case 0x16: return "local host terminated";
    case 0x1f: return "unspecified";
    case 0x22: return "LL response timeout";
    case 0x28: return "instant passed";
    case 0x2a: return "transaction collision";
    case 0x3b: return "unacceptable conn params";
    case 0x3d: return "MIC failure";
    case 0x3e: return "failed to establish";
    default:   return "?";
    }
}

// "host" = a PC/phone we serve as peripheral; "link" = a device we connected to
// as central (the split peripheral half, or the BLE mouse).
static bool describe(struct bt_conn *conn, char *addr, size_t len, const char **kind,
                     struct bt_conn_info *info) {
    if (bt_conn_get_info(conn, info) != 0 || info->type != BT_CONN_TYPE_LE) {
        return false;
    }
    bt_addr_le_to_str(info->le.dst, addr, len);
    *kind = (info->role == BT_CONN_ROLE_PERIPHERAL) ? "host" : "link";
    return true;
}

static void on_connected(struct bt_conn *conn, uint8_t err) {
    char addr[BT_ADDR_LE_STR_LEN];
    const char *kind;
    struct bt_conn_info info;
    if (!describe(conn, addr, sizeof(addr), &kind, &info)) {
        return;
    }
    if (err) {
        LOG_INF("%s %s connect failed: 0x%02x (%s)", kind, addr, err, reason_str(err));
        return;
    }
    LOG_INF("%s %s connected (interval %u x1.25ms, latency %u, timeout %u x10ms)", kind,
            addr, info.le.interval, info.le.latency, info.le.timeout);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];
    const char *kind;
    struct bt_conn_info info;
    if (!describe(conn, addr, sizeof(addr), &kind, &info)) {
        return;
    }
    LOG_INF("%s %s disconnected: 0x%02x (%s)", kind, addr, reason, reason_str(reason));
}

static void on_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
                             uint16_t timeout) {
    char addr[BT_ADDR_LE_STR_LEN];
    const char *kind;
    struct bt_conn_info info;
    if (!describe(conn, addr, sizeof(addr), &kind, &info)) {
        return;
    }
    LOG_INF("%s %s params: interval %u x1.25ms, latency %u, timeout %u x10ms", kind, addr,
            interval, latency, timeout);
}

BT_CONN_CB_DEFINE(ble_conn_log_cb) = {
    .connected = on_connected,
    .disconnected = on_disconnected,
    .le_param_updated = on_param_updated,
};
