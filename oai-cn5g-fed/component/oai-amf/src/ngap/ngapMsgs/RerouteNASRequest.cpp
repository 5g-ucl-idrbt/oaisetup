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

#include "RerouteNASRequest.hpp"
#include "logger.hpp"

extern "C" {
#include "asn_codecs.h"
#include "constr_TYPE.h"
#include "constraints.h"
#include "dynamic_memory_check.h"
#include "per_decoder.h"
#include "per_encoder.h"
}

#include <iostream>
using namespace std;

namespace ngap {

//------------------------------------------------------------------------------
RerouteNASRequest::RerouteNASRequest() {
  rerouteNASRequestPdu = nullptr;
  rerouteNASRequestIEs = nullptr;
  amfUeNgapId          = nullptr;
  allowedNssai         = nullptr;
}

//------------------------------------------------------------------------------
RerouteNASRequest::~RerouteNASRequest() {}

//------------------------------------------------------------------------------
void RerouteNASRequest::setMessageType() {
  if (!rerouteNASRequestPdu)
    rerouteNASRequestPdu =
        (Ngap_NGAP_PDU_t*) calloc(1, sizeof(Ngap_NGAP_PDU_t));

  MessageType rerouteNASRequestPduTypeIE;
  rerouteNASRequestPduTypeIE.setProcedureCode(
      Ngap_ProcedureCode_id_RerouteNASRequest);
  rerouteNASRequestPduTypeIE.setTypeOfMessage(
      Ngap_NGAP_PDU_PR_initiatingMessage);
  rerouteNASRequestPduTypeIE.setCriticality(Ngap_Criticality_reject);
  rerouteNASRequestPduTypeIE.setValuePresent(
      Ngap_InitiatingMessage__value_PR_RerouteNASRequest);

  if (rerouteNASRequestPduTypeIE.getProcedureCode() ==
          Ngap_ProcedureCode_id_RerouteNASRequest &&
      rerouteNASRequestPduTypeIE.getTypeOfMessage() ==
          Ngap_NGAP_PDU_PR_initiatingMessage &&
      rerouteNASRequestPduTypeIE.getCriticality() == Ngap_Criticality_reject) {
    rerouteNASRequestPduTypeIE.encode2pdu(rerouteNASRequestPdu);
    rerouteNASRequestIEs = &(rerouteNASRequestPdu->choice.initiatingMessage
                                 ->value.choice.RerouteNASRequest);
  } else {
    Logger::ngap().warn(
        " This information doesn't refer to RerouteNASRequest message!");
  }
}

//------------------------------------------------------------------------------
void RerouteNASRequest::setAmfUeNgapId(unsigned long id) {
  if (!amfUeNgapId) amfUeNgapId = new AMF_UE_NGAP_ID();
  amfUeNgapId->setAMF_UE_NGAP_ID(id);

  Ngap_RerouteNASRequest_IEs_t* ie = (Ngap_RerouteNASRequest_IEs_t*) calloc(
      1, sizeof(Ngap_RerouteNASRequest_IEs_t));
  ie->id            = Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality   = Ngap_Criticality_ignore;
  ie->value.present = Ngap_RerouteNASRequest_IEs__value_PR_AMF_UE_NGAP_ID;

  int ret = amfUeNgapId->encode2AMF_UE_NGAP_ID(ie->value.choice.AMF_UE_NGAP_ID);
  if (!ret) {
    Logger::ngap().error("Encode AMF_UE_NGAP_ID IE error!");
    free_wrapper((void**) &ie);
    return;
  }

  ret = ASN_SEQUENCE_ADD(&rerouteNASRequestIEs->protocolIEs.list, ie);
  if (ret != 0) Logger::ngap().error("Encode AMF_UE_NGAP_ID IE error!");
  // free_wrapper((void**) &ie);
}

//------------------------------------------------------------------------------
void RerouteNASRequest::setRanUeNgapId(uint32_t ran_ue_ngap_id) {
  ranUeNgapId.setRanUeNgapId(ran_ue_ngap_id);

  Ngap_RerouteNASRequest_IEs_t* ie = (Ngap_RerouteNASRequest_IEs_t*) calloc(
      1, sizeof(Ngap_RerouteNASRequest_IEs_t));
  ie->id            = Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality   = Ngap_Criticality_reject;
  ie->value.present = Ngap_RerouteNASRequest_IEs__value_PR_RAN_UE_NGAP_ID;

  int ret = ranUeNgapId.encode2RAN_UE_NGAP_ID(ie->value.choice.RAN_UE_NGAP_ID);
  if (!ret) {
    Logger::ngap().error("Encode RAN_UE_NGAP_ID IE error!");

    free_wrapper((void**) &ie);
    return;
  }

  ret = ASN_SEQUENCE_ADD(&rerouteNASRequestIEs->protocolIEs.list, ie);
  if (ret != 0) Logger::ngap().error("Encode RAN_UE_NGAP_ID IE error!");
  // free_wrapper((void**) &ie);
}

//------------------------------------------------------------------------------
void RerouteNASRequest::setAllowedNssai(std::vector<S_Nssai> list) {
  if (!allowedNssai) allowedNssai = new AllowedNSSAI();
  S_NSSAI* m_snssai = new S_NSSAI[list.size()]();
  for (int i = 0; i < list.size(); i++) {
    m_snssai[i].setSst(list[i].sst);
    if (list[i].sd.size() &&
        (list[i].sd.compare("None") && list[i].sd.compare("none")))
      m_snssai[i].setSd(list[i].sd);
  }
  allowedNssai->setAllowedNSSAI(m_snssai, list.size());

  Ngap_RerouteNASRequest_IEs_t* ie = (Ngap_RerouteNASRequest_IEs_t*) calloc(
      1, sizeof(Ngap_RerouteNASRequest_IEs_t));
  ie->id            = Ngap_ProtocolIE_ID_id_AllowedNSSAI;
  ie->criticality   = Ngap_Criticality_reject;
  ie->value.present = Ngap_RerouteNASRequest_IEs__value_PR_AllowedNSSAI;

  int ret = allowedNssai->encode2AllowedNSSAI(&ie->value.choice.AllowedNSSAI);
  if (!ret) {
    Logger::ngap().error("Encode AllowedNSSAI IE error!");

    free_wrapper((void**) &ie);
    return;
  }

  ret = ASN_SEQUENCE_ADD(&rerouteNASRequestIEs->protocolIEs.list, ie);
  if (ret != 0) Logger::ngap().error("Encode AllowedNSSAI IE error!");
  // free_wrapper((void**) &ie);
}

//------------------------------------------------------------------------------
int RerouteNASRequest::encode2buffer(uint8_t* buf, int buf_size) {
  asn_fprint(stderr, &asn_DEF_Ngap_NGAP_PDU, rerouteNASRequestPdu);
  asn_enc_rval_t er = aper_encode_to_buffer(
      &asn_DEF_Ngap_NGAP_PDU, NULL, rerouteNASRequestPdu, buf, buf_size);
  Logger::ngap().debug("er.encoded( %d )", er.encoded);
  return er.encoded;
}

//------------------------------------------------------------------------------
bool RerouteNASRequest::decodefrompdu(Ngap_NGAP_PDU_t* ngap_msg_pdu) {
  rerouteNASRequestPdu = ngap_msg_pdu;

  if (rerouteNASRequestPdu->present == Ngap_NGAP_PDU_PR_initiatingMessage) {
    if (rerouteNASRequestPdu->choice.initiatingMessage &&
        rerouteNASRequestPdu->choice.initiatingMessage->procedureCode ==
            Ngap_ProcedureCode_id_RerouteNASRequest &&
        rerouteNASRequestPdu->choice.initiatingMessage->criticality ==
            Ngap_Criticality_reject &&
        rerouteNASRequestPdu->choice.initiatingMessage->value.present ==
            Ngap_InitiatingMessage__value_PR_RerouteNASRequest) {
      rerouteNASRequestIEs = &rerouteNASRequestPdu->choice.initiatingMessage
                                  ->value.choice.RerouteNASRequest;
    } else {
      cout << "Check RerouteNASRequest message error!!!" << endl;
      return false;
    }
  } else {
    Logger::ngap().error("MessageType error!");
    return false;
  }
  for (int i = 0; i < rerouteNASRequestIEs->protocolIEs.list.count; i++) {
    switch (rerouteNASRequestIEs->protocolIEs.list.array[i]->id) {
      case Ngap_ProtocolIE_ID_id_AMF_UE_NGAP_ID: {
        if (rerouteNASRequestIEs->protocolIEs.list.array[i]->criticality ==
                Ngap_Criticality_ignore &&
            rerouteNASRequestIEs->protocolIEs.list.array[i]->value.present ==
                Ngap_RerouteNASRequest_IEs__value_PR_AMF_UE_NGAP_ID) {
          amfUeNgapId = new AMF_UE_NGAP_ID();
          if (!amfUeNgapId->decodefromAMF_UE_NGAP_ID(
                  rerouteNASRequestIEs->protocolIEs.list.array[i]
                      ->value.choice.AMF_UE_NGAP_ID)) {
            Logger::ngap().error("Decoded ngap AMF_UE_NGAP_ID IE error");
            return false;
          }
        } else {
          Logger::ngap().error("Decoded ngap AMF_UE_NGAP_ID IE error");
          return false;
        }
      } break;
      case Ngap_ProtocolIE_ID_id_RAN_UE_NGAP_ID: {
        if (rerouteNASRequestIEs->protocolIEs.list.array[i]->criticality ==
                Ngap_Criticality_reject &&
            rerouteNASRequestIEs->protocolIEs.list.array[i]->value.present ==
                Ngap_RerouteNASRequest_IEs__value_PR_RAN_UE_NGAP_ID) {
          if (!ranUeNgapId.decodefromRAN_UE_NGAP_ID(
                  rerouteNASRequestIEs->protocolIEs.list.array[i]
                      ->value.choice.RAN_UE_NGAP_ID)) {
            Logger::ngap().error("Decoded NGAP RAN_UE_NGAP_ID IE error");
            return false;
          }
        } else {
          Logger::ngap().error("Decoded NGAP RAN_UE_NGAP_ID IE error");
          return false;
        }
      } break;

      case Ngap_ProtocolIE_ID_id_NGAP_Message: {
        if (rerouteNASRequestIEs->protocolIEs.list.array[i]->criticality ==
                Ngap_Criticality_reject &&
            rerouteNASRequestIEs->protocolIEs.list.array[i]->value.present ==
                Ngap_RerouteNASRequest_IEs__value_PR_OCTET_STRING) {
          ngapMessage = rerouteNASRequestIEs->protocolIEs.list.array[i]
                            ->value.choice.OCTET_STRING;
          Logger::ngap().error("Decoded NGAP Message IE error");
        }
      } break;

      case Ngap_ProtocolIE_ID_id_AMFSetID: {
        if (rerouteNASRequestIEs->protocolIEs.list.array[i]->criticality ==
                Ngap_Criticality_reject &&
            rerouteNASRequestIEs->protocolIEs.list.array[i]->value.present ==
                Ngap_RerouteNASRequest_IEs__value_PR_AMFSetID) {
          if (!amfSetID.decodefrombitstring(
                  rerouteNASRequestIEs->protocolIEs.list.array[i]
                      ->value.choice.AMFSetID)) {
            Logger::ngap().error("Decoded NGAP AMFSetID error");
            return false;
          }
        } else {
          Logger::ngap().error("Decoded NGAP AMFSetID IE error");
          return false;
        }
      } break;

      case Ngap_ProtocolIE_ID_id_AllowedNSSAI: {
        if (rerouteNASRequestIEs->protocolIEs.list.array[i]->criticality ==
                Ngap_Criticality_reject &&
            rerouteNASRequestIEs->protocolIEs.list.array[i]->value.present ==
                Ngap_RerouteNASRequest_IEs__value_PR_AllowedNSSAI) {
          allowedNssai = new AllowedNSSAI();
          if (!allowedNssai->decodefromAllowedNSSAI(
                  &rerouteNASRequestIEs->protocolIEs.list.array[i]
                       ->value.choice.AllowedNSSAI)) {
            Logger::ngap().error("Decoded NGAP AllowedNSSAI IE error");
            return false;
          }
        } else {
          Logger::ngap().error("Decoded NGAP AllowedNSSAI IE error");
          return false;
        }
      } break;
      default: {
        Logger::ngap().error("Decoded NGAP Message PDU error");
        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
unsigned long RerouteNASRequest::getAmfUeNgapId() {
  if (!amfUeNgapId) return -1;
  return amfUeNgapId->getAMF_UE_NGAP_ID();
}

//------------------------------------------------------------------------------
uint32_t RerouteNASRequest::getRanUeNgapId() {
  return ranUeNgapId.getRanUeNgapId();
}

//------------------------------------------------------------------------------
bool RerouteNASRequest::getAllowedNssai(std::vector<S_Nssai>& list) {
  if (!allowedNssai) return false;
  S_NSSAI* m_snssai;
  int m_numofsnssai;
  allowedNssai->getAllowedNSSAI(m_snssai, m_numofsnssai);
  for (int i = 0; i < m_numofsnssai; i++) {
    S_Nssai s_nssai;
    m_snssai[i].getSst(s_nssai.sst);
    m_snssai[i].getSd(s_nssai.sd);
    list.push_back(s_nssai);
  }

  return true;
}

//------------------------------------------------------------------------------
void RerouteNASRequest::setNgapMessage(OCTET_STRING_t& message) {
  Ngap_RerouteNASRequest_IEs_t* ie = (Ngap_RerouteNASRequest_IEs_t*) calloc(
      1, sizeof(Ngap_RerouteNASRequest_IEs_t));
  ie->id            = Ngap_ProtocolIE_ID_id_NGAP_Message;
  ie->criticality   = Ngap_Criticality_reject;
  ie->value.present = Ngap_RerouteNASRequest_IEs__value_PR_OCTET_STRING;

  ie->value.choice.OCTET_STRING = message;

  int ret = ASN_SEQUENCE_ADD(&rerouteNASRequestIEs->protocolIEs.list, ie);
  if (ret != 0) Logger::ngap().error("Encode NGAP Message IE error!");
}

//------------------------------------------------------------------------------
bool RerouteNASRequest::getNgapMessage(OCTET_STRING_t& message) const {
  message = ngapMessage;
  return true;
}

//------------------------------------------------------------------------------
void RerouteNASRequest::setAMFSetID(const uint16_t& amf_set_id) {
  amfSetID.setAMFSetID(amf_set_id);

  Ngap_RerouteNASRequest_IEs_t* ie = (Ngap_RerouteNASRequest_IEs_t*) calloc(
      1, sizeof(Ngap_RerouteNASRequest_IEs_t));
  ie->id            = Ngap_ProtocolIE_ID_id_AMFSetID;
  ie->criticality   = Ngap_Criticality_reject;
  ie->value.present = Ngap_RerouteNASRequest_IEs__value_PR_AMFSetID;

  int ret = amfSetID.encode2bitstring(ie->value.choice.AMFSetID);
  if (!ret) {
    Logger::ngap().error("Encode AMFSetID IE error!");
    free_wrapper((void**) &ie);
    return;
  }

  ret = ASN_SEQUENCE_ADD(&rerouteNASRequestIEs->protocolIEs.list, ie);
  if (ret != 0) Logger::ngap().error("Encode AMFSetID IE error!");
  // free_wrapper((void**) &ie);
}

//------------------------------------------------------------------------------
void RerouteNASRequest::getAMFSetID(std::string& amf_set_id) {
  amfSetID.getAMFSetID(amf_set_id);
}

}  // namespace ngap
