/** © Copyright CERN, 2018. All rights not expressly granted are reserved.
 *
 * canmodule_wrapper cpp:
 * C-API wrapper for C++ CanModule. Solves the invocation problem
 * how to call an object(-method, C++) from a non-OO code (in C).
 * As a side effect CanModule becomes a C-interface.
 *
 * objects are allocated on the heap. Destructors can therefore be called as well.
 *
 * CanModule is preferably distributed as source (gitlab/github, mind the copyright quand meme), but this
 * more or less imposes the cmake build chain. For iseg's qmake it is cleaner to use
 * the CanModule as *.so/dll with header.
 * This is a binary distribution of CanModule which
 * is "copied for friends" ;-). These all go into cernCanModuleInterface/libs.
 */

#include <vector>
#include <map>

#include "../include/canmodule_wrapper.h"

#define CONNECTION_INDEX_MIN 0
#define CONNECTION_INDEX_MAX 15


/**
 * mutex and cond variables must be static == global in C
 * Therefore the max number of connections is hardcoded to 16.
 * ugly. sorry. come up with something better, please, which works as well ;-)
 */
boost::mutex reception_mtx0; boost::condition_variable_any reception_cond0;
boost::mutex reception_mtx1; boost::condition_variable_any reception_cond1;
boost::mutex reception_mtx2; boost::condition_variable_any reception_cond2;
boost::mutex reception_mtx3; boost::condition_variable_any reception_cond3;
boost::mutex reception_mtx4; boost::condition_variable_any reception_cond4;
boost::mutex reception_mtx5; boost::condition_variable_any reception_cond5;
boost::mutex reception_mtx6; boost::condition_variable_any reception_cond6;
boost::mutex reception_mtx7; boost::condition_variable_any reception_cond7;
boost::mutex reception_mtx8; boost::condition_variable_any reception_cond8;
boost::mutex reception_mtx9; boost::condition_variable_any reception_cond9;
boost::mutex reception_mtx10; boost::condition_variable_any reception_cond10;
boost::mutex reception_mtx11; boost::condition_variable_any reception_cond11;
boost::mutex reception_mtx12; boost::condition_variable_any reception_cond12;
boost::mutex reception_mtx13; boost::condition_variable_any reception_cond13;
boost::mutex reception_mtx14; boost::condition_variable_any reception_cond14;
boost::mutex reception_mtx15; boost::condition_variable_any reception_cond15;
std::vector<CAN_CONNECTION_t>  _connection_v;


/**
 * SYSTEC:
 * basically the baud rate can be set, default is 250000.
 *
 * Under linux you can't set this on the bridge alone. The speed has to be set
 * on the usb level for the
 * usb-can subsystem, for each can port, and both should correspond.
 *  Already you should have see the subsystems:
 * systemctl -a | grep can
 * sys-subsystem-net-devices-can0.device                                                                          loaded    active   plugged   Mutiport_CAN-to-USB
 * sys-subsystem-net-devices-can1.device                                                                          loaded    active   plugged   Mutiport_CAN-to-USB
 * sys-subsystem-net-devices-can2.device                                                                          loaded    active   plugged   Mutiport_CAN-to-USB
 * sys-subsystem-net-devices-can3.device                                                                          loaded    active   plugged   Mutiport_CAN-to-USB
 * sys-subsystem-net-devices-can4.device                                                                          loaded    active   plugged   Mutiport_CAN-to-USB
 * sys-subsystem-net-devices-can5.device                                                                          loaded    active   plugged   Mutiport_CAN-to-USB
 * sys-subsystem-net-devices-can6.device                                                                          loaded    active   plugged   Mutiport_CAN-to-USB
 * sys-subsystem-net-devices-can7.device
 *
 * Then, to set the port speed for i.e. can0:
 * sudo ip link set can0 type can bitrate 250000
 */

/**
 * ANAGATE:
 * name = bus name = 3 parameters separated by ":" line "n0:n1:n2"
 * 		n0 = "an" for anagate
 * 		n1 = port number on the module, 0=A, 1=B, etc etc
 * 		n2 = ip number
 * 		ex.: "an:1:137.138.12.99" speaks to port B (=1) on anagate module at the ip
 *
 * 	parameters = up to 6 parameters separated by whitespaces : "p0 p1 p2 p3 p4 p5" in THAT order, positive integers
 * 		see module documentation for the exact meaning http://www.anagate.de/download/Manual-AnaGateCAN-duo-en.pdf
 * 	    p0 = baud rate, 125000 or whatever the module supports
 * 	    p1 = operation mode
 * 	    p2 = termination
 * 	    p3 = high speed
 * 	    p4 = time stamp
 * 	    p5 = sync mode
 *
 * 	    advanced settings:
 * 	    	reception timer interval=500us is good. default=3000us, that does not work at first
 * 	    	attempt.
 * 	    	must reopen the connection once this is changed. Anyway, the firmware reboots and that
 * 	    	made it work. Now also works with 3000us.
 */

