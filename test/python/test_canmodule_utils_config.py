import json
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[2]
sys.path.append(str(ROOT / "python"))
sys.path.append(str(ROOT / "build"))
sys.path.append(str(ROOT / "build" / "Debug"))
sys.path.append(str(ROOT / "build" / "Release"))


from canmodule_utils import cli, config


def write_json(tmp_path, data):
    path = tmp_path / "config.json"
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


def test_parse_set_item_accepts_uint32():
    assert config.parse_set_item("bitrate=500000") == ("bitrate", 500000)


def test_parse_set_item_accepts_bool():
    assert config.parse_set_item("vcan=true") == ("vcan", True)


def test_parse_set_item_normalizes_hyphenated_key():
    assert config.parse_set_item("enable-termination=false") == (
        "enable_termination",
        False,
    )


def test_parse_set_item_rejects_unknown_key():
    with pytest.raises(ValueError, match="Unknown configuration key 'bitrte'"):
        config.parse_set_item("bitrte=500000")


def test_parse_set_item_rejects_missing_equals():
    with pytest.raises(ValueError, match="Expected KEY=VALUE"):
        config.parse_set_item("bitrate")


def test_parse_set_item_rejects_comma_separated_values():
    with pytest.raises(ValueError, match="Use repeated --set KEY=VALUE"):
        config.parse_set_item("bitrate=500000,vcan=false")


def test_parse_bool_rejects_invalid_values():
    with pytest.raises(ValueError, match="invalid boolean value"):
        config.parse_bool("maybe")


def test_parse_uint32_rejects_negative_values():
    with pytest.raises(ValueError, match="outside 0..0xFFFFFFFF"):
        config.parse_uint32("-1")


def test_parse_uint32_accepts_hexadecimal_values():
    assert config.parse_uint32("0x7A120") == 500000


def test_load_json_config_accepts_flat_object(tmp_path):
    path = write_json(
        tmp_path,
        {
            "bus_name": "can0",
            "bitrate": 500000,
            "timeout": 100,
            "vcan": False,
        },
    )

    assert config.load_json_config(path) == {
        "bus_name": "can0",
        "bitrate": 500000,
        "timeout": 100,
        "vcan": False,
    }


def test_load_json_config_rejects_arrays(tmp_path):
    path = write_json(tmp_path, ["bus_name", "can0"])

    with pytest.raises(ValueError, match="JSON config root must be an object"):
        config.load_json_config(path)


def test_load_json_config_rejects_unknown_keys(tmp_path):
    path = write_json(tmp_path, {"bitrte": 500000})

    with pytest.raises(ValueError, match="Unknown configuration key 'bitrte'"):
        config.load_json_config(path)


def test_merge_precedence_is_json_then_set(tmp_path):
    path = write_json(
        tmp_path,
        {
            "bus_name": "can0",
            "bitrate": 500000,
            "vcan": False,
        },
    )

    merged = config.merge_config_sources(
        config_file=path,
        set_items=["bitrate=250000", "bitrate=125000", "vcan=true"],
    )

    assert merged == {
        "bus_name": "can0",
        "bitrate": 125000,
        "vcan": True,
    }


def test_null_values_remove_merged_config(tmp_path):
    path = write_json(tmp_path, {"bus_name": "can0", "bitrate": 500000})

    merged = config.merge_config_sources(config_file=path, set_items=["bitrate=null"])

    assert merged == {"bus_name": "can0"}


def test_parse_arguments_accepts_new_syntax_and_any_config_position():
    args = cli.parse_arguments(
        [
            "--config",
            "can0.json",
            "socketcan",
            "send",
            "123#AA",
            "--set=bus_name=can0",
        ]
    )

    assert args.vendor == "socketcan"
    assert args.action == "send"
    assert args.can_frame == "123#AA"
    assert args.config_file == "can0.json"
    assert args.set_items == ["bus_name=can0"]


def test_parse_arguments_rejects_legacy_socketcan_layout():
    with pytest.raises(SystemExit):
        cli.parse_arguments(["socketcan", "can0", "dump"])


def test_parse_arguments_rejects_legacy_anagate_layout():
    with pytest.raises(SystemExit):
        cli.parse_arguments(["anagate", "192.168.1.20", "0", "diag"])


def test_validate_required_config_rejects_missing_socketcan_bus_name():
    with pytest.raises(
        ValueError, match="socketcan requires configuration key 'bus_name'"
    ):
        config.validate_required_config("socketcan", {})


def test_validate_required_config_rejects_missing_anagate_fields():
    with pytest.raises(ValueError, match="anagate requires configuration keys"):
        config.validate_required_config("anagate", {"host": "192.168.1.20"})


def test_build_can_device_configuration_sets_all_supported_fields():
    configuration = config.build_can_device_configuration(
        {
            "host": "192.168.1.20",
            "bus_number": 0,
            "bitrate": 125000,
            "enable_termination": True,
            "high_speed": False,
            "timeout": 6000,
            "sent_acknowledgement": 1,
        }
    )

    assert configuration.host == "192.168.1.20"
    assert configuration.bus_number == 0
    assert configuration.bitrate == 125000
    assert configuration.enable_termination is True
    assert configuration.high_speed is False
    assert configuration.timeout == 6000
    assert configuration.sent_acknowledgement == 1
