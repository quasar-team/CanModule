/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanLibLoader.h
 *
 *  Created on: Feb 22, 2012
 *      Author: vfilimon
 *      mludwig at cern dot ch
 *
 *  This file is part of Quasar.
 *
 *  Quasar is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public Licence as published by
 *  the Free Software Foundation, either version 3 of the Licence.
 *
 *  Quasar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public Licence for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Quasar.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <string>
#include <map>
// #include <boost/thread/thread.hpp>

#include "CCanAccess.h"
#include "ExportDefinition.h"

namespace CanModule 
{

/**
 * boost::signal2 need implicit this-> pointers to work, that means they only work as methods of existing objects. So we need a class
 * for specifically implementing the globalErrorSignal, which is independent of the existence of lib and bus instances.
 */
class GlobalErrorSignaler /* singleton */
{

protected:
	GlobalErrorSignaler(){};
	~GlobalErrorSignaler(){
		globalErrorSignal.disconnect_all_slots();
	}

public:
	/**
	 * one (singleton) global signal for errors independent of bus. This is needed to listen to bus opening/creation errors where the bus does not yet exist.
	 * All buses and errors go into this signal.
	 *
	 * The recommendation is:
	 *    connect a handler to the global signal
	 *    open the bus
	 *    if successful, connect bus specific handlers for errors, receptions, port status changes, then disconnect global handler
	 *    if not successful, keep global handler and try again (server logic)
	 */
	boost::signals2::signal<void (const int,const char *,timeval &) > globalErrorSignal;

	// GlobalErrorSignaler(const GlobalErrorSignaler& obj) = delete;

	static GlobalErrorSignaler *instancePtr;
	SHARED_LIB_EXPORT_DEFN static GlobalErrorSignaler* getInstance() {
		if ( GlobalErrorSignaler::instancePtr == NULL) {
			GlobalErrorSignaler::instancePtr = new GlobalErrorSignaler();
			return GlobalErrorSignaler::instancePtr;
		} else {
			return GlobalErrorSignaler::instancePtr;
		}
	}

	//void connect(const int code, const char *msg,timeval &tv ){
	//	globalErrorSignal.connect( code, msg, tv );
	//}

};

class CanLibLoader 	{

protected:
	//Empty constructor
	CanLibLoader(const std::string& libName);

public:
	//Will cleanup the loaded dynamic library
	virtual ~CanLibLoader();
	/**
	 * creates an instance
	 */
	SHARED_LIB_EXPORT_DEFN static CanLibLoader* createInstance(const std::string& libName);

	SHARED_LIB_EXPORT_DEFN CanModule::CCanAccess * openCanBus(std::string name, std::string parameters);
	SHARED_LIB_EXPORT_DEFN	void closeCanBus(CanModule::CCanAccess *cca);

	// LogIt handle
	Log::LogComponentHandle lh;

	void setLibName( std::string ln ){ m_libName = ln; }
	std::string getLibName(){ return (m_libName);}

	static timeval timevalNow();

protected:
	//Load a dynamic library.
	virtual void dynamicallyLoadLib(const std::string& libName) = 0;
	//Uses the loaded library to create a HAL object and store it in p_halInstance
	virtual CCanAccess* createCanAccess() = 0;

private:
	std::string m_libName;

};
}
