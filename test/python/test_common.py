import pytest
from common import *


def test_frame_id():
    frame = CanFrame(1)
    assert frame.id() == 1
    assert frame.is_extended_id() is False
    assert frame.is_remote_request() is False
    assert frame.length() == 0


def test_extended_id():
    frame = CanFrame(4096)
    assert frame.id() == 4096
    assert frame.is_extended_id() is True
    assert frame.is_remote_request() is False
    assert frame.length() == 0


def test_full_frame():
    frame = CanFrame(4096, ["H", "e", "l", "l", "o"], can_flags.extended_id)
    assert frame.id() == 4096
    assert frame.is_extended_id() is True
    assert "".join(frame.message()) == "Hello"


def test_full_frame():
    with pytest.raises(ValueError) as e:
        frame = CanFrame(
            4096,
            ["H", "e", "l", "l", "o", "W", "o", "r", "l", "d"],
            can_flags.extended_id,
        )
    assert str(e.value) == "Invalid CAN frame: Message length exceeds 8 bytes."


def test_loopback_single_message():
    received_frames = []
    myDevice = CanDevice.create(
        "loopback", CanDeviceArguments(CanDeviceConfiguration(), received_frames.append)
    )
    myDevice.open()
    myDevice.send(CanFrame(1, ["H", "e", "l", "l", "o"]))
    assert len(received_frames) == 1
    assert received_frames[0].id() == 1
    assert received_frames[0].message() == ["H", "e", "l", "l", "o"]


def test_loopback_multiple_messages():
    send_frames = [
        CanFrame(1, ["H", "e", "l", "l", "o"]),
        CanFrame(2, ["W", "o", "r", "l", "d"]),
    ]
    received_frames = []
    myDevice = CanDevice.create(
        "loopback", CanDeviceArguments(CanDeviceConfiguration(), received_frames.append)
    )
    myDevice.open()
    myDevice.send(send_frames)
    assert len(received_frames) == 2
    assert received_frames[0].id() == 1
    assert received_frames[0].message() == ["H", "e", "l", "l", "o"]
    assert received_frames[1].id() == 2
    assert received_frames[1].message() == ["W", "o", "r", "l", "d"]


def test_loopback_construction_empty_callback():
    myDevice = CanDevice.create(
        "loopback", CanDeviceArguments(CanDeviceConfiguration())
    )
    myDevice.open()
