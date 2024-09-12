from __future__ import annotations
import typing
from . import can_flags

__all__ = [
    "CanDevice",
    "CanDeviceArguments",
    "CanDeviceConfiguration",
    "CanDiagnostics",
    "CanFrame",
    "CanReturnCode",
    "can_flags",
    "disconnected",
    "internal_api_error",
    "invalid_bitrate",
    "lost_arbitration",
    "not_ack",
    "rx_error",
    "socket_error",
    "success",
    "timeout",
    "too_many_connections",
    "tx_buffer_overflow",
    "tx_error",
    "unknown_close_error",
    "unknown_open_error",
    "unknown_send_error",
]

class CanDevice:
    @staticmethod
    def create(arg0: str, arg1: CanDeviceArguments) -> CanDevice: ...
    def args(self) -> CanDeviceArguments: ...
    def close(self) -> CanReturnCode: ...
    def diagnostics(self) -> CanDiagnostics: ...
    def open(self) -> CanReturnCode: ...
    @typing.overload
    def send(self, arg0: CanFrame) -> CanReturnCode: ...
    @typing.overload
    def send(self, arg0: list[CanFrame]) -> list[CanReturnCode]: ...

class CanDeviceArguments:
    def __init__(
        self,
        config: CanDeviceConfiguration,
        receiver: typing.Callable[[CanFrame], None],
    ) -> None: ...
    @property
    def config(self) -> CanDeviceConfiguration: ...

class CanDeviceConfiguration:
    bitrate: int | None
    bus_name: str | None
    bus_number: int | None
    enable_termination: bool | None
    host: str | None
    sent_acknowledgement: int | None
    timeout: int | None
    vcan: bool | None
    def __init__(self) -> None: ...
    def __str__(self) -> str: ...

class CanDiagnostics:
    def __init__(self) -> None: ...
    def __str__(self) -> str: ...
    @property
    def arbitration_lost(self) -> int | None: ...
    @property
    def bitrate(self) -> int | None: ...
    @property
    def bus_error(self) -> int | None: ...
    @property
    def bus_off(self) -> int | None: ...
    @property
    def connected_clients_addresses(self) -> list[str] | None: ...
    @property
    def connected_clients_ports(self) -> list[int] | None: ...
    @property
    def connected_clients_timestamps(self) -> list[str] | None: ...
    @property
    def error_passive(self) -> int | None: ...
    @property
    def error_warning(self) -> int | None: ...
    @property
    def handle(self) -> int | None: ...
    @property
    def log_entries(self) -> list[str] | None: ...
    @property
    def mode(self) -> str | None: ...
    @property
    def name(self) -> str | None: ...
    @property
    def number_connected_clients(self) -> int | None: ...
    @property
    def restarts(self) -> int | None: ...
    @property
    def rx(self) -> int | None: ...
    @property
    def rx_drop(self) -> int | None: ...
    @property
    def rx_error(self) -> int | None: ...
    @property
    def state(self) -> str | None: ...
    @property
    def tcp_rx(self) -> int | None: ...
    @property
    def tcp_tx(self) -> int | None: ...
    @property
    def temperature(self) -> float | None: ...
    @property
    def tx(self) -> int | None: ...
    @property
    def tx_drop(self) -> int | None: ...
    @property
    def tx_error(self) -> int | None: ...
    @property
    def tx_timeout(self) -> int | None: ...
    @property
    def uptime(self) -> int | None: ...

class CanFrame:
    @typing.overload
    def __init__(self, id: int, requested_length: int, flags: int) -> None: ...
    @typing.overload
    def __init__(self, id: int, requested_length: int) -> None: ...
    @typing.overload
    def __init__(self, id: int) -> None: ...
    @typing.overload
    def __init__(self, id: int, message: list[str]) -> None: ...
    @typing.overload
    def __init__(self, id: int, message: list[str], flags: int) -> None: ...
    def __str__(self) -> str: ...
    def flags(self) -> int: ...
    def id(self) -> int: ...
    def is_error(self) -> bool: ...
    def is_extended_id(self) -> bool: ...
    def is_remote_request(self) -> bool: ...
    def is_standard_id(self) -> bool: ...
    def length(self) -> int: ...
    def message(self) -> list[str]: ...

