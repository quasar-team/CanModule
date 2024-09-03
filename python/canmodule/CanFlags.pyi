"""
Namespace for CAN frame flags
"""

from __future__ import annotations

__all__ = ["ERROR_FRAME", "EXTENDED_ID", "REMOTE_REQUEST", "STANDARD_ID"]
ERROR_FRAME: int = 2
EXTENDED_ID: int = 1
REMOTE_REQUEST: int = 4
STANDARD_ID: int = 0
