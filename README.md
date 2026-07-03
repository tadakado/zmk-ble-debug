# zmk-ble-debug

Two small BLE debugging helpers for [ZMK](https://zmk.dev), on the central /
non-split side:

- **BLE profile dump** — a keymap behavior (`&ble_dump`) that logs the BLE
  profile / bond list to the console.
- **Host-link RSSI logger** — an opt-in background thread that logs the RSSI of
  the host BLE connection every few seconds (useful for diagnosing weak-signal /
  interference disconnects).

Both are debug tools; enable what you need and read the output on the ZMK USB
logging console.

## Installation

Add the module to your `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: tadakado
      url-base: https://github.com/tadakado
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-ble-debug
      remote: tadakado
      revision: main
  self:
    path: config
```

## BLE profile dump (`&ble_dump`)

Declare the behavior and bind it to a key; pressing it prints the profile list.

```dts
/ {
    behaviors {
        ble_dump: ble_dump {
            compatible = "zmk,behavior-ble-dump";
            #binding-cells = <0>;
        };
    };
};
```

```dts
&ble_dump   // in your keymap
```

Output (one line per profile — address, `[active]`, `[connected]`, or `(open)`):

```
<inf> ble_dump: === BLE profiles (active=0) ===
<inf> ble_dump:   profile 0: AA:BB:CC:DD:EE:FF (public) [active] [connected]
<inf> ble_dump:   profile 1: (open)
```

It builds only when a `zmk,behavior-ble-dump` node exists, so adding the module
alone costs nothing.

## Host-link RSSI logger

Enable it in your `.conf` (off by default):

```ini
CONFIG_ZMK_BLE_RSSI=y
CONFIG_ZMK_BLE_RSSI_INTERVAL_SEC=5   # optional, default 5
```

Every interval it reads the connection RSSI of each link where this device is the
peripheral (a host such as a PC/phone — not the split other half) and logs it:

```
<inf> ble_rssi: host AA:BB:CC:DD:EE:FF (public) RSSI = -63 dBm
```

Roughly: > -60 excellent, -60..-75 good, -75..-85 marginal, below -85 weak.
`select`s `BT_CTLR_CONN_RSSI` automatically.

## Configuration

| Kconfig | Default | Description |
|---|---|---|
| `CONFIG_ZMK_BEHAVIOR_BLE_DUMP` | `y` when the node exists | The `&ble_dump` behavior. |
| `CONFIG_ZMK_BLE_RSSI` | `n` | Enable the periodic host-link RSSI logger. |
| `CONFIG_ZMK_BLE_RSSI_INTERVAL_SEC` | `5` | RSSI log interval (seconds). |

## License

MIT — see [LICENSE](LICENSE).
