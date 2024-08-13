#include "CanVendorAnagate.h"

// #include <AnaGateDllCan.h>

/*

CANOpenDevice — Opens an network connection (TCP)
to an AnaGate CAN device.

Syntax
#include <AnaGateDllCan.h>
AnaInt32 CANOpenDevice(AnaInt32 *pHandle, AnaInt32 bSendDataConfirm,
AnaInt32 bSendDataInd, AnaInt32 nCANPort, const char * pcIPAddress,
AnaInt32 nTimeout);

Parameter
pHandle Pointer to a variable in which the access handle is saved in case
of a successful connection to the AnaGate device.
bSendDataConfirm If set to TRUE, all incoming and outgoing data requests
are confirmed by the internal message protocol. Without
confirmations a better transmission performance is reached.
bSendDataInd If set to FALSE, all incoming telegrams are discarded.
nCANPort CAN port number. 0 to 7
pcIPAddress Network address of the AnaGate partner.
nTimeout Default timeout for accessing the AnaGate in milliseconds.
A timeout is reported if the AnaGate partner does not respond
within the defined timeout period. This global timeout value is
valid on the current network connection for all commands and
functions which do not offer a specific timeout value.

Return value
Returns Null if successful or an error value otherwise (Appendix A, API return
codes
).
*/

// int vendor_open() {
// int CanVendorAnagate::vendor_open() {
//   AnaInt32 handle;
//   AnaInt32 bSendDataConfirm =
//       TRUE;  // Assuming we want confirmations for this example
//   AnaInt32 bSendDataInd =
//       TRUE;               // Assuming we want to receive incoming telegrams
//   AnaInt32 nCANPort = 0;  // Example CAN port number
//   const char *pcIPAddress = "192.168.1.1";  // Example IP address
//   AnaInt32 nTimeout = 1000;                 // Example timeout in
//   milliseconds

//   AnaInt32 result = CANOpenDevice(&handle, bSendDataConfirm, bSendDataInd,
//                                   nCANPort, pcIPAddress, nTimeout);
//   if (result == 0) {
//     // Save the handle or perform additional setup if needed
//     return 0;  // Success
//   } else {
//     // Handle the error accordingly
//     return result;  // Return the error code
//   }
// }
// return 0;  // Placeholder for actual implementation
// }
