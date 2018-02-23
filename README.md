# CanModule
CanModule is a cross-platform library for controlling any kind of CAN device. All CAN devices are represented by a simple abstract interface (class CanModule::CCanAccess) - user code uses this interface (*only* this interface) to send messages, receive messages, etc. i.e. to interact with the CAN device as per the needs of the application. Of course, abstract interfaces require concrete implementations - these implementations are a kind of functional mapping; driving underlying CAN hardware through some lower level API in a way that satisifies the behaviour 'described' in the CCanAccess interface. Implementations for certain CAN devices are already available (see table below). Implementations for other CAN devices can be added - please submit a pull request with your implementation for review.

## Currently Available Implementations
| **Linux** | **Windows** |
| Systec (via [socketCAN](https://www.kernel.org/doc/Documentation/networking/can.txt)) | Systec (via the Systec windows API) |
| Peak (via [socketCAN](https://www.kernel.org/doc/Documentation/networking/can.txt)) | Systec (via the Peak windows API) |
| Anagate (via the Anagate linux API) | Anagate (via the Anagate windows API) |
| *Mock* implementation (simulates CAN hardware, for testing) | *Mock* implementation (simulates CAN hardware, for testing) |