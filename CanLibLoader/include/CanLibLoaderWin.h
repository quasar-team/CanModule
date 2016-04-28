#pragma once
#define _WINSOCKAPI_
#include <windows.h>
#include "CanLibLoader.h"

class CanLibLoaderWin : public CanLibLoader
{
 public:
	//Empty constructor
	CanLibLoaderWin(const std::string& libName);
	//Will cleanup the loaded dynamic library
	virtual ~CanLibLoaderWin();
 protected:
	//Load a dynamic library.
	virtual void dynamicallyLoadLib(const std::string& libName);
	//Uses the loaded library to create a HAL object and store it in p_halInstance
	virtual void createCanAccess();
 private:
	//Pointer to the dynamic library stored on the memory
	HMODULE p_dynamicLibrary;	
};
