# Troubleshooting

## Bitrate setting

When you set the bitrate, CanModule will try to configure and start the socket. In the case
of Virtual SocketCAN (`vcan`), please set a dummy value for the bitrate and `vcan` to `true`.

When the bitrate is not set, CanModule assumes the socket is already configured and started.

## Non-deterministic can0/can1/... mapping with multiple Peak devices

When multiple Peak devices are connected via USB, the mapping between the USB ports and the
resulting `can0`, `can1`, etc. interfaces is not deterministic.
