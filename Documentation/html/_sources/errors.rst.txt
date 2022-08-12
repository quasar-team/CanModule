===============================
error reporting and diagnostics
===============================

for error reporting and diagnostics, we have:

boost signal error messages, delivered to a connected boost handler
-------------------------------------------------------------------
A boost signal is sent with an error message to a subscibing error handler
- anagate: used in sendMessage() and sendRemoteMessage()
- peak: sendErrorCode() sends an error code (not a message) at sendMessage(), triggerReconnectionThread() and sendRemoteRequest()
- systec: sendErrorCode() sends an error code (not a message) at message reception with USB error AND the return code of sendMessage (also if it is OK)
- sock: no wrapper method (defined but not used), error messages are sent when: socket recovers, error reading from socket, a CAN error 
  was flagged from the socket (plus message and socket timestamp if possible), when opening a CAN port produces an error
    

unified port status
-------------------
This just gives a bitpattern which is the unified port status as documented. It does NOT give any statistics. Each vendor has different 
status bits and they are all reported there.


statistics
----------
statistics on activity are recorded during runtime and can be retrieved with getStatistics(..) and related methods.
The statistics 
- counts received and sent messages per time interval. 
- txRate(), rxRate() and busLoad() 
- the time since the CAN bus was opened, sent, received.


LogIt messages
--------------
messages are reported generally and per implementation, that makes 5 LogIt components in total: CanModule, anagate, sock, peak, systec.
The TRC level will show all internal details, the recommended log levels are INF or WRN accordingly.

