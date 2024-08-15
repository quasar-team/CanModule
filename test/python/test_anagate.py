from time import sleep
import pytest
from common import *

DEVICE_ONE = "128.141.159.236,0"
DEVICE_TWO = "128.141.159.236,1"


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
