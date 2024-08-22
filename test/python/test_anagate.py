from time import sleep
import pytest
from common import *

DEVICE_ONE = CanDeviceConfiguration()
DEVICE_ONE.host = "128.141.159.236"
DEVICE_ONE.bus_address = 0

DEVICE_TWO = CanDeviceConfiguration()
DEVICE_TWO.host = "128.141.159.236"
DEVICE_TWO.bus_address = 1


def test_anagate_single_message():
    received_frames_dev1 = []
    received_frames_dev2 = []

    myDevice1 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
    )

    myDevice2 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_TWO, received_frames_dev2.append)
    )
    myDevice1.open()
    myDevice2.open()

    myDevice1.send(CanFrame(123, ["H", "e", "l", "l", "o"]))

    sleep(0.1)

    assert len(received_frames_dev2) == 1
    assert received_frames_dev2[0].id() == 123
    assert received_frames_dev2[0].message() == ["H", "e", "l", "l", "o"]

    assert len(received_frames_dev1) == 0


def test_anagate_multiple_messages():
    received_frames_dev1 = []
    received_frames_dev2 = []

    myDevice1 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_ONE, received_frames_dev1.append)
    )

    myDevice2 = CanDevice.create(
        "anagate", CanDeviceArguments(DEVICE_TWO, received_frames_dev2.append)
    )
    myDevice1.open()
    myDevice2.open()
    send_frames = [
        CanFrame(123, ["H", "e", "l", "l", "o"]),
        CanFrame(234, ["W", "o", "r", "l", "d"]),
        CanFrame(345, 5),
        CanFrame(456, ["J", "u", "s", "t"], CanFlags.EXTENDED_ID),
        CanFrame(1 << 25, ["J", "u", "s", "t"], CanFlags.EXTENDED_ID),
    ]

    myDevice1.send(send_frames)

    sleep(0.1)

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


def test_anagate_diagnostics():
    myDevice1 = CanDevice.create("anagate", CanDeviceArguments(DEVICE_ONE, None))
    myDevice1.open()
    diag = myDevice1.diagnostics()
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
    assert diag.temperature > 10
    assert diag.uptime > 0
    assert diag.tcp_rx > 5
    assert diag.tcp_tx >= 0
    assert diag.rx >= 0
    assert diag.tx >= 5
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
