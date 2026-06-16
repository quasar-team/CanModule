import json

from canmodule import CanDeviceConfiguration


UINT32_MAX = 0xFFFFFFFF
NULL_STRINGS = {"null", "none"}
TRUE_STRINGS = {"true", "1", "yes", "on", "enable", "enabled"}
FALSE_STRINGS = {"false", "0", "no", "off", "disable", "disabled"}


def normalize_config_key(key):
    return key.replace("-", "_")


def parse_string(value):
    if value is None:
        return None
    if not isinstance(value, str):
        raise ValueError("expected a string")
    return value


def parse_bool(value):
    if value is None:
        return None
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        normalized = value.strip().lower()
        if normalized in NULL_STRINGS:
            return None
        if normalized in TRUE_STRINGS:
            return True
        if normalized in FALSE_STRINGS:
            return False
    raise ValueError(f"invalid boolean value '{value}'")


def parse_uint32(value):
    if value is None:
        return None
    if isinstance(value, bool):
        raise ValueError("expected an unsigned integer")

    try:
        if isinstance(value, int):
            parsed = value
        elif isinstance(value, str):
            normalized = value.strip().lower()
            if normalized in NULL_STRINGS:
                return None
            parsed = int(normalized, 0)
        else:
            raise ValueError
    except ValueError as error:
        raise ValueError(f"invalid unsigned integer value '{value}'") from error

    if parsed < 0 or parsed > UINT32_MAX:
        raise ValueError(f"unsigned integer value '{value}' is outside 0..0xFFFFFFFF")
    return parsed


def parse_sent_acknowledgement(value):
    if value is None:
        return None
    if isinstance(value, str) and value.strip().lower() in NULL_STRINGS:
        return None
    if isinstance(value, bool):
        return 1 if value else 0
    if isinstance(value, str) and value.strip().lower() in TRUE_STRINGS | FALSE_STRINGS:
        return 1 if parse_bool(value) else 0

    parsed = parse_uint32(value)
    if parsed not in (0, 1):
        raise ValueError("sent_acknowledgement must be 0, 1, true, or false")
    return parsed


CONFIG_FIELDS = {
    "bus_name": parse_string,
    "bus_number": parse_uint32,
    "host": parse_string,
    "bitrate": parse_uint32,
    "enable_termination": parse_bool,
    "high_speed": parse_bool,
    "timeout": parse_uint32,
    "vcan": parse_bool,
    "sent_acknowledgement": parse_sent_acknowledgement,
}


def parse_config_value(key, value):
    normalized_key = normalize_config_key(key)
    if normalized_key not in CONFIG_FIELDS:
        raise ValueError(f"Unknown configuration key '{key}'")
    try:
        return CONFIG_FIELDS[normalized_key](value)
    except ValueError as error:
        raise ValueError(f"Invalid value for '{normalized_key}': {error}") from error


def parse_set_item(item):
    if "=" not in item:
        raise ValueError(f"Invalid --set value '{item}'. Expected KEY=VALUE.")
    if "," in item:
        raise ValueError(
            "Use repeated --set KEY=VALUE arguments instead of comma-separated values."
        )

    key, value = item.split("=", 1)
    if not key:
        raise ValueError("Invalid --set value. Configuration key cannot be empty.")

    normalized_key = normalize_config_key(key)
    return normalized_key, parse_config_value(normalized_key, value)


def load_json_config(path):
    try:
        with open(path, "r", encoding="utf-8") as config_file:
            data = json.load(config_file)
    except json.JSONDecodeError as error:
        raise ValueError(f"Invalid JSON in '{path}': {error}") from error
    except OSError as error:
        raise ValueError(f"Unable to read config file '{path}': {error}") from error

    if not isinstance(data, dict):
        raise ValueError("JSON config root must be an object")

    config = {}
    for key, value in data.items():
        normalized_key = normalize_config_key(key)
        config[normalized_key] = parse_config_value(normalized_key, value)
    return config


def merge_config_sources(config_file=None, set_items=None):
    merged = {}

    if config_file:
        merged.update(load_json_config(config_file))

    for item in set_items or []:
        key, value = parse_set_item(item)
        merged[key] = value

    return {key: value for key, value in merged.items() if value is not None}


def validate_required_config(vendor, config):
    if vendor == "socketcan" and "bus_name" not in config:
        raise ValueError(
            "socketcan requires configuration key 'bus_name'. "
            "Use --set bus_name=can0 or --config FILE."
        )

    missing_anagate = [key for key in ("host", "bus_number") if key not in config]
    if vendor == "anagate" and missing_anagate:
        raise ValueError(
            "anagate requires configuration keys 'host' and 'bus_number'. "
            "Use --set host=... --set bus_number=... or --config FILE."
        )


def build_can_device_configuration(config):
    configuration = CanDeviceConfiguration()
    for key, value in config.items():
        setattr(configuration, key, value)
    return configuration
