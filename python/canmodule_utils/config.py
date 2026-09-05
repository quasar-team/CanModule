import json

from canmodule import CanDeviceConfiguration

NULL_STRINGS = {"null", "none"}

# Derive from the CanDeviceConfiguration bindings
VALID_CONFIG_KEYS = frozenset(
    name
    for name in dir(CanDeviceConfiguration)
    if isinstance(getattr(CanDeviceConfiguration, name), property)
)


def normalize_config_key(key):
    return key.replace("-", "_")


def parse_config_value(key, value):
    if key not in VALID_CONFIG_KEYS:
        raise ValueError(f"Unknown configuration key '{key}'")
    if value is None:
        return None
    if isinstance(value, str) and value.strip().lower() in NULL_STRINGS:
        return None
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, (str, int)):
        return str(value)
    raise ValueError(f"unsupported value type for '{key}': {type(value).__name__}")


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
    return CanDeviceConfiguration.from_map(config)
