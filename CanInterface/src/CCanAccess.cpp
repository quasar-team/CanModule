#include <CCanAccess.h>

using namespace std;

namespace CanModule
{

// TODO: rewrite this.
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

std::vector<std::string> CCanAccess::parseNameAndParameters(string name, string parameters){
		LOG(Log::TRC, m_lh) << __FUNCTION__ << " name= " << name << " parameters= " << parameters;

		bool isSocketCanLinux = false;
		std::size_t s = name.find("sock");
		if ( s != std::string::npos ){
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
		std::size_t foundSeperator = name.find_first_of (":", 0);
		std::size_t foundPortNumber = name.find_first_of ( "0123456789", foundSeperator );
		if ( foundPortNumber != std::string::npos ) {
			name.erase( foundSeperator + 1, foundPortNumber - foundSeperator - 1 );
		} else {
			throw std::runtime_error("could not decode port number (need an integer, i.e. can0)");
		}

		// for socketcan, have to prefix "can" or "vcan" to port number
		if ( isSocketCanLinux ){
			foundSeperator = name.find_first_of (":", 0);
			if ( foundVcan != std::string::npos ) {
				m_sBusName = name.insert( foundSeperator + 1, "vcan");
			} else {
				m_sBusName = name.insert( foundSeperator + 1, "can");
			}
		} else {
			m_sBusName = name;
		}

		LOG(Log::TRC, m_lh) << __FUNCTION__ << " m_sBusName= " << m_sBusName;

		vector<string> stringVector;
		istringstream nameSS(name);
		string temporalString;
		while (getline(nameSS, temporalString, ':')) {
			stringVector.push_back(temporalString);
			LOG(Log::TRC, m_lh) << __FUNCTION__ << " stringVector new element= " << temporalString;
		}
		m_CanParameters.scanParameters(parameters);
		LOG(Log::TRC, m_lh) << __FUNCTION__ << " stringVector size= " << stringVector.size();
		for(const auto& value: stringVector) {
			LOG(Log::TRC, m_lh) << __FUNCTION__ << " " << value;
		}
		return stringVector;
	}

}