from time import sleep, time
import pytest
from common import *
import os
import subprocess
import struct
import socket

# pytestmark = pytest.mark.skipif(
#     not sys.platform.startswith("win"),
#     reason="This test module is only for Windows environments.",
# )

pytestmark = pytest.mark.skipif(
    socket.gethostname() != "pcaticswin11",
    reason="Tests currently only work when run on the pcaticswin11 development server",
)

ELMB_ID = 15


@pytest.fixture
def device_and_frames():
    config = CanDeviceConfiguration()
    config.bitrate = 125_000
    config.enable_termination = True
    config.high_speed = True
    config.bus_number = 0
    received = []
    device = CanDevice.create("systec", CanDeviceArguments(config, received.append))
    o1 = device.open()
    assert o1 == CanReturnCode.success
    received.clear()
    yield device, received
    c1 = device.close()
    assert c1 == CanReturnCode.success


def test_sync_messages_elmb(device_and_frames):
    device, received = device_and_frames
    # on new connect, all statistics reset (for now)
    diag = device.diagnostics()
    assert diag.mode == "NORMAL"
    assert diag.state == "No error."
    assert isinstance(diag.uptime, int)
    rx = diag.rx
    tx = diag.tx
    assert diag.rx_error == 0
    assert diag.tx_error == 0

    sleep(1)
    received.clear()  # hopefully clear any previously buffered frames
    r = device.send(CanFrame(0x80))  # send sync message
    assert r == CanReturnCode.success

    start = time()
    while time() - start < 15:
        if len(received) == 65:
            break
    else:
        raise RuntimeError(
            f"Did not receive expected frames before timeout: {len(received)}/65"
        )
    diff = time() - start

    diag = device.diagnostics()
    assert diag.state == "No error."
    assert diag.rx - rx == 65
    assert diag.tx - tx == 1
    assert diag.rx_per_second == pytest.approx(65 / diff, rel=0.3)
    assert diag.tx_per_second == pytest.approx(1 / diff, rel=0.3)

    digital_input_frame = received[0]
    # see page 16 https://www.nikhef.nl/pub/departments/ct/po/html/ELMB128/ELMB24.pdf
    assert digital_input_frame.id() == 0x180 + ELMB_ID
    assert ord(digital_input_frame.message()[0]) == 255
    for idx, frame in enumerate(received[1:]):
        assert frame.id() == 911
        assert ord(frame.message()[0]) == idx
