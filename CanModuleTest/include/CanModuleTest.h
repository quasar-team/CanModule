/*
 * CanModuleTest.h
 *
 *  Created on: Oct 8, 2015
 *      Author: dabalo
 */

#ifndef CANMODULETEST_H_
#define CANMODULETEST_H_

#include "gtest/gtest.h"

class CanModuleTest;

class CanModuleTest  : public ::testing::Test
{
public:

	CanModuleTest();
	virtual ~CanModuleTest();

	virtual void SetUp();
};

#endif /* CANMODULETEST_H_ */
