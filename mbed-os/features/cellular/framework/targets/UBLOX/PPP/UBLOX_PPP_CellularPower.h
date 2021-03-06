/*
 * Copyright (c) 2017, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UBLOX_PPP_CELLULARPOWER_H_
#define UBLOX_PPP_CELLULARPOWER_H_

#include "AT_CellularPower.h"

namespace mbed {

class UBLOX_PPP_CellularPower : public AT_CellularPower {
public:
    UBLOX_PPP_CellularPower(ATHandler &atHandler);
    virtual ~UBLOX_PPP_CellularPower();

public: //from CellularPower

    virtual nsapi_error_t on();

    virtual nsapi_error_t off();
};

MBED_DEPRECATED_SINCE("mbed-os-5.9", "This API will be deprecated, Use UBLOX_PPP_CellularPower instead of UBLOX_LISA_U_CellularPower.")
class UBLOX_LISA_U_CellularPower  : public UBLOX_PPP_CellularPower {
};

} // namespace mbed

#endif // UBLOX_PPP_CELLULARPOWER_H_
