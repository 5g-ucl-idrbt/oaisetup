/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of the
 * License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file
 \brief
 \author  Keliang DU, BUPT
 \date 2020
 \email: contact@openairinterface.org
 */

#include "UENetworkCapability.hpp"

#include <iostream>

#include "logger.hpp"
using namespace nas;
using namespace std;

//------------------------------------------------------------------------------
UENetworkCapability::UENetworkCapability(uint8_t iei) {
  _iei       = iei;
  _5g_EEASel = 0;
  _5g_EIASel = 0;
  length     = 0;
}

//------------------------------------------------------------------------------
UENetworkCapability::UENetworkCapability() {
  _iei       = 0;
  _5g_EEASel = 0;
  _5g_EIASel = 0;
  length     = 0;
}

//------------------------------------------------------------------------------
UENetworkCapability::~UENetworkCapability() {}

//------------------------------------------------------------------------------
UENetworkCapability::UENetworkCapability(
    const uint8_t iei, uint8_t _5gg_EEASel, uint8_t _5gg_EIASel) {
  _iei       = iei;
  _5g_EEASel = _5gg_EEASel;
  _5g_EIASel = _5gg_EIASel;
  length     = 4;  // Minimum length
  Logger::nas_mm().debug(
      "decoded UENetworkCapability EA(0x%d),IA(0x%d)", _5g_EEASel, _5g_EIASel);
}

//------------------------------------------------------------------------------
void UENetworkCapability::setEEASel(uint8_t sel) {
  _5g_EEASel = sel;
}

//------------------------------------------------------------------------------
void UENetworkCapability::setEIASel(uint8_t sel) {
  _5g_EIASel = sel;
}

//------------------------------------------------------------------------------
uint8_t UENetworkCapability::getEEASel() {
  return _5g_EEASel;
}

//------------------------------------------------------------------------------
uint8_t UENetworkCapability::getEIASel() {
  return _5g_EIASel;
}

//------------------------------------------------------------------------------
int UENetworkCapability::encode2buffer(uint8_t* buf, int len) {
  Logger::nas_mm().debug("encoding UENetworkCapability IEI (0x%x)", _iei);
  if (len < length) {
    Logger::nas_mm().error("Len is less than %d", length);
    return 0;
  }
  int encoded_size = 0;
  if (_iei) {
    *(buf + encoded_size) = _iei;
    encoded_size++;
    *(buf + encoded_size) = length - 2;
    encoded_size++;
    *(buf + encoded_size) = _5g_EEASel;
    encoded_size++;
    *(buf + encoded_size) = _5g_EIASel;
    encoded_size++;
  } else {
    *(buf + encoded_size) = length - 1;
    encoded_size++;
    *(buf + encoded_size) = _5g_EEASel;
    encoded_size++;
    *(buf + encoded_size) = _5g_EIASel;
    encoded_size++;
  }
  Logger::nas_mm().debug("Encoded UENetworkCapability len (%d)", encoded_size);
  return encoded_size;
}

//------------------------------------------------------------------------------
int UENetworkCapability::decodefrombuffer(
    uint8_t* buf, int len, bool is_option) {
  Logger::nas_mm().debug("Decoding UENetworkCapability IEI");
  int decoded_size = 0;
  int ie_length    = 0;
  if (is_option) {
    _iei = *(buf + decoded_size);
    decoded_size++;
  }
  length = *(buf + decoded_size);
  decoded_size++;
  ie_length = length + decoded_size;

  _5g_EEASel = *(buf + decoded_size);
  decoded_size++;
  _5g_EIASel = *(buf + decoded_size);
  decoded_size++;
  Logger::nas_mm().debug(
      "Decoded UENetworkCapability EA (0x%d), IA (0x%d)", _5g_EEASel,
      _5g_EIASel);
  Logger::nas_mm().debug(
      "Decoded UENetworkCapability len 0x%d, actual length 0x%d", decoded_size,
      ie_length);

  return ie_length;
}
