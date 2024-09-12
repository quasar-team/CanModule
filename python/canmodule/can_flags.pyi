"""
Namespace for CAN frame flags
"""
from __future__ import annotations
__all__ = ['error_frame', 'extended_id', 'remote_request', 'standard_id']
error_frame: int = 2
extended_id: int = 1
remote_request: int = 4
standard_id: int = 0
