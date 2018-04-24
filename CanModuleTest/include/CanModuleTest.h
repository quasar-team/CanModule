/*
 * CanModuleTest.h
 *
 *  Created on: Oct 8, 2015
 *      Author: dabalo
 */

#ifndef CANMODULETEST_H_
#define CANMODULETEST_H_

#include "gtest/gtest.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include "CanMessage.h"

class CanModuleTest;

class CanModuleTest  : public ::testing::Test
{
public:

	CanModuleTest();
	virtual ~CanModuleTest();

	class MsgRcvdFunctor
	{
	public:
		MsgRcvdFunctor();
		virtual ~MsgRcvdFunctor();

		void operator()(const CanMessage& msg);
		std::vector< boost::shared_ptr<CanMessage> > m_msgsRecvd;
	};
};

#endif /* CANMODULETEST_H_ */