unsigned int canmodule_delay_default = 1000000; // 1kHz, default, works for iseg systec and anagate

void _canmodule_slowDown( int connectionIndex ){
	if ( connectionIndex >= CONNECTION_INDEX_MIN && connectionIndex <= CONNECTION_INDEX_MAX ){
		struct timespec tim, tim2;
		tim.tv_sec = 0;
		tim.tv_nsec = _connection_v[ connectionIndex ].delay;
		if(nanosleep(&tim , &tim2) < 0 ) {
			LOG(Log::ERR) << "nano sleep system call failed";
		}
	} else {
		LOG(Log::ERR) << "connectionIndex illegal, [0...15] allowed only.";
	}
}

void canmodule_setDelay_us( int connectionIndex, unsigned int dd ){
	if ( connectionIndex >= CONNECTION_INDEX_MIN && connectionIndex <= CONNECTION_INDEX_MAX ){
		_connection_v[ connectionIndex ].delay = dd;
	} else {
		LOG(Log::ERR) << "connectionIndex illegal, [0...15] allowed only.";
	}
}

/**
 * The user-handlerX gets invoked each time there is a new can message on connectionX, according to HW sync.
 * We put this message into a buffer, protect it from the wait, and notify the wait that
 * the new msg can be picked up. Using boost for sync.
 *
 * this handler must be connected to the CanModule access point by this wrapper in the canmodule_init call,
 * and this is hardcoded. We can have only static mutexes and cond_vars, therefore we have to decide BEFORE
 * compile-time how many reception threads we want. While in principle we can have a big number easily, let's
 * limit the number of connections ( PC port or ip-number && CAN port ) we can have PER TASK to 16.
 * This corresponds to 16 CAN buses, using i.e. one systec16 module or 8 anagate-duos.
 *
 * The user can conveniently pick up the new message from the waitForNewMessage call which
 * blocks until new reception. Just a separate thread or whatever sequence is needed for the
 * blocking reception call, the user can implement whatever scheme she likes.
 */
