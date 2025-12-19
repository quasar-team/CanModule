from time import sleep, time
import pytest
from common import *
import os
import subprocess

pytestmark = pytest.mark.skipif(
    sys.platform.startswith("win"),
    reason="This test module is only for Linux environments.",
)

DEVICE_ONE = CanDeviceConfiguration()
DEVICE_ONE.bus_name = "vcan0"

# If we are inside the pipeline, we have admin rights to create virtual CAN interfaces
# and therefore can bring up/down the device, useful for testing the callback on error functionality
if "CI" in os.environ and not sys.platform.startswith("win"):
    subprocess.run(["modprobe", "vcan"], check=False)
    DEVICE_ONE.vcan = True
    DEVICE_ONE.bitrate = 500000  # We need a bitrate to trigger configuration of the device, although because it is a vcan, it will be ignored

DEVICE_TWO = CanDeviceConfiguration()
DEVICE_TWO.bus_name = "vcan0"


def test_socketcan_single_message():
    received_frames_dev1 = []
    received_frames_dev2 = []

    myDevice1 = CanDevice.create(
        "socketcan", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
    )
    myDevice2 = CanDevice.create(
        "socketcan", CanDeviceArguments(DEVICE_TWO, received_frames_dev2.append)
    )
    r = myDevice1.open()
    assert r == CanReturnCode.success

    r = myDevice2.open()
    assert r == CanReturnCode.success

    r = myDevice1.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
    assert r == CanReturnCode.success

    sleep(1)

    assert len(received_frames_dev2) == 1
    assert received_frames_dev2[0].id() == 123
    assert received_frames_dev2[0].message() == ["H", "e", "l", "l", "o"]

    assert len(received_frames_dev1) == 0


def test_socketcan_multiple_messages():
    received_frames_dev1 = []
    received_frames_dev2 = []

    myDevice1 = CanDevice.create(
        "socketcan", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
    )
    myDevice2 = CanDevice.create(
        "socketcan", CanDeviceArguments(DEVICE_TWO, received_frames_dev2.append)
    )

    r = myDevice1.open()
    assert r == CanReturnCode.success
    r = myDevice2.open()
    assert r == CanReturnCode.success

    send_frames = [
        CanFrame(123, ["H", "e", "l", "l", "o"]),
        CanFrame(234, ["W", "o", "r", "l", "d"]),
        CanFrame(345, 5),
        CanFrame(456, ["J", "u", "s", "t"], can_flags.extended_id),
        CanFrame(1 << 25, ["J", "u", "s", "t"], can_flags.extended_id),
    ]

    rl = myDevice1.send(send_frames)
    for r in rl:
        assert r == CanReturnCode.success

    sleep(1)

    assert len(received_frames_dev2) == 5

    assert received_frames_dev2[0].id() == 123
    assert received_frames_dev2[0].message() == ["H", "e", "l", "l", "o"]
    assert received_frames_dev2[0].is_extended_id() is False
    assert received_frames_dev2[0].is_remote_request() is False

    assert received_frames_dev2[1].id() == 234
    assert received_frames_dev2[1].message() == ["W", "o", "r", "l", "d"]
    assert received_frames_dev2[1].is_extended_id() is False
    assert received_frames_dev2[1].is_remote_request() is False

    assert received_frames_dev2[2].id() == 345
    assert received_frames_dev2[2].message() == []
    assert received_frames_dev2[2].length() == 5
    assert received_frames_dev2[2].is_extended_id() is False
    assert received_frames_dev2[2].is_remote_request() is True

    assert received_frames_dev2[3].id() == 456
    assert received_frames_dev2[3].message() == ["J", "u", "s", "t"]
    assert received_frames_dev2[3].is_extended_id() is True
    assert received_frames_dev2[3].is_remote_request() is False

    assert received_frames_dev2[4].id() == 1 << 25
    assert received_frames_dev2[4].message() == ["J", "u", "s", "t"]
    assert received_frames_dev2[4].is_extended_id() is True
    assert received_frames_dev2[4].is_remote_request() is False
    assert received_frames_dev2[4].is_error() is False


def test_socketcan_diagnostics():
    received_frames_dev1 = []

    myDevice1 = CanDevice.create(
        "socketcan", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
    )

    start_time = time()
    r = myDevice1.open()
    assert r == CanReturnCode.success

    stats = myDevice1.diagnostics()

    assert stats.tx_per_second is None
    assert stats.rx_per_second is None

    n_attemps = 3
    n_frames = 5
    for _ in range(n_attemps):
        for _ in range(n_frames):
            myDevice1.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
        sleep(1)

    stats = myDevice1.diagnostics()
    end_time = time()

    time_elapsed = end_time - start_time

    assert stats.tx > 5
    assert stats.rx > 5

    assert stats.tx_per_second == pytest.approx(
        n_attemps * n_frames / time_elapsed, abs=0.1
    )
    assert stats.rx_per_second == pytest.approx(
        n_attemps * n_frames / time_elapsed, abs=0.1
    )
