zephyr_library()
# The BLE-dump behavior uses ZMK app headers (drivers/behavior.h, zmk/behavior.h).
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_BLE_DUMP src/ble_dump.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_BLE_RSSI          src/ble_rssi.c)
