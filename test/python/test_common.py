import pytest
from common import *


def test_frame_id():
    frame = CanFrame(1)
    assert frame.is_valid() is True
    assert frame.id() == 1
    assert frame.is_extended_id() is False
    assert frame.is_remote_request() is True


def test_extended_id():
    frame = CanFrame(4096)
    assert frame.is_valid() is True
    assert frame.id() == 4096
    assert frame.is_extended_id() is True
    assert frame.is_remote_request() is True


def test_full_frame():
    frame = CanFrame(4096, ["H", "e", "l", "l", "o"], CanFlags.EXTENDED_ID)
    assert frame.is_valid() is True
    assert frame.id() == 4096
    assert frame.is_extended_id() is True
    assert "".join(frame.message()) == "Hello"


def test_full_frame():
    frame = CanFrame(
        4096, ["H", "e", "l", "l", "o", "W", "o", "r", "l", "d"], CanFlags.EXTENDED_ID
    )
    assert frame.is_valid() is False
