/*
 * CanModuleTest.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: dabalo
 */

#include "CanModuleTest.h"
#include <stdint.h>
//#include <list>
#include <LogIt.h>
#include "CanLibExternCWrapper.h"
#include "CanLibLoader.h"
#include "CCanAccess.h"
/*	#include <boost/thread.hpp>
#include <boost/foreach.hpp>*/

#ifdef _WIN32

#define INITIALIZELOGITIFNEEDED  

#else

#define INITIALIZELOGITIFNEEDED Log::initializeLogging(Log::INF);

#endif


using namespace CanModule;

TEST(CanModuleTest, loadWrongLib)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	try 
	{
		CanLibLoader::createInstance("MockUpCanImplementationFake");
	}
	catch (...)
	{
		catched = true;
	}
	EXPECT_TRUE(catched);
}

TEST (CanModuleTest, loadWrongLibWrapper)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	try
	{
		initializeWrapper("MockUpCanImplementationFake");
	}
	catch (...)
	{
		catched = true;
	}
	EXPECT_TRUE(catched);
}


TEST(CanModuleTest, loadCorrectLib)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	try 
	{
		CanLibLoader::createInstance("MockUpCanImplementation");
	}
	catch (int e)
	{
		catched = true;
	}
	EXPECT_FALSE(catched);
}

TEST(CanModuleTest, loadCorrectLibWrapper)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	try
	{
		initializeWrapper("MockUpCanImplementation");
	}
	catch (int e)
	{
		catched = true;
	}
	EXPECT_FALSE(catched);
}

TEST(CanModuleTest, openPort)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	CanLibLoader* libLoaderInstance;
	try
	{
		libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	}
	catch (int e)
	{
		catched = true;
	}
	EXPECT_FALSE(catched);
	char* parameters = "Unspecified";
	char* name = "FakeName";
	CCanAccess* canBusAccessInstance = libLoaderInstance->openCanBus(name, parameters);
	EXPECT_FALSE(canBusAccessInstance == 0);
}

TEST(CanModuleTest, openPortWrapper)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	CanLibLoader* libLoaderInstance;
	try
	{
		libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	}
	catch (int e)
	{
		catched = true;
	}
	EXPECT_FALSE(catched);
	char* parameters = "Unspecified";
	char* name = "FakeName1";
	openCanBus(name, parameters);
}

TEST(CanModuleTest, sendMessage)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	CanLibLoader* libLoaderInstance;
	try
	{
		libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	}
	catch (int e)
	{
		catched = true;
	}
	EXPECT_FALSE(catched);
	
	char* parameters = "Unspecified";
	char* name = "FakeName9";
	CCanAccess* canBusAccessInstance = libLoaderInstance->openCanBus(name, parameters);
	EXPECT_FALSE(canBusAccessInstance == 0);	
	EXPECT_TRUE(canBusAccessInstance->sendMessage(1, 8, (unsigned char*)("12345678"), false));
	EXPECT_TRUE(canBusAccessInstance->sendMessage(2, 8, (unsigned char*)("22345678"), false));
	EXPECT_TRUE(canBusAccessInstance->sendMessage(3, 8, (unsigned char*)("32345678"), false));
	
}

TEST(CanModuleTest, sendMessageWrapper)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	CanLibLoader* libLoaderInstance;
	try
	{
		libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	}
	catch (int e)
	{
		catched = true;
	}
	EXPECT_FALSE(catched);
	
	char* parameters = "Unspecified";
	char* name = "FakeName2";
	openCanBus(name, parameters);
	EXPECT_TRUE(sendMessage(name, 1, 8, (unsigned char*)("12345678"), false));
	EXPECT_TRUE(sendMessage(name, 2, 8, (unsigned char*)("22345678"), false));
	EXPECT_TRUE(sendMessage(name, 3, 8, (unsigned char*)("32345678"), false));
}

TEST(CanModuleTest, readMessageWrapper)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	CanLibLoader* libLoaderInstance;
	try
	{
		libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	}
	catch (int e)
	{
		catched = true;
	}
	EXPECT_FALSE(catched);
	
	char* parameters = "Unspecified";
	char* name = "FakeName3";
	openCanBus(name, parameters);
	CanMsgStruct* msg = new CanMsgStruct();
	EXPECT_FALSE(readMessage(name, msg));//We expect false because the queue is empty
}

TEST(CanModuleTest, closePortWrapper)
{
	INITIALIZELOGITIFNEEDED
	bool catched = false;
	CanLibLoader* libLoaderInstance;
	try
	{
		libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	}
	catch (int e)
	{
		catched = true;
	}
	EXPECT_FALSE(catched);

	char* parameters = "Unspecified";
	char* name = "FakeName4";
	openCanBus(name, parameters);
	closeCanBus(name);
}

CanModuleTest::CanModuleTest()
{
	Log::initializeLogging(Log::INF);
}

CanModuleTest::~CanModuleTest()
{}
