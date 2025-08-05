import pytest

from time import sleep, time
import platform
from common import *

DEVICE_ONE = CanDeviceConfiguration()
DEVICE_ONE.host = "ANACANFZ8-010211FC.CERN.CH"
DEVICE_ONE.bitrate = 125_000
DEVICE_ONE.enable_termination = True
DEVICE_ONE.high_speed = False
DEVICE_ONE.bus_number = 0 if platform.system() == "Linux" else 2

DEVICE_TWO = CanDeviceConfiguration()
DEVICE_TWO.host = DEVICE_ONE.host
DEVICE_TWO.bitrate = DEVICE_ONE.bitrate
DEVICE_TWO.enable_termination = False
DEVICE_TWO.high_speed = DEVICE_ONE.high_speed
DEVICE_TWO.bus_number = 1 if platform.system() == "Linux" else 3


def test_anagate_single_message():
    received_frames_dev1 = []
    received_frames_dev2 = []

    myDevice1 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
    )

    myDevice2 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_TWO, received_frames_dev2.append)
    )
    o1 = myDevice1.open()
    o2 = myDevice2.open()

    assert o1 == CanReturnCode.success
    assert o2 == CanReturnCode.success

    r = myDevice1.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
    assert r == CanReturnCode.success

    r = myDevice2.send(CanFrame(456, ["W", "o", "r", "l", "d"]))
    assert r == CanReturnCode.success

    sleep(1)

    assert len(received_frames_dev2) == 1
    assert received_frames_dev2[0].id() == 123
    assert received_frames_dev2[0].message() == ["H", "e", "l", "l", "o"]

    assert len(received_frames_dev1) == 1
    assert received_frames_dev1[0].id() == 456
    assert received_frames_dev1[0].message() == ["W", "o", "r", "l", "d"]


def test_anagate_multiple_messages():
    received_frames_dev1 = []
    received_frames_dev2 = []

    myDevice1 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
    )

    myDevice2 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_TWO, received_frames_dev2.append)
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

    r = myDevice1.send(send_frames)

    for e in r:
        assert e == CanReturnCode.success

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


def assert_diagnostics(diag, port_number=None):
    assert diag.log_entries is not None
    assert diag.name is None
    assert diag.handle is not None
    assert diag.mode == "NORMAL"
    assert diag.state is None
    assert diag.bitrate > 0
    assert len(diag.connected_clients_addresses) > 0
    assert len(diag.connected_clients_timestamps) > 0
    assert len(diag.connected_clients_ports) > 0
    assert diag.number_connected_clients > 0
    if port_number == 0:
        assert diag.temperature > 10
    else:
        assert diag.temperature is None
    assert diag.uptime > 0
    assert diag.tcp_rx > 0
    assert diag.tcp_tx >= 0
    assert diag.rx >= 0
    assert diag.tx >= 0
    assert diag.rx_error >= 0
    assert diag.tx_error >= 0
    assert diag.rx_drop >= 0
    assert diag.tx_drop >= 0
    assert diag.tx_timeout >= 0
    assert diag.bus_error is None
    assert diag.error_warning is None
    assert diag.error_passive is None
    assert diag.bus_off is None
    assert diag.arbitration_lost is None
    assert diag.restarts is None


def test_anagate_diagnostics_device_one():
    myDevice1 = CanDevice.create("anagate", CanDeviceArguments(DEVICE_ONE, None))
    myDevice1.open()
    diag = myDevice1.diagnostics()
    assert_diagnostics(diag, DEVICE_ONE.bus_number)


def test_anagate_diagnostics_device_two():
    myDevice1 = CanDevice.create("anagate", CanDeviceArguments(DEVICE_TWO, None))
    myDevice1.open()
    diag = myDevice1.diagnostics()
    assert_diagnostics(diag, DEVICE_TWO.bus_number)


def test_anagate_diagnostics():
    received_frames_dev2 = []

    myDevice1 = CanDevice.create("anagate", CanDeviceArguments(DEVICE_ONE))

    myDevice2 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_TWO, received_frames_dev2.append)
    )
    o1 = myDevice1.open()
    o2 = myDevice2.open()

    assert o1 == CanReturnCode.success
    assert o2 == CanReturnCode.success

    start_time = time()

    stats1 = myDevice1.diagnostics()
    stats2 = myDevice2.diagnostics()

    assert stats1.tx_per_second is None
    assert stats2.rx_per_second is None

    n_attemps = 3
    n_frames = 5
    for _ in range(n_attemps):
        for _ in range(n_frames):
            myDevice1.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
        sleep(1)

    stats1 = myDevice1.diagnostics()
    stats2 = myDevice2.diagnostics()

    end_time = time()

    time_elapsed = end_time - start_time

    assert stats1.tx > 5
    assert stats2.rx > 5

    assert stats1.tx_per_second == pytest.approx(
        n_attemps * n_frames / time_elapsed, abs=0.1
    )
    assert stats2.rx_per_second == pytest.approx(
        n_attemps * n_frames / time_elapsed, abs=0.1
    )


def test_anagate_bus_off_recovery():
    received_frames_dev1 = []
    myDevice1 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
    )
    myDevice1.open()

    received_frames_dev2 = []

    canDeviceConfig = CanDeviceConfiguration()
    canDeviceConfig.sent_acknowledgement = True
    canDeviceConfig.host = DEVICE_TWO.host
    canDeviceConfig.bus_number = DEVICE_TWO.bus_number
    canDeviceConfig.high_speed = DEVICE_ONE.high_speed
    canDeviceConfig.bitrate = 500_000
    canDeviceConfig.enable_termination = DEVICE_TWO.enable_termination

    myDevice2 = CanDevice.create(
        "anagate", CanDeviceArguments(canDeviceConfig, received_frames_dev2.append)
    )
    myDevice2.open()

    # Send a few frames to trigger bus off
    send_error = False
    for i in range(100_000):
        return_code = myDevice2.send(CanFrame(123, ["H", "e", "l", "l", "o"]))
        if return_code != CanReturnCode.success:
            assert i == 0
            assert return_code == CanReturnCode.not_ack
            send_error = True
            break

    assert send_error is True