void myUserHandler0( const CanMsgStruct/*&*/ message) {
	int connectionIndex = 0;
	LOG(Log::TRC) << __FUNCTION__ << " received a message [id= " << message.c_id << " data0= " << (int) message.c_data[ 0 ]
				  << "] connectionIndex= " << connectionIndex;
	_canmodule_slowDown( connectionIndex );
	{
		boost::unique_lock<boost::mutex> lock{reception_mtx0};
		_connection_v[ connectionIndex ].reception.newArrived = true;
		_connection_v[ connectionIndex ].reception.receivedMessageBuffer = message;
	}
	LOG(Log::TRC) << __FUNCTION__ << " new message copied, notify_all connectionIndex= " << connectionIndex;
	reception_cond0.notify_all();
}
void myUserHandler1( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 1 );
	boost::unique_lock<boost::mutex> lock{reception_mtx1};
	_connection_v[ 1 ].reception.newArrived = true;
	_connection_v[ 1 ].reception.receivedMessageBuffer = message;
	reception_cond1.notify_all();
}
void myUserHandler2( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 2 );
	boost::unique_lock<boost::mutex> lock{reception_mtx2};
	_connection_v[ 2 ].reception.newArrived = true;
	_connection_v[ 2 ].reception.receivedMessageBuffer = message;
	reception_cond2.notify_all();
}
void myUserHandler3( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 3 );
	boost::unique_lock<boost::mutex> lock{reception_mtx3};
	_connection_v[ 3 ].reception.newArrived = true;
	_connection_v[ 3 ].reception.receivedMessageBuffer = message;
	reception_cond3.notify_all();
}
void myUserHandler4( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 4 );
	boost::unique_lock<boost::mutex> lock{reception_mtx4};
	_connection_v[ 4 ].reception.newArrived = true;
	_connection_v[ 4 ].reception.receivedMessageBuffer = message;
	reception_cond4.notify_all();
}
void myUserHandler5( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 5 );
	boost::unique_lock<boost::mutex> lock{reception_mtx5};
	_connection_v[ 5 ].reception.newArrived = true;
	_connection_v[ 5 ].reception.receivedMessageBuffer = message;
	reception_cond5.notify_all();
}
void myUserHandler6( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 6 );
	boost::unique_lock<boost::mutex> lock{reception_mtx6};
	_connection_v[ 6 ].reception.newArrived = true;
	_connection_v[ 6 ].reception.receivedMessageBuffer = message;
	reception_cond6.notify_all();
}
void myUserHandler7( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 7 );
	boost::unique_lock<boost::mutex> lock{reception_mtx7};
	_connection_v[ 7 ].reception.newArrived = true;
	_connection_v[ 7 ].reception.receivedMessageBuffer = message;
	reception_cond7.notify_all();
}
void myUserHandler8( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 8 );
	boost::unique_lock<boost::mutex> lock{reception_mtx8};
	_connection_v[ 8 ].reception.newArrived = true;
	_connection_v[ 8 ].reception.receivedMessageBuffer = message;
	reception_cond8.notify_all();
}
void myUserHandler9( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 9 );
	boost::unique_lock<boost::mutex> lock{reception_mtx9};
	_connection_v[ 9 ].reception.newArrived = true;
	_connection_v[ 9 ].reception.receivedMessageBuffer = message;
	reception_cond9.notify_all();
}
void myUserHandler10( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 10 );
	boost::unique_lock<boost::mutex> lock{reception_mtx10};
	_connection_v[ 10 ].reception.newArrived = true;
	_connection_v[ 10 ].reception.receivedMessageBuffer = message;
	reception_cond10.notify_all();
}
void myUserHandler11( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 11 );
	boost::unique_lock<boost::mutex> lock{reception_mtx11};
	_connection_v[ 11 ].reception.newArrived = true;
	_connection_v[ 11 ].reception.receivedMessageBuffer = message;
	reception_cond11.notify_all();
}
void myUserHandler12( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 12 );
	boost::unique_lock<boost::mutex> lock{reception_mtx12};
	_connection_v[ 12 ].reception.newArrived = true;
	_connection_v[ 12 ].reception.receivedMessageBuffer = message;
	reception_cond12.notify_all();
}
void myUserHandler13( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 13 );
	boost::unique_lock<boost::mutex> lock{reception_mtx13};
	_connection_v[ 13 ].reception.newArrived = true;
	_connection_v[ 13 ].reception.receivedMessageBuffer = message;
	reception_cond13.notify_all();
}
void myUserHandler14( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 14 );
	boost::unique_lock<boost::mutex> lock{reception_mtx14};
	_connection_v[ 14 ].reception.newArrived = true;
	_connection_v[ 14 ].reception.receivedMessageBuffer = message;
	reception_cond14.notify_all();
}
void myUserHandler15( const CanMsgStruct/*&*/ message) {
	_canmodule_slowDown( 15 );
	boost::unique_lock<boost::mutex> lock{reception_mtx15};
	_connection_v[ 15 ].reception.newArrived = true;
	_connection_v[ 15 ].reception.receivedMessageBuffer = message;
	reception_cond15.notify_all();
}

// sending a signal for fun
void canmodule_testSignalSlot( int connectionIndex ) {
	LOG(Log::TRC) << " testing slot" << connectionIndex;
	if ( connectionIndex >= CONNECTION_INDEX_MIN && connectionIndex <= CONNECTION_INDEX_MAX ){
		_connection_v[ connectionIndex ].cca->testSignalSlot();
		LOG(Log::TRC) << " slot" <<  connectionIndex << " tested OK";
	} else {
		LOG(Log::ERR) << "connectionIndex illegal, [0...15] allowed only.";
	}
}

/**
 * connect one can access point=one CAN port for sending or receiving
 * We need some parameters to set this up: canmoduleParameters
 * [0]=vendor, "an", "st"
 * [1]=can port number
 * then, according to vendor:
 * st:
 * [2]=usb port number
 * [3]=systec parameters, i.e baudrate = "250000"
 *
 * an:
 * [2]=ip number
 * [3]=anagate parameters = up to 6 integers separated by whitespaces, i.e.
 * 	for bitrate=250kbit/s, opmode=normal, terminated=true, no high speed, no time stamp, sync mode=true
 *	"250000 0 1 0 0 1";
 */
