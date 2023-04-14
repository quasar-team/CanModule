#include <CCanAccess.h>
#include <CanLibLoader.h>
using namespace std;

namespace CanModule
{

/* static */ GlobalErrorSignaler *GlobalErrorSignaler::instancePtr = NULL;
/* static */ LogItInstance * GlobalErrorSignaler::m_st_logIt = NULL;
/* static */ Log::LogComponentHandle GlobalErrorSignaler::m_st_lh = 0;

/* static */ LogItInstance* CCanAccess::st_logItRemoteInstance = NULL;


GlobalErrorSignaler::~GlobalErrorSignaler(){
	globalErrorSignal.disconnect_all_slots();
	LOG( Log::TRC, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " disconnected all handlers from signal.";
}


/* static */ void GlobalErrorSignaler::initializeLogIt(LogItInstance *remoteInstance) {
	if ( remoteInstance != NULL ){
		bool ret = Log::initializeDllLogging( remoteInstance );
		if ( ret ) {
			Log::LogComponentHandle lh = 0;
			remoteInstance->getComponentHandle( CanModule::LogItComponentName, lh );
			LOG(Log::INF, lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << "created singleton instance of GlobalErrorSignaler with LogIt Dll initialized. Nice.";

			GlobalErrorSignaler::m_st_logIt = remoteInstance;
			GlobalErrorSignaler::m_st_lh = lh;
		} else {
			std::cout << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " remote logIt instance Dll init failed" << std::endl;
		}
	} else {
		std::cout << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " remote logIt instance is NULL" << std::endl;
	}
}

/**
 * singleton fabricator. We have one global signal only which is neither lib/vendor nor port specific, per task.
 */
GlobalErrorSignaler* GlobalErrorSignaler::getInstance() {
	std::cout <<  __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << std::endl;
	if ( GlobalErrorSignaler::instancePtr == NULL) {
		GlobalErrorSignaler::instancePtr = new GlobalErrorSignaler();
	}
	return GlobalErrorSignaler::instancePtr;
}


/**
 * connect/disconnect the handler provided by "the user": void myHandler( int code, const char *myMessage )
 *  connect/disconnect a given handler to the signal of the singleton. function pointer is the argument.
 *  Can have as many connected handlers as you want, but they will all be disconnected when the object goes out of scope (dtor).
 */
bool GlobalErrorSignaler::connectHandler( void (*fcnPtr)( int, const char*, timeval ) ){
	if ( fcnPtr != NULL ){
		globalErrorSignal.connect( fcnPtr );
		LOG( Log::INF, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__
				<< " connect handler to signal (" << globalErrorSignal.num_slots() << " connected handlers)";
		return true;
	} else {
		LOG( Log::ERR, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " cannot connect NULL handler to signal. skipping...";
		return false;
	}
}
bool GlobalErrorSignaler::disconnectHandler( void (*fcnPtr)( int, const char*, timeval ) ){
	if ( fcnPtr != NULL ){

		std::stringstream msg;
		msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " disconnecting handler from signal";
		fireSignal( 000, msg.str().c_str() );

		globalErrorSignal.disconnect( fcnPtr );
		LOG( Log::INF, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " disconnect handler from signal.";
		return true;
	} else {
		LOG( Log::ERR, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " cannot disconnect NULL handler to signal. skipping...";
		return false;
	}
}
void GlobalErrorSignaler::disconnectAllHandlers() {
	std::stringstream msg;
	msg << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " disconnecting all handlers from signal";
	fireSignal( 000, msg.str().c_str() );

	globalErrorSignal.disconnect_all_slots();
	LOG( Log::INF, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " disconnected all handlers from signal.";
}

// fire the signal with payload. Timestamp done internally
void GlobalErrorSignaler::fireSignal( const int code, const char *msg ){
	timeval ftTimeStamp;
	auto now = std::chrono::system_clock::now();
	auto nMicrosecs = std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch());
	ftTimeStamp.tv_sec = nMicrosecs.count() / 1000000L;
	ftTimeStamp.tv_usec = (nMicrosecs.count() % 1000000L) ;
	globalErrorSignal( code, msg, ftTimeStamp );

	// would like to throw out always an ERR log as well, but the signal should work also if LogIt is bad, independently
	LOG( Log::ERR, GlobalErrorSignaler::m_st_lh ) << __FUNCTION__ << " " << __FILE__ << " " << __LINE__ << " fire an ERR signal: code= " << code << " " << msg;
}

// TODO: rewrite this -- from Piotr
void CanParameters::scanParameters(std::string parameters)
{
    const char * canpars = parameters.c_str();
    if (strcmp(canpars, "Unspecified") != 0) {
#ifdef _WIN32
        m_iNumberOfDetectedParameters = sscanf_s(canpars, "%ld %u %u %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp, &m_iSyncMode, &m_iTimeout);
#else
        m_iNumberOfDetectedParameters = sscanf(canpars, "%ld %u %u %u %u %u %u", &m_lBaudRate, &m_iOperationMode, &m_iTermination, &m_iHighSpeed, &m_iTimeStamp, &m_iSyncMode, &m_iTimeout);
#endif
    } else {
        m_dontReconfigure = true;
    }
}
#if 0
/* static */ LogItInstance* CCanAccess::st_getLogItInstance()
{
	return( CCanAccess::st_logItRemoteInstance );
}
#endif

bool CCanAccess::initialiseLogging(LogItInstance* remoteInstance)
{
	bool ret = Log::initializeDllLogging(remoteInstance);
	m_logItRemoteInstance = remoteInstance;
	CCanAccess::st_logItRemoteInstance = remoteInstance;
	return ret;
}

/**
 * compared to the last received message, are we in timeout?
 */
bool CCanAccess::hasTimeoutOnReception() {
	m_dnow = std::chrono::high_resolution_clock::now();
	auto delta_us = std::chrono::duration<double, std::chrono::microseconds::period>( m_dnow - m_dopen);
	if ( delta_us.count() / 1000 > m_timeoutOnReception ) return true;
	else return false;
}




/**
 * we publish port status via a "portStatusChanged" signal, for all vendors, if it has changed, or if a new handler got connected.
 * When a port is created we get a port status, and we can fire a signal, but no handler is yet connected. So we fire again if the
 * connection count has increased.
 * This is just the standardized mechanism
 */
void CCanAccess::publishPortStatusChanged ( unsigned int status )
{
	// std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << " port_status_change status=  " << status << " m_portStatus= " << m_portStatus << std::endl;
	bool fireSignal = false;
	m_canPortStateChanged_nbSlots_current = canPortStateChanged.num_slots();
	if ( m_canPortStateChanged_nbSlots_current > m_canPortStateChanged_nbSlots_previous ){
		fireSignal = true;
		LOG(Log::TRC, m_lh) << __FUNCTION__ << " new handler got connected, sending a canPortStateChanged signal (" << canPortStateChanged.num_slots() << " handlers connected)";
	}
	m_canPortStateChanged_nbSlots_previous = m_canPortStateChanged_nbSlots_current;
	if ( m_portStatus != status ){
		fireSignal = true;
		LOG(Log::TRC, m_lh) << __FUNCTION__ << " port status has changed, sending a canPortStateChanged signal";
	}
	if ( fireSignal ){
		std::string msg = CanModule::translateCanBusStateToText((CanModule::CanModule_bus_state) status);

		timeval ftTimeStamp;
		auto now = std::chrono::system_clock::now();
		auto nMicrosecs = std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch());
		ftTimeStamp.tv_sec = nMicrosecs.count() / 1000000L;
		ftTimeStamp.tv_usec = (nMicrosecs.count() % 1000000L) ;
		canPortStateChanged( status, msg.c_str(), ftTimeStamp ); // signal

		LOG(Log::TRC, m_lh) << __FUNCTION__ << " sent canPortStateChanged signal (" << canPortStateChanged.num_slots() << " handlers connected)";
	}
	m_portStatus = status;
}


std::vector<std::string> CCanAccess::parseNameAndParameters(std::string name, std::string parameters)
{
	LOG(Log::INF) << "******";
	LOG(Log::INF) << __FUNCTION__ << " name= " << name << " parameters= " << parameters;

	bool isSocketCanLinux = false;
	std::size_t s = name.find("sock");
	if (s != std::string::npos)
	{
		isSocketCanLinux = true;
	}

	/** care for different syntax:
	 * "sock:0" => sock:can0
	 * "sock:can0" => "sock:can0"
	 * "sock:vcan0" => "sock:vcan0"
	 *
	 * "sock:whatsoever0" => "sock:can0"
	 * "sock:whatsoevervcan0" => "sock:vcan0"
	 * "sock:wvcanhatso0" => "sock:vcan0"
	 *
	 * if you specify only an integer portnumber we default to "can"
	 * if you specify ":can" we stay with "can"
	 * if you specify ":vcan" we stay with "vcan"
	 * if you specify anything else which contains a string and an integer, we use can
	 * if you specify anything else which contains a string containing ":vcan" and an integer, we use vcan
	 * so basically you have the freedom to call your devices as you want. Maybe this is too much freedom.
	 */
	std::size_t foundVcan = name.find(":vcan", 0);
	std::size_t foundSeperator = name.find_first_of(":", 0);
	std::size_t foundPortNumber = name.find_first_of("0123456789", foundSeperator);
	if (foundPortNumber != std::string::npos)
	{
		name.erase(foundSeperator + 1, foundPortNumber - foundSeperator - 1);
	}
	else
	{
		throw std::runtime_error("could not decode port number (need an integer, i.e. can0)");
	}

	// for socketcan, have to prefix "can" or "vcan" to port number
	if (isSocketCanLinux)
	{
		foundSeperator = name.find_first_of(":", 0);
		if (foundVcan != std::string::npos)
		{
			m_sBusName = name.insert(foundSeperator + 1, "vcan");
		}
		else
		{
			m_sBusName = name.insert(foundSeperator + 1, "can");
		}
	}
	else
	{
		m_sBusName = name;
	}

	LOG(Log::DBG) << __FUNCTION__ << " m_sBusName= " << m_sBusName;

	vector<string> stringVector;
	istringstream nameSS(name);
	string temporalString;
	while (getline(nameSS, temporalString, ':'))
	{
		stringVector.push_back(temporalString);
		//LOG(Log::TRC, m_lh) << __FUNCTION__ << " stringVector new element= " << temporalString;
	}
	m_CanParameters.scanParameters(parameters);
	LOG(Log::DBG) << __FUNCTION__ << " stringVector size= " << stringVector.size();
	for (const auto &value : stringVector)
	{
		LOG(Log::INF) << __FUNCTION__ << " " << value;
	}
	return stringVector;
}

}
