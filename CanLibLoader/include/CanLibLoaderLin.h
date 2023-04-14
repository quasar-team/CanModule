/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * CanLibLoaderLin.h
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
#include "CanLibLoader.h"

namespace CanModule 
{
class CanLibLoaderLin : public CanLibLoader
{
public:

	CanLibLoaderLin(const std::string& libName);
	virtual ~CanLibLoaderLin();

protected:
	virtual void dynamicallyLoadLib(const std::string& libName);
	virtual CCanAccess*  createCanAccess();

private:
	//Pointer to the dynamic library stored on the memory
	void *p_dynamicLibrary;
	GlobalErrorSignaler *m_gsig;

};
}