void canmodule_init( int connectionIndex, Log::LOG_LEVEL loglevel, vector<string> canmoduleParameters ) {
	if ( connectionIndex < CONNECTION_INDEX_MIN || connectionIndex > CONNECTION_INDEX_MAX ){
		LOG(Log::ERR) << "connectionIndex illegal, [0...15] allowed only.";
		return;
	}
	string parameters;
	string libname;
	string portname;
	string vendor = "an";
	string port_can = "0";
	string ip = "128.141.159.194";
	string port_usb = "0";
	bool ret = Log::initializeLogging(Log::TRC);
	if ( ret ) {
		cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt initialized OK" << endl;
	} else {
		cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " LogIt problem at initialisation" << endl;
	}
	Log::setNonComponentLogLevel( loglevel );
	LOG(Log::INF) << " VERSION " << __DATE__ << " " << __TIME__ ;
	for ( unsigned int i = 0; i < canmoduleParameters.size(); i++ ){
		LOG(Log::TRC) << " canmoduleParameters[" << i << "]= "<< canmoduleParameters[ i ];
	}
	if ( canmoduleParameters.size() < 2 ){
		LOG(Log::ERR) << "missing canmodule parameters, can't set up CAN bridge, exiting.";
		exit(-1);
	} else {
		vendor = canmoduleParameters[0];
		port_can = canmoduleParameters[1];
	}
	if ( canmoduleParameters[0] == "an" ) {
		if ( canmoduleParameters.size() < 3 ){
			LOG(Log::ERR) << "anagate missing canmodule ip parameter, can't set up CAN bridge, exiting.";
			exit(-1);
		} else {
			ip = canmoduleParameters[2];
		}
		if ( canmoduleParameters.size() < 4 ){
			parameters = "250000 0 1 0 0 1";
			// bitrate=250kbit/s, opmode=normal, terminated=true, no high speed, no time stamp, sync mode=true

			LOG(Log::WRN) << "anagate missing parameters taking defaults: " << parameters;
		} else {
			parameters = canmoduleParameters[3];
			LOG(Log::INF) << "found anagate parameters: " << parameters;
		}
		libname = "an"; // libancan.so
		portname = libname + ":" + port_can + ":" + ip;// "an:0:127.0.0.1";
		LOG(Log::INF) << "anagate portname= " << portname;
	}

	if ( canmoduleParameters[0] == "st" ) {
		if ( canmoduleParameters.size() < 3 ){
			LOG(Log::ERR) << "systec missing canmodule usb port parameter, can't set up CAN bridge, exiting.";
			exit(-1);
		} else {
			port_usb = canmoduleParameters[2];
		}
		if ( canmoduleParameters.size() < 4 ){
			parameters = "Unspecified";// or Unspecified
			LOG(Log::WRN) << "systec missing parameters taking defaults: " << parameters;
		} else {
			parameters = canmoduleParameters[3];
			LOG(Log::INF) << "found systec parameters: " << parameters;
		}
#if WIN32
		libname = "st"; // libstcan.so
#else
		libname = "sock"; // libsockcan.so
#endif
		portname = libname + ":can" + port_can;// "sock:can0" or "st:can0"
		LOG(Log::INF) << "systec portname= " << portname;
	}

	if ( connectionIndex >= _connection_v.size() ){
		CAN_CONNECTION_t ncon;
		ncon.isConnected = false;
		ncon.delay = canmodule_delay_default;
		_connection_v.push_back( ncon );
	}

	if ( _connection_v[ connectionIndex ].isConnected ) {
		LOG(Log::WRN) << "WARN: open CAN port [" << portname << "] _parameters [" << parameters << "]"
				<< " lib [" << libname << "] is already connected, skipping.";
	} else {
		_connection_v[ connectionIndex ].loader = CanModule::CanLibLoader::createInstance( libname );
		_connection_v[ connectionIndex ].cca = _connection_v[ connectionIndex ].loader->openCanBus( portname, parameters );

			if ( _connection_v[ connectionIndex ].cca != NULL ){
			bool ret = _connection_v[ connectionIndex ].cca->createBus( portname.c_str(), parameters.c_str() );
			if ( ret ) {
				LOG(Log::INF) << "open CAN port [" << portname << "] _parameters [" << parameters << "]" << " lib [" << libname << "]";
			} else {
				LOG(Log::ERR) << "ERROR: open CAN port [" << portname << "] _parameters [" << parameters << "]" << " lib [" << libname << "]";
			}
			LOG(Log::TRC) << "CAN access bus OK ";
			_connection_v[ connectionIndex ].isConnected = true;
		} else {
			LOG(Log::ERR) << " could not open CAN port, problem loading lib [" << portname << "] _parameters [" << parameters << "]" << " lib [" << libname << "]";
			exit(-1);
		}
	}
	LOG(Log::TRC) << " connecting reception slot0 to boost signal";
	_connection_v[ connectionIndex ].cca->connectReceptionSlotX( connectionIndex );
	LOG(Log::TRC) << " connecting user function to reception slot" << connectionIndex;
	switch( connectionIndex ){
	case 0:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler0 ); break; }
	case 1:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler1 ); break; }
	case 2:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler2 ); break; }
	case 3:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler3 ); break; }
	case 4:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler4 ); break; }
	case 5:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler5 ); break; }
	case 6:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler6 ); break; }
	case 7:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler7 ); break; }
	case 8:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler8 ); break; }
	case 9:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler9 ); break; }
	case 10:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler10 ); break; }
	case 11:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler11 ); break; }
	case 12:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler12 ); break; }
	case 13:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler13 ); break; }
	case 14:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler14 ); break; }
	case 15:{ _connection_v[ connectionIndex ].cca->connectFwSlotX( connectionIndex, myUserHandler15 ); break; }
	default: {
		LOG(Log::ERR) << "connectionIndex wrong, sth is seriously wrong.."; break;
	}
	}
	LOG(Log::TRC) << " connection slot" << connectionIndex << " done, seems OK";
}


