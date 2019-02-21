/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * ExportDefinition.h
 *
 *  Created on: Apr 4, 2011
 *      Author: vfilimon
 *      maintaining touches: mludwig, quasar team
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
 *
 */
#ifndef EXPORT_DEFINITION_H_
#define EXPORT_DEFINITION_H_

/**
* If building dll, visual studio needs to know which functions are externally visible
*/
#ifdef _WINDOWS
	#define SHARED_LIB_EXPORT_DEFN __declspec(dllexport)
#else
	#define SHARED_LIB_EXPORT_DEFN // nothing for linux
#endif //_WINDOWS

#endif /* EXPORT_DEFINITION_H_ */
