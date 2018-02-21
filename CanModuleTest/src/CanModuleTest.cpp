/*
 * CanModuleTest.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: dabalo
 */

#include "CanModuleTest.h"
#include <stdint.h>
#include <LogIt.h>
#include <LogLevels.h>
#include "CanLibLoader.h"
#include "CCanAccess.h"

using namespace CanModule;
using std::string;

CanModuleTest::CanModuleTest()
{
	Log::initializeLogging(Log::TRC);
}

CanModuleTest::~CanModuleTest()
{}

TEST(CanModuleTest, loadWrongLib)
{
	try 
	{
		CanLibLoader::createInstance("LibDoesNotExist");
		FAIL() << "expected exception to be thrown, lib does not exist";
	}
	catch (...){/*OK, expected*/}
}

TEST(CanModuleTest, loadCorrectLib)
{
	try 
	{
		CanLibLoader::createInstance("MockUpCanImplementation");
	}
	catch (int e)
	{
		FAIL() << "unexpected exception thrown, lib should exist";
	}
}

TEST(CanModuleTest, openCanBus)
{
	CanLibLoader* libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");

	string parameters = "Unspecified";
	const char* name = "FakeName";
	CCanAccess* canBusAccessInstance = libLoaderInstance->openCanBus(name, parameters);
	EXPECT_FALSE(canBusAccessInstance == 0);
}

TEST(CanModuleTest, sendMessage)
{
	CanLibLoader* libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	
	const char*  parameters = "Unspecified";
	const char*  name = "FakeName9";
	CCanAccess* canBusAccessInstance = libLoaderInstance->openCanBus(name, parameters);
	EXPECT_TRUE(canBusAccessInstance->sendMessage(1, 8, (unsigned char*)("12345678"), false));
	EXPECT_TRUE(canBusAccessInstance->sendMessage(2, 8, (unsigned char*)("22345678"), false));
	EXPECT_TRUE(canBusAccessInstance->sendMessage(3, 8, (unsigned char*)("32345678"), false));
}