/**
 * no error checking, we might throw exceptions instead. OK=false
 */
bool canmodule_send( int connectionIndex, CanMessage cm ) {
	if ( connectionIndex < CONNECTION_INDEX_MIN || connectionIndex > CONNECTION_INDEX_MAX ){
		LOG(Log::ERR) << "connectionIndex illegal, [0...15] allowed only.";
		return( false );
	}
	LOG(Log::TRC) << "sending a message [id= " << ( unsigned long int) cm.c_id
			<< " data0= " << (int) cm.c_data[ 0 ] << "]";
	// seems needed for systec, anagate
	_canmodule_slowDown( connectionIndex );
	bool ret = _connection_v[ connectionIndex ].cca->sendMessage( &cm ); // always returns true
	if ( ret ) {
		return( false ); // OK, flip logic
	}
	else {
		LOG(Log::ERR) << "sending message failed [id= " << cm.c_id << " data0= " << (int) cm.c_data[ 0 ] << "]";
		return( true );
	}
}

/**
 * reformat a linux can frame to a CanMessage (CanModule format with extra tits and tats).
 * Kind of stupid, but for cleanliness let's have it.
 *
 * struct can_frame - basic CAN frame structure
 * @can_id:  CAN ID of the frame and CAN_*_FLAG flags, see canid_t definition
 * @can_dlc: frame payload length in byte (0 .. 8) aka data length code
 *           N.B. the DLC field from ISO 11898-1 Chapter 8.4.2.3 has a 1:1
 *           mapping of the 'data length code' to the real payload length
 * @data:    CAN frame payload (up to 8 byte)
 *
 * struct can_frame {
 * 	canid_t can_id;   32 bit CAN_ID + EFF/RTR/ERR flags
 * 	__u8    can_dlc;  frame payload length in byte (0 .. CAN_MAX_DLEN)
 * 	__u8    data[CAN_MAX_DLEN] __attribute__((aligned(8)));
 * }  ;
 *
 *
 * Struct that defines a Can Message, used by CCanAccess
 *
 * typedef struct CanMsgStruct
 * {
 * 	//Identifier of the message
 * 	long c_id;
 * 	//Flags activated in the message
 * 	unsigned char c_ff;
 * 	//Length of the message
 * 	unsigned char c_dlc;
 * 	//Body of the message
 * 	unsigned char c_data[8];
 * 	//Timestamp of the message
 * 	struct timeval	c_time;
 * 	//Is the message a remote Transmission request? Must be true for remote request frames and false for data frames
 * 	bool c_rtr;
 * #ifdef __cplusplus
 * public:
 * 	CanMsgStruct() :
 * 		c_id(0),
 * 		c_ff(0),
 * 		c_dlc(0),
 * 		c_rtr(false)
 * 		{
 * 			/// todo memset, and message length > 8 for extended CAN
 * 			for (int i=0; i<8; i++)
 * 				c_data[i] = 0;
 * 		}
 *  #endif //Be careful when using this struct from C
 *
 * } CanMessage;
 */
