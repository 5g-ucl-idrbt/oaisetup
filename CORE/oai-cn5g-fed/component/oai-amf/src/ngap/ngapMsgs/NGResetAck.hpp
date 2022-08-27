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
 \date 2021
 \email: contact@openairinterface.org
 */

#ifndef _NG_RESET_ACK_H_
#define _NG_RESET_ACK_H_

#include "MessageType.hpp"
#include "NgapIEsStruct.hpp"
#include "UEAssociationLogicalNGConnectionItem.hpp"
#include "UEAssociationLogicalNGConnectionList.hpp"
#include <memory>

extern "C" {
#include "Ngap_NGAP-PDU.h"
#include "Ngap_NGResetAcknowledge.h"
#include "Ngap_UE-associatedLogicalNG-connectionList.h"
#include "Ngap_ProtocolIE-Field.h"
}

namespace ngap {

class NGResetAckMsg {
 public:
  NGResetAckMsg();
  virtual ~NGResetAckMsg();

  void setMessageType();  // Initialize the PDU and populate the MessageType;

  void setUE_associatedLogicalNG_connectionList(
      std::vector<UEAssociationLogicalNGConnectionItem>& list);
  void getUE_associatedLogicalNG_connectionList(
      std::vector<UEAssociationLogicalNGConnectionItem>& list);

  void addUE_associatedLogicalNG_connectionList();
  int encode2buffer(uint8_t* buf, int buf_size);
  bool decodefrompdu(Ngap_NGAP_PDU_t* ngap_msg_pdu);
  // TODO: CriticalityDiagnostics

 private:
  Ngap_NGAP_PDU_t* ngResetAckPdu;
  Ngap_NGResetAcknowledge_t* ngResetAckIEs;
  UEAssociationLogicalNGConnectionList* ueAssociationLogicalNGConnectionList;
  Ngap_CriticalityDiagnostics_t* CriticalityDiagnostics;
};

}  // namespace ngap

#endif