class CanReturnCode:
    """
    Members:

      success

      unknown_open_error

      socket_error

      too_many_connections

      timeout

      disconnected

      internal_api_error

      unknown_send_error

      not_ack

      tx_error

      rx_error

      tx_buffer_overflow

      lost_arbitration

      invalid_bitrate

      unknown_close_error
    """

    __members__: typing.ClassVar[
        dict[str, CanReturnCode]
    ]  # value = {'success': <CanReturnCode.success: 0>, 'unknown_open_error': <CanReturnCode.unknown_open_error: 1>, 'socket_error': <CanReturnCode.socket_error: 2>, 'too_many_connections': <CanReturnCode.too_many_connections: 3>, 'timeout': <CanReturnCode.timeout: 4>, 'disconnected': <CanReturnCode.disconnected: 5>, 'internal_api_error': <CanReturnCode.internal_api_error: 6>, 'unknown_send_error': <CanReturnCode.unknown_send_error: 7>, 'not_ack': <CanReturnCode.not_ack: 8>, 'tx_error': <CanReturnCode.tx_error: 10>, 'rx_error': <CanReturnCode.rx_error: 9>, 'tx_buffer_overflow': <CanReturnCode.tx_buffer_overflow: 11>, 'lost_arbitration': <CanReturnCode.lost_arbitration: 12>, 'invalid_bitrate': <CanReturnCode.invalid_bitrate: 13>, 'unknown_close_error': <CanReturnCode.unknown_close_error: 14>}
    disconnected: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.disconnected: 5>
    internal_api_error: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.internal_api_error: 6>
    invalid_bitrate: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.invalid_bitrate: 13>
    lost_arbitration: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.lost_arbitration: 12>
    not_ack: typing.ClassVar[CanReturnCode]  # value = <CanReturnCode.not_ack: 8>
    rx_error: typing.ClassVar[CanReturnCode]  # value = <CanReturnCode.rx_error: 9>
    socket_error: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.socket_error: 2>
    success: typing.ClassVar[CanReturnCode]  # value = <CanReturnCode.success: 0>
    timeout: typing.ClassVar[CanReturnCode]  # value = <CanReturnCode.timeout: 4>
    too_many_connections: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.too_many_connections: 3>
    tx_buffer_overflow: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.tx_buffer_overflow: 11>
    tx_error: typing.ClassVar[CanReturnCode]  # value = <CanReturnCode.tx_error: 10>
    unknown_close_error: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.unknown_close_error: 14>
    unknown_open_error: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.unknown_open_error: 1>
    unknown_send_error: typing.ClassVar[
        CanReturnCode
    ]  # value = <CanReturnCode.unknown_send_error: 7>
    def __eq__(self, other: typing.Any) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __init__(self, value: int) -> None: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __setstate__(self, state: int) -> None: ...
    def __str__(self) -> str: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

disconnected: CanReturnCode  # value = <CanReturnCode.disconnected: 5>
internal_api_error: CanReturnCode  # value = <CanReturnCode.internal_api_error: 6>
invalid_bitrate: CanReturnCode  # value = <CanReturnCode.invalid_bitrate: 13>
lost_arbitration: CanReturnCode  # value = <CanReturnCode.lost_arbitration: 12>
not_ack: CanReturnCode  # value = <CanReturnCode.not_ack: 8>
rx_error: CanReturnCode  # value = <CanReturnCode.rx_error: 9>
socket_error: CanReturnCode  # value = <CanReturnCode.socket_error: 2>
success: CanReturnCode  # value = <CanReturnCode.success: 0>
timeout: CanReturnCode  # value = <CanReturnCode.timeout: 4>
too_many_connections: CanReturnCode  # value = <CanReturnCode.too_many_connections: 3>
tx_buffer_overflow: CanReturnCode  # value = <CanReturnCode.tx_buffer_overflow: 11>
tx_error: CanReturnCode  # value = <CanReturnCode.tx_error: 10>
unknown_close_error: CanReturnCode  # value = <CanReturnCode.unknown_close_error: 14>
unknown_open_error: CanReturnCode  # value = <CanReturnCode.unknown_open_error: 1>
unknown_send_error: CanReturnCode  # value = <CanReturnCode.unknown_send_error: 7>