CanMessage canmodule_reformat2CanModuleMessage( can_frame m_in ){
	CanMessage m_out;
	for ( int i = 0; i < CAN_MAX_DLEN; i++ ) {
		m_out.c_data[ i ] = m_in.data[ i ];
	}
	m_out.c_dlc = m_in.can_dlc;
	m_out.c_id = m_in.can_id;
	return( m_out );
}
can_frame canmodule_reformat2LinuxFrame( CanMessage m_in ){
	can_frame m_out;
	for ( int i = 0; i < CAN_MAX_DLEN; i++ )
		m_out.data[ i ] = m_in.c_data[ i ];
	m_out.can_dlc = m_in.c_dlc;
	m_out.can_id = m_in.c_id;
	return( m_out );
}


/**
 * message reception. Wait until the buffer is refreshed,
 * then copy the buffer and return it.
 */
CanMessage canmodule_waitForNewCanMessage( int connectionIndex ){
	CanMessage m_new;
	LOG(Log::TRC) << "waiting for new msg on connection "<< connectionIndex;
	if ( connectionIndex < CONNECTION_INDEX_MIN || connectionIndex > CONNECTION_INDEX_MAX ){
		LOG(Log::ERR) << "connectionIndex illegal, [0...15] allowed only. Return empty message immediately.";
		return( m_new );
	}
	boost::mutex *mtx;
	boost::condition_variable_any *cond;
	switch( connectionIndex ){
	case 0: { mtx = &reception_mtx0; cond = &reception_cond0; break; }
	case 1: { mtx = &reception_mtx1; cond = &reception_cond1; break; }
	case 2: { mtx = &reception_mtx2; cond = &reception_cond2; break; }
	case 3: { mtx = &reception_mtx3; cond = &reception_cond3; break; }
	case 4: { mtx = &reception_mtx4; cond = &reception_cond4; break; }
	case 5: { mtx = &reception_mtx5; cond = &reception_cond5; break; }
	case 6: { mtx = &reception_mtx6; cond = &reception_cond6; break; }
	case 7: { mtx = &reception_mtx7; cond = &reception_cond7; break; }
	case 8: { mtx = &reception_mtx8; cond = &reception_cond8; break; }
	case 9: { mtx = &reception_mtx9; cond = &reception_cond9; break; }
	case 10: { mtx = &reception_mtx10; cond = &reception_cond10; break; }
	case 11: { mtx = &reception_mtx11; cond = &reception_cond11; break; }
	case 12: { mtx = &reception_mtx12; cond = &reception_cond12; break; }
	case 13: { mtx = &reception_mtx13; cond = &reception_cond13; break; }
	case 14: { mtx = &reception_mtx14; cond = &reception_cond14; break; }
	case 15: { mtx = &reception_mtx15; cond = &reception_cond15; break; }
	default: {
		LOG(Log::ERR) << "connectionIndex wrong, sth is seriously wrong.."; break;
	}
	}
	boost::unique_lock<boost::mutex> lock{ *mtx };
	while ( !_connection_v[ connectionIndex ].reception.newArrived ){
		cond->wait( *mtx );
	}
	LOG(Log::TRC) << "new msg arrived on connection "<< connectionIndex;
	m_new = _connection_v[ connectionIndex ].reception.receivedMessageBuffer;
	_connection_v[ connectionIndex ].reception.newArrived = false;
	LOG(Log::TRC) << " returning new msg on connection " << connectionIndex;
	return( m_new );
}

void canmodule_delete( int connectionIndex ){
	if ( connectionIndex < CONNECTION_INDEX_MIN || connectionIndex > CONNECTION_INDEX_MAX ){
		LOG(Log::ERR) << "connectionIndex illegal, [0...15] allowed only. Can't delete.";
		return;
	}
	if ( _connection_v[ connectionIndex ].cca ) {
		_connection_v[ connectionIndex ].cca->disconnectReceptionSlotX();
		delete( _connection_v[ connectionIndex ].cca );
	} else {
		LOG(Log::WRN) << "this connection with connectionIndex= " << connectionIndex
				<< " is already deleted or got never init. Can't delete it.";
	}
}



