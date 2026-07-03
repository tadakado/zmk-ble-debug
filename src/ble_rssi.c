// SPDX-License-Identifier: MIT
// Periodically log the RSSI of the host BLE link(s) -- links where this device is
// the peripheral (a PC / phone, not the split other half) -- to help diagnose
// weak-signal / interference disconnects.

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_rssi, CONFIG_ZMK_LOG_LEVEL);

// Host links live on an endpoint (central or non-split). On a split peripheral
// the only link is to the central, so compile to nothing there.
#if !defined(CONFIG_ZMK_SPLIT) || defined(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/sys/byteorder.h>

static int read_conn_rssi(struct bt_conn *conn, int8_t *out) {
    uint16_t handle;
    int err = bt_hci_get_conn_handle(conn, &handle);
    if (err) {
        return err;
    }

    struct net_buf *buf =
        bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(struct bt_hci_cp_read_rssi));
    if (!buf) {
        return -ENOBUFS;
    }
    struct bt_hci_cp_read_rssi *cp = net_buf_add(buf, sizeof(*cp));
    cp->handle = sys_cpu_to_le16(handle);

    struct net_buf *rsp = NULL;
    err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
    if (err) {
        return err;
    }

    struct bt_hci_rp_read_rssi *rp = (void *)rsp->data;
    *out = rp->rssi;
    net_buf_unref(rsp);
    return 0;
}

static void log_conn_rssi(struct bt_conn *conn, void *data) {
    ARG_UNUSED(data);

    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) != 0 || info.type != BT_CONN_TYPE_LE) {
        return;
    }
    // Only the host link(s); skip the split other-half (where we are central).
    if (info.role != BT_CONN_ROLE_PERIPHERAL) {
        return;
    }

    int8_t rssi;
    if (read_conn_rssi(conn, &rssi) == 0) {
        char addr[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(info.le.dst, addr, sizeof(addr));
        LOG_INF("host %s RSSI = %d dBm", addr, rssi);
    }
}

static void ble_rssi_thread(void *a, void *b, void *c) {
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    while (1) {
        k_sleep(K_SECONDS(CONFIG_ZMK_BLE_RSSI_INTERVAL_SEC));
        bt_conn_foreach(BT_CONN_TYPE_LE, log_conn_rssi, NULL);
    }
}

K_THREAD_DEFINE(ble_rssi_tid, 1024, ble_rssi_thread, NULL, NULL, NULL,
                K_LOWEST_APPLICATION_THREAD_PRIO, 0, 2000);

#endif
