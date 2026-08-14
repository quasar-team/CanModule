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
if "CI" in os.environ:
    DEVICE_ONE.vcan = True
    DEVICE_ONE.bitrate = 500000  # We need a bitrate to trigger configuration of the device, although because it is a vcan, it will be ignored

DEVICE_TWO = CanDeviceConfiguration()
DEVICE_TWO.bus_name = "vcan0"


def vcan_down() -> None:
    subprocess.run(["ip", "link", "set", "vcan0", "down"], check=True)


def vcan_up() -> None:
    subprocess.run(["ip", "link", "set", "vcan0", "up"], check=True)


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
        n_attemps * n_frames / time_elapsed, abs=0.2
    )
    assert stats.rx_per_second == pytest.approx(
        n_attemps * n_frames / time_elapsed, abs=0.2
    )


def test_socketcan_on_error_without_receiver():

    error_codes_dev1 = []

    myDevice1 = CanDevice.create(
        "socketcan",
        # Also test that it works when no receiver was provided
        CanDeviceArguments(DEVICE_ONE, None, error_codes_dev1.append),
    )

    r = myDevice1.open()
    assert r == CanReturnCode.success
    assert len(error_codes_dev1) == 0

    try:
        # Simulate disconnection problem
        vcan_down()

        sleep(2)

        assert len(error_codes_dev1) == 1
        assert error_codes_dev1[0] == CanReturnCode.disconnected
    finally:
        vcan_up()


def test_socketcan_systec_on_error():

    error_codes = []

    myDevice = CanDevice.create(
        "socketcan_systec",
        CanDeviceArguments(DEVICE_ONE, None, error_codes.append),
    )

    r = myDevice.open()
    assert r == CanReturnCode.success
    assert len(error_codes) == 0

    try:
        # Simulate disconnection problem
        vcan_down()

        sleep(2)

        assert len(error_codes) == 1
        assert error_codes[0] == CanReturnCode.disconnected
    finally:
        vcan_up()


def test_socketcan_closing_and_opening_device_on_error():

    received_frames = []
    error_codes = []
    restart_needed = False

    def restart_callback(code: CanReturnCode):
        nonlocal error_codes, restart_needed
        error_codes.append(code)
        # The restart can't be trigerred directly from the thread running the callback
        # as it touches its own device, so just flag it and handle it later
        restart_needed = True

    myDevice = CanDevice.create(
        "socketcan",
        CanDeviceArguments(DEVICE_ONE, received_frames.append, restart_callback),
    )

    r = myDevice.open()
    assert r == CanReturnCode.success
    assert len(error_codes) == 0

    # Simulate disconnection problem
    try:
        vcan_down()

        sleep(2)

        assert len(error_codes) == 1
        assert error_codes[0] == CanReturnCode.disconnected
    finally:
        vcan_up()

    assert restart_needed
    r = myDevice.close()
    assert r == CanReturnCode.success
    r = myDevice.open()
    assert r == CanReturnCode.success

    # Check if device is operational
    r = myDevice.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
    assert r == CanReturnCode.success
    assert len(error_codes) == 1  # Only the previous error


def test_socketcan_throwing_callbacks():

    receiver_callback_called = False

    def throwing_receiver_cb(_: CanFrame):
        nonlocal receiver_callback_called
        receiver_callback_called = True
        raise RuntimeError("error")

    error_callback_called = False

    def throwing_error_cb(_: CanReturnCode):
        nonlocal error_callback_called
        error_callback_called = True
        raise RuntimeError("error")

    throwing_device = CanDevice.create(
        "socketcan",
        CanDeviceArguments(DEVICE_ONE, throwing_receiver_cb, throwing_error_cb),
    )

    sender_device = CanDevice.create("socketcan", CanDeviceArguments(DEVICE_TWO))

    r = throwing_device.open()
    assert r == CanReturnCode.success
    r = sender_device.open()
    assert r == CanReturnCode.success

    # Trigger throwing_device callbacks
    try:
        r = sender_device.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
        assert r == CanReturnCode.success

        vcan_down()

        sleep(2)
    finally:
        vcan_up()

    assert receiver_callback_called
    assert error_callback_called

    # None of the devices can send because the vcan interface went down
    r = throwing_device.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
    assert r == CanReturnCode.unknown_send_error
    r = sender_device.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
    assert r == CanReturnCode.unknown_send_error

    # So restart the throwing device to check it can send again
    r = throwing_device.close()
    assert r == CanReturnCode.success
    r = throwing_device.open()
    assert r == CanReturnCode.success
    r = throwing_device.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
    assert r == CanReturnCode.success
