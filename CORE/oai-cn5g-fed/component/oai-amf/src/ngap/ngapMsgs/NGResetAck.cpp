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

#include "NGResetAck.hpp"
#include "logger.hpp"

extern "C" {
#include "Ngap_NGAP-PDU.h"
#include "asn_codecs.h"
#include "constr_TYPE.h"
#include "constraints.h"
#include "dynamic_memory_check.h"
#include "per_decoder.h"
#include "per_encoder.h"
}

#include <iostream>
#include <vector>

using namespace std;

namespace ngap {

//------------------------------------------------------------------------------
NGResetAckMsg::NGResetAckMsg() {
  ngResetAckPdu                        = nullptr;
  ngResetAckIEs                        = nullptr;
  ueAssociationLogicalNGConnectionList = nullptr;
  CriticalityDiagnostics               = nullptr;
}
//------------------------------------------------------------------------------
NGResetAckMsg::~NGResetAckMsg() {
  if (ngResetAckPdu) free_wrapper((void**) &ngResetAckPdu);
  if (ngResetAckIEs) free_wrapper((void**) &ngResetAckIEs);
  if (ueAssociationLogicalNGConnectionList)
    free_wrapper((void**) &ueAssociationLogicalNGConnectionList);
  if (CriticalityDiagnostics) free_wrapper((void**) &CriticalityDiagnostics);
}

//------------------------------------------------------------------------------
void NGResetAckMsg::setMessageType() {
  if (!ngResetAckPdu) {
    ngResetAckPdu = (Ngap_NGAP_PDU_t*) calloc(1, sizeof(Ngap_NGAP_PDU_t));
  }

  MessageType NgResetMessageTypeIE;
  NgResetMessageTypeIE.setProcedureCode(Ngap_ProcedureCode_id_NGReset);
  NgResetMessageTypeIE.setTypeOfMessage(Ngap_NGAP_PDU_PR_successfulOutcome);
  NgResetMessageTypeIE.setValuePresent(
      Ngap_SuccessfulOutcome__value_PR_NGResetAcknowledge);

  if (NgResetMessageTypeIE.getProcedureCode() ==
          Ngap_ProcedureCode_id_NGReset &&
      NgResetMessageTypeIE.getTypeOfMessage() ==
          Ngap_NGAP_PDU_PR_successfulOutcome) {
    NgResetMessageTypeIE.encode2pdu(ngResetAckPdu);

    ngResetAckIEs = &(ngResetAckPdu->choice.successfulOutcome->value.choice
                          .NGResetAcknowledge);
  } else {
    Logger::ngap().warn(
        "This information doesn't refer to NGResetAck message!");
  }
}

//------------------------------------------------------------------------------
void NGResetAckMsg::setUE_associatedLogicalNG_connectionList(
    std::vector<UEAssociationLogicalNGConnectionItem>& list) {
  if (!ueAssociationLogicalNGConnectionList) {
    ueAssociationLogicalNGConnectionList =
        (UEAssociationLogicalNGConnectionList*) calloc(
            1, sizeof(UEAssociationLogicalNGConnectionList));
  }
  ueAssociationLogicalNGConnectionList->setUEAssociationLogicalNGConnectionItem(
      list);
  addUE_associatedLogicalNG_connectionList();
}

//------------------------------------------------------------------------------
void NGResetAckMsg::getUE_associatedLogicalNG_connectionList(
    std::vector<UEAssociationLogicalNGConnectionItem>& list) {
  if (ueAssociationLogicalNGConnectionList) {
    ueAssociationLogicalNGConnectionList
        ->getUEAssociationLogicalNGConnectionItem(list);
  }
}

//------------------------------------------------------------------------------
void NGResetAckMsg::addUE_associatedLogicalNG_connectionList() {
  Ngap_NGResetAcknowledgeIEs_t* ie = (Ngap_NGResetAcknowledgeIEs_t*) calloc(
      1, sizeof(Ngap_NGResetAcknowledgeIEs_t));
  ie->id          = Ngap_ProtocolIE_ID_id_UE_associatedLogicalNG_connectionList;
  ie->criticality = Ngap_Criticality_ignore;
  ie->value.present =
      Ngap_NGResetAcknowledgeIEs__value_PR_UE_associatedLogicalNG_connectionList;

  ueAssociationLogicalNGConnectionList->encode(
      &ie->value.choice.UE_associatedLogicalNG_connectionList);

  int ret = ASN_SEQUENCE_ADD(&ngResetAckIEs->protocolIEs.list, ie);
  if (ret != 0)
    Logger::ngap().error(
        "Encode NGAP UE_associatedLogicalNG_connectionList IE error");
}

//------------------------------------------------------------------------------
int NGResetAckMsg::encode2buffer(uint8_t* buf, int buf_size) {
  asn_fprint(stderr, &asn_DEF_Ngap_NGAP_PDU, ngResetAckPdu);
  asn_enc_rval_t er = aper_encode_to_buffer(
      &asn_DEF_Ngap_NGAP_PDU, NULL, ngResetAckPdu, buf, buf_size);
  Logger::ngap().debug("er.encoded( %d )", er.encoded);
  return er.encoded;
}

//------------------------------------------------------------------------------
bool NGResetAckMsg::decodefrompdu(Ngap_NGAP_PDU_t* ngap_msg_pdu) {
  ngResetAckPdu = ngap_msg_pdu;

  if (ngResetAckPdu->present == Ngap_NGAP_PDU_PR_successfulOutcome) {
    if (ngResetAckPdu->choice.successfulOutcome &&
        ngResetAckPdu->choice.successfulOutcome->procedureCode ==
            Ngap_ProcedureCode_id_NGReset &&
        ngResetAckPdu->choice.successfulOutcome->criticality ==
            Ngap_Criticality_reject &&
        ngResetAckPdu->choice.successfulOutcome->value.present ==
            Ngap_SuccessfulOutcome__value_PR_NGResetAcknowledge) {
      ngResetAckIEs = &ngResetAckPdu->choice.successfulOutcome->value.choice
                           .NGResetAcknowledge;
      for (int i = 0; i < ngResetAckIEs->protocolIEs.list.count; i++) {
        switch (ngResetAckIEs->protocolIEs.list.array[i]->id) {
          case Ngap_ProtocolIE_ID_id_UE_associatedLogicalNG_connectionList: {
            if (ngResetAckIEs->protocolIEs.list.array[i]->criticality ==
                    Ngap_Criticality_ignore &&
                ngResetAckIEs->protocolIEs.list.array[i]->value.present ==
                    Ngap_NGResetAcknowledgeIEs__value_PR_UE_associatedLogicalNG_connectionList) {
              ueAssociationLogicalNGConnectionList =
                  new UEAssociationLogicalNGConnectionList();
              if (!ueAssociationLogicalNGConnectionList->decode(
                      &ngResetAckIEs->protocolIEs.list.array[i]
                           ->value.choice
                           .UE_associatedLogicalNG_connectionList)) {
                Logger::ngap().error(
                    "Decoded NGAP UE_associatedLogicalNG_connectionList IE "
                    "error");
                return false;
              }

            } else {
              Logger::ngap().error(
                  "Decoded NGAP UE_associatedLogicalNG_connectionList IE "
                  "error");
              return false;
            }
          } break;
          case Ngap_ProtocolIE_ID_id_CriticalityDiagnostics: {
            // TODO:
          } break;
          default: {
            Logger::ngap().error(
                "Decoded NGAP NGResetAck message PDU IE error");
            return false;
          }
        }
      }
    } else {
      Logger::ngap().error("Check NGResetAck message error!");
      return false;
    }
  } else {
    Logger::ngap().error("Check NGResetAck message error!");
    return false;
  }
  return true;
}

}  // namespace ngap
