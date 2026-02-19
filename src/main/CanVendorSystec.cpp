/** © Copyright CERN, 2015. All rights not expressly granted are reserved.
 *
 * STCanScap.cpp
 *
 *  Created on: Jul 21, 2011
 *  Based on work by vfilimon
 *  Rework and logging done by Piotr Nikiel <piotr@nikiel.info>
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

#include "CanVendorSystec.h"

#include <time.h>
#include <vector>
#include <LogIt.h>

CanVendorSystec::CanVendorSystec(const CanDeviceArguments& args)
    : CanDevice("systec", args) {
  if (!args.config.bus_name.has_value()) {
    throw std::invalid_argument("Missing required configuration parameters");
  }
}

CanReturnCode CanVendorSystec::vendor_open() noexcept {
  return CanReturnCode::internal_api_error;
}

CanReturnCode CanVendorSystec::vendor_close() noexcept {
  return CanReturnCode::internal_api_error;
};
CanReturnCode CanVendorSystec::vendor_send(const CanFrame& frame) noexcept {
  return CanReturnCode::internal_api_error;
};
CanDiagnostics CanVendorSystec::vendor_diagnostics() noexcept {
  CanDiagnostics diagnostics{};
  return diagnostics;
};