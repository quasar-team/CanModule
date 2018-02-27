/*
 * CanModuleTest.cpp
 *
 *  Created on: Oct 8, 2015
 *      Author: dabalo
 */

#include "CanModuleTest.h"
#include <stdint.h>
#include <boost/bind.hpp>
#include <LogIt.h>
#include <LogLevels.h>
#include "CanLibLoader.h"
#include "CCanAccess.h"
#include "MockCanAccess.h"

using namespace CanModule;
using std::string;

CanModuleTest::CanModuleTest()
{
	Log::initializeLogging(Log::TRC);
}

CanModuleTest::~CanModuleTest()
{}

CanModuleTest::MsgRcvdFunctor::MsgRcvdFunctor()
{}

CanModuleTest::MsgRcvdFunctor::~MsgRcvdFunctor()
{}

void CanModuleTest::MsgRcvdFunctor::operator()(const CanMessage& msg)
{
	const string dataAsString(std::string(msg.c_data, msg.c_data + (msg.c_dlc*sizeof(unsigned char))));
	LOG(Log::INF) << __FUNCTION__ << " ID ["<<msg.c_id<<"] len ["<<(unsigned int)msg.c_dlc<<"] message [0x"<<dataAsString<<"] rtr ["<<msg.c_rtr<<"]";
	boost::shared_ptr<CanMessage> msgClone(new CanMessage());

	msgClone->c_id = msg.c_id;
	msgClone->c_ff = msg.c_ff;
	msgClone->c_dlc = msg.c_dlc;
	memcpy(msgClone->c_data, msg.c_data, 8);
	msgClone->c_time.tv_sec = msg.c_time.tv_sec;
	msgClone->c_time.tv_usec = msg.c_time.tv_usec;
	msgClone->c_rtr = msg.c_rtr;
	m_msgsRecvd.push_back(msgClone);
}

TEST_F(CanModuleTest, loadWrongLib)
{
	try 
	{
		CanLibLoader::createInstance("LibDoesNotExist");
		FAIL() << "expected exception to be thrown, lib does not exist";
	}
	catch (...){/*OK, expected*/}
}

TEST_F(CanModuleTest, loadCorrectLib)
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

TEST_F(CanModuleTest, openCanBus)
{
	CanLibLoader* libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");

	string parameters = "Unspecified";
	const char* name = "FakeName";
	CCanAccess* canBusAccessInstance = libLoaderInstance->openCanBus(name, parameters);
	EXPECT_FALSE(canBusAccessInstance == 0);
}

TEST_F(CanModuleTest, sendMessage)
{
	CanLibLoader* libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	
	const char*  parameters = "Unspecified";
	const char*  name = "FakeName9";
	CCanAccess* canBusAccessInstance = libLoaderInstance->openCanBus(name, parameters);
	EXPECT_TRUE(canBusAccessInstance->sendMessage(1, 8, (unsigned char*)("12345678"), false));
	EXPECT_TRUE(canBusAccessInstance->sendMessage(2, 8, (unsigned char*)("22345678"), false));
	EXPECT_TRUE(canBusAccessInstance->sendMessage(3, 8, (unsigned char*)("32345678"), false));

	CanStatistics stats;
	canBusAccessInstance->getStatistics(stats);
	EXPECT_EQ(3, stats.totalTransmitted()) << "test transmits 3 packages";
}

TEST_F(CanModuleTest, sendRecvEchoMessage)
{
	CanLibLoader* libLoaderInstance = CanLibLoader::createInstance("MockUpCanImplementation");
	CCanAccess* canBusAccessInstance = libLoaderInstance->openCanBus("", "");

	CanModuleTest::MsgRcvdFunctor handler;
	canBusAccessInstance->canMessageCame.connect(boost::ref(handler));

	canBusAccessInstance->sendMessage(1, 8, CAN_ECHO_MSG, false); // send special echo test message
	EXPECT_EQ(1, handler.m_msgsRecvd.size());
	EXPECT_EQ(0, memcmp(CAN_ECHO_MSG, handler.m_msgsRecvd[0]->c_data, 8)) << "expected msg [0x"<<std::hex<<CAN_ECHO_MSG<<std::dec<<"] received [0x"<<std::hex<<handler.m_msgsRecvd[0]->c_data<<std::dec<<"]";
}
