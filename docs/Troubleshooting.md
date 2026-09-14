# Troubleshooting

## Bitrate setting with vcan

Setting the bitrate on a virtual CAN (`vcan`) interface will either be ignored or result in an
error, depending on the CAN device configuration:

| `bus_name` | `bitrate` | `vcan` | Result |
| --- | --- | --- | --- |
| vcan0 | null | true | Works fine |
| vcan0 | defined | true | Works fine after running `sudo setcap cap_net_admin=ep /path/to/your/binary`  (but bitrate is ignored) |
| vcan0 | null | false | Works fine |
| vcan0 | defined | false | Results in an error while trying to set the bitrate |

## Non-deterministic can0/can1/... mapping with multiple Peak devices

When multiple Peak devices are connected via USB, the mapping between the USB ports and the
resulting `can0`, `can1`, etc. interfaces is not deterministic.
