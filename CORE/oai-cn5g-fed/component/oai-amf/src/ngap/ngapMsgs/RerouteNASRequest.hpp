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
 \author
 \date 2022
 \email: contact@openairinterface.org
 */

#ifndef _REROUTE_NAS_REQUEST_H_
#define _REROUTE_NAS_REQUEST_H_

#include "AMF-UE-NGAP-ID.hpp"
#include "MessageType.hpp"
#include "NAS-PDU.hpp"
#include "NgapIEsStruct.hpp"
#include "RAN-UE-NGAP-ID.hpp"
#include "AllowedNssai.hpp"
#include "AMFSetID.hpp"

extern "C" {
#include "Ngap_RerouteNASRequest.h"
#include "Ngap_NGAP-PDU.h"
#include "Ngap_ProtocolIE-Field.h"
}

namespace ngap {

class RerouteNASRequest {
 public:
  RerouteNASRequest();
  virtual ~RerouteNASRequest();

  void setMessageType();

  void setAmfUeNgapId(unsigned long id);  // 40 bits
  unsigned long getAmfUeNgapId();

  void setRanUeNgapId(uint32_t id);  // 32 bits
  uint32_t getRanUeNgapId();

  void setNgapMessage(OCTET_STRING_t& message);
  bool getNgapMessage(OCTET_STRING_t& message) const;

  // void setAMFSetID(const std::string& amf_set_id);
  void setAMFSetID(const uint16_t& amf_set_id);
  void getAMFSetID(std::string& amf_set_id);

  void setAllowedNssai(std::vector<S_Nssai> list);
  bool getAllowedNssai(std::vector<S_Nssai>& list);

  int encode2buffer(uint8_t* buf, int buf_size);
  bool decodefrompdu(Ngap_NGAP_PDU_t* ngap_msg_pdu);

 private:
  Ngap_NGAP_PDU_t* rerouteNASRequestPdu;
  Ngap_RerouteNASRequest_t* rerouteNASRequestIEs;

  RAN_UE_NGAP_ID ranUeNgapId;   // Mandatory
  AMF_UE_NGAP_ID* amfUeNgapId;  // Optional
  OCTET_STRING_t ngapMessage;   // Mandatory
  AMFSetID amfSetID;            // Mandatory
  AllowedNSSAI* allowedNssai;   // Optional
  // SourceToTarget-AMFInformationReroute //Optional
};

}  // namespace ngap
#endif
