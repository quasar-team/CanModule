#pragma once
#include "CanLibLoader.h"

class CanLibLoaderLin : public CanLibLoader
{
 public:
	//Empty constructor
	CanLibLoaderLin(const std::string& libName);
	//Will cleanup the loaded dynamic library
	virtual ~CanLibLoaderLin();
 protected:
	//Load a dynamic library.
	virtual void dynamicallyLoadLib(const std::string& libName);
	//Uses the loaded library to create a HAL object and store it in p_halInstance
	virtual void createCanAccess();
 private:
	//Pointer to the dynamic library stored on the memory
	void *p_dynamicLibrary;	
};
