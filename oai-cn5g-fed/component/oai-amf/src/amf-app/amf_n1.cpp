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

/*! \file amf_n1.cpp
 \brief
 \author Keliang DU (BUPT), Tien-Thinh NGUYEN (EURECOM)
 \date 2020
 \email: contact@openairinterface.org
 */

#include "amf_n1.hpp"

#include <curl/curl.h>

#include <bitset>

#include "AuthenticationFailure.hpp"
#include "AuthenticationInfo.h"
#include "AuthenticationRequest.hpp"
#include "AuthenticationResponse.hpp"
#include "ConfirmationData.h"
#include "ConfirmationDataResponse.h"
#include "DeregistrationAccept.hpp"
#include "DeregistrationRequest.hpp"
#include "IdentityRequest.hpp"
#include "IdentityResponse.hpp"
#include "RegistrationAccept.hpp"
#include "RegistrationReject.hpp"
#include "RegistrationRequest.hpp"
#include "SecurityModeCommand.hpp"
#include "ServiceAccept.hpp"
#include "SecurityModeComplete.hpp"
#include "ServiceRequest.hpp"
#include "String2Value.hpp"
#include "UEAuthenticationCtx.h"
#include "ULNASTransport.hpp"
#include "amf_app.hpp"
#include "amf_config.hpp"
#include "amf_n11.hpp"
#include "amf_n2.hpp"
#include "comUt.hpp"
#include "itti.hpp"
#include "itti_msg_n11.hpp"
#include "itti_msg_n2.hpp"
#include "logger.hpp"
#include "nas_algorithms.hpp"
#include "comUt.hpp"
#include "3gpp_24.501.h"
#include "sha256.hpp"
#include "AmfEventReport.h"
#include "AmfEventType.h"

extern "C" {
#include "bstrlib.h"
#include "dynamic_memory_check.h"
}

using namespace nas;
using namespace amf_application;
using namespace config;

extern itti_mw* itti_inst;
extern amf_n1* amf_n1_inst;
extern amf_n11* amf_n11_inst;
extern amf_config amf_cfg;
extern amf_app* amf_app_inst;
extern amf_n2* amf_n2_inst;
extern statistics stacs;

// Static variables
uint8_t amf_n1::no_random_delta                        = 0;
std::map<std::string, std::string> amf_n1::rand_record = {};

void amf_n1_task(void*);

//------------------------------------------------------------------------------
void amf_n1_task(void*) {
  const task_id_t task_id = TASK_AMF_N1;
  itti_inst->notify_task_ready(task_id);
  do {
    std::shared_ptr<itti_msg> shared_msg = itti_inst->receive_msg(task_id);
    auto* msg                            = shared_msg.get();

    switch (msg->msg_type) {
      case UL_NAS_DATA_IND: {
        Logger::amf_n1().info("Received UL_NAS_DATA_IND");
        itti_uplink_nas_data_ind* m =
            dynamic_cast<itti_uplink_nas_data_ind*>(msg);
        amf_n1_inst->handle_itti_message(ref(*m));
      } break;
      case DOWNLINK_NAS_TRANSFER: {
        Logger::amf_n1().info("Received DOWNLINK_NAS_TRANSFER");
        itti_downlink_nas_transfer* m =
            dynamic_cast<itti_downlink_nas_transfer*>(msg);
        amf_n1_inst->handle_itti_message(ref(*m));
      } break;
      case TIME_OUT: {
        if (itti_msg_timeout* to = dynamic_cast<itti_msg_timeout*>(msg)) {
          switch (to->arg1_user) {
            case TASK_AMF_MOBILE_REACHABLE_TIMER_EXPIRE:
              amf_n1_inst->mobile_reachable_timer_timeout(
                  to->timer_id, to->arg2_user);
              break;
            case TASK_AMF_IMPLICIT_DEREGISTRATION_TIMER_EXPIRE:
              amf_n1_inst->implicit_deregistration_timer_timeout(
                  to->timer_id, to->arg2_user);
              break;
            default:
              Logger::amf_n1().info(
                  "No handler for timer(%d) with arg1_user(%d) ", to->timer_id,
                  to->arg1_user);
          }
        }
      } break;

      case TERMINATE: {
        if (itti_msg_terminate* terminate =
                dynamic_cast<itti_msg_terminate*>(msg)) {
          Logger::amf_n1().info("Received terminate message");
          return;
        }
      } break;
      default:
        Logger::amf_n1().error("No handler for msg type %d", msg->msg_type);
    }
  } while (true);
}

//------------------------------------------------------------------------------
amf_n1::amf_n1() {
  if (itti_inst->create_task(TASK_AMF_N1, amf_n1_task, nullptr)) {
    Logger::amf_n1().error("Cannot create task TASK_AMF_N1");
    throw std::runtime_error("Cannot create task TASK_AMF_N1");
  }
  Logger::amf_n1().startup("amf_n1 started");

  // EventExposure: subscribe to UE Location Report
  ee_ue_location_report_connection = event_sub.subscribe_ue_location_report(
      boost::bind(&amf_n1::handle_ue_location_change, this, _1, _2, _3));

  // EventExposure: subscribe to UE Reachability Status change
  ee_ue_reachability_status_connection =
      event_sub.subscribe_ue_reachability_status(boost::bind(
          &amf_n1::handle_ue_reachability_status_change, this, _1, _2, _3));

  // EventExposure: subscribe to UE Registration State change
  ee_ue_registration_state_connection =
      event_sub.subscribe_ue_registration_state(boost::bind(
          &amf_n1::handle_ue_registration_state_change, this, _1, _2, _3));

  // EventExposure: subscribe to UE Connectivity State change
  ee_ue_connectivity_state_connection =
      event_sub.subscribe_ue_connectivity_state(boost::bind(
          &amf_n1::handle_ue_connectivity_state_change, this, _1, _2, _3));
}

//------------------------------------------------------------------------------
amf_n1::~amf_n1() {
  // Disconnect the boost connection
  if (ee_ue_location_report_connection.connected())
    ee_ue_location_report_connection.disconnect();
  if (ee_ue_reachability_status_connection.connected())
    ee_ue_reachability_status_connection.disconnect();
  if (ee_ue_registration_state_connection.connected())
    ee_ue_registration_state_connection.disconnect();
  if (ee_ue_connectivity_state_connection.connected())
    ee_ue_connectivity_state_connection.disconnect();
}

//------------------------------------------------------------------------------
void amf_n1::handle_itti_message(itti_downlink_nas_transfer& itti_msg) {
  long amf_ue_ngap_id             = itti_msg.amf_ue_ngap_id;
  uint32_t ran_ue_ngap_id         = itti_msg.ran_ue_ngap_id;
  std::shared_ptr<nas_context> nc = {};
  if (is_amf_ue_id_2_nas_context(amf_ue_ngap_id))
    nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);
  else {
    Logger::amf_n1().warn(
        "No existed nas_context with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT ")",
        amf_ue_ngap_id);
    return;
  }
  nas_secu_ctx* secu = nc.get()->security_ctx;
  if (!secu) {
    Logger::amf_n1().error("No Security Context found");
    return;
  }

  bstring protected_nas;
  encode_nas_message_protected(
      secu, false, INTEGRITY_PROTECTED_AND_CIPHERED, NAS_MESSAGE_DOWNLINK,
      (uint8_t*) bdata(itti_msg.dl_nas), blength(itti_msg.dl_nas),
      protected_nas);

  if (itti_msg.is_n2sm_set) {
    // PDU Session Resource Release Command
    if (itti_msg.n2sm_info_type.compare("PDU_RES_REL_CMD") == 0) {
      std::shared_ptr<itti_pdu_session_resource_release_command>
          release_command =
              std::make_shared<itti_pdu_session_resource_release_command>(
                  TASK_AMF_N1, TASK_AMF_N2);
      release_command->nas            = protected_nas;
      release_command->n2sm           = itti_msg.n2sm;
      release_command->amf_ue_ngap_id = amf_ue_ngap_id;
      release_command->ran_ue_ngap_id = ran_ue_ngap_id;
      release_command->pdu_session_id = itti_msg.pdu_session_id;

      int ret = itti_inst->send_msg(release_command);
      if (0 != ret) {
        Logger::amf_n1().error(
            "Could not send ITTI message %s to task TASK_AMF_N2",
            release_command->get_msg_name());
      }
      // PDU Session Resource Modify Request
    } else if (itti_msg.n2sm_info_type.compare("PDU_RES_MOD_REQ") == 0) {
      std::shared_ptr<itti_pdu_session_resource_modify_request>
          itti_modify_request_msg =
              std::make_shared<itti_pdu_session_resource_modify_request>(
                  TASK_AMF_N1, TASK_AMF_N2);
      itti_modify_request_msg->nas            = protected_nas;
      itti_modify_request_msg->n2sm           = itti_msg.n2sm;
      itti_modify_request_msg->amf_ue_ngap_id = amf_ue_ngap_id;
      itti_modify_request_msg->ran_ue_ngap_id = ran_ue_ngap_id;
      itti_modify_request_msg->pdu_session_id = itti_msg.pdu_session_id;

      // Get NSSAI
      std::shared_ptr<nas_context> nc = {};
      if (!is_amf_ue_id_2_nas_context(amf_ue_ngap_id)) {
        Logger::amf_n1().warn(
            "No existed NAS context for UE with amf_ue_ngap_id "
            "(" AMF_UE_NGAP_ID_FMT ")",
            amf_ue_ngap_id);
        return;
      }
      nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);

      std::shared_ptr<pdu_session_context> psc = {};
      if (!amf_app_inst->find_pdu_session_context(
              nc->imsi, itti_msg.pdu_session_id, psc)) {
        Logger::amf_n1().error(
            "Cannot get pdu_session_context with SUPI (%s)", nc->imsi.c_str());
        return;
      }

      itti_modify_request_msg->s_NSSAI.setSd(psc->snssai.sD);
      itti_modify_request_msg->s_NSSAI.setSst(std::to_string(psc->snssai.sST));

      int ret = itti_inst->send_msg(itti_modify_request_msg);
      if (0 != ret) {
        Logger::amf_n1().error(
            "Could not send ITTI message %s to task TASK_AMF_N2",
            itti_modify_request_msg->get_msg_name());
      }

    } else {
      std::shared_ptr<ue_context> uc = {};

      if (!find_ue_context(ran_ue_ngap_id, amf_ue_ngap_id, uc)) {
        Logger::amf_n1().warn("Cannot find the UE context");
        return;
      }

      if (uc.get()->isUeContextRequest) {
        // PDU SESSION RESOURCE SETUP_REQUEST
        std::shared_ptr<itti_pdu_session_resource_setup_request> psrsr =
            std::make_shared<itti_pdu_session_resource_setup_request>(
                TASK_AMF_N1, TASK_AMF_N2);
        psrsr->nas            = protected_nas;
        psrsr->n2sm           = itti_msg.n2sm;
        psrsr->amf_ue_ngap_id = amf_ue_ngap_id;
        psrsr->ran_ue_ngap_id = ran_ue_ngap_id;
        psrsr->pdu_session_id = itti_msg.pdu_session_id;

        int ret = itti_inst->send_msg(psrsr);
        if (0 != ret) {
          Logger::amf_n1().error(
              "Could not send ITTI message %s to task TASK_AMF_N2",
              psrsr->get_msg_name());
        }
      } else {
        // send using InitialContextSetupRequest
        uint8_t* kamf = nc.get()->kamf[secu->vector_pointer];
        uint8_t kgnb[32];
        uint32_t ulcount =
            secu->ul_count.seq_num | (secu->ul_count.overflow << 8);
        Authentication_5gaka::derive_kgnb(0, 0x01, kamf, kgnb);
        comUt::print_buffer("amf_n1", "kamf", kamf, 32);
        bstring kgnb_bs = blk2bstr(kgnb, 32);

        std::shared_ptr<itti_initial_context_setup_request> csr =
            std::make_shared<itti_initial_context_setup_request>(
                TASK_AMF_N1, TASK_AMF_N2);
        csr->ran_ue_ngap_id = ran_ue_ngap_id;
        csr->amf_ue_ngap_id = amf_ue_ngap_id;
        csr->kgnb           = kgnb_bs;
        csr->nas            = protected_nas;
        csr->pdu_session_id = itti_msg.pdu_session_id;
        csr->is_pdu_exist   = true;
        csr->n2sm           = itti_msg.n2sm;
        csr->is_sr          = false;  // TODO: for Service Request procedure

        int ret = itti_inst->send_msg(csr);
        if (0 != ret) {
          Logger::amf_n1().error(
              "Could not send ITTI message %s to task TASK_AMF_N2",
              csr->get_msg_name());
        }
      }
    }
  } else {
    std::shared_ptr<itti_dl_nas_transport> dnt =
        std::make_shared<itti_dl_nas_transport>(TASK_AMF_N1, TASK_AMF_N2);
    dnt->nas            = protected_nas;
    dnt->amf_ue_ngap_id = amf_ue_ngap_id;
    dnt->ran_ue_ngap_id = ran_ue_ngap_id;

    int ret = itti_inst->send_msg(dnt);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N2",
          dnt->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_n1::handle_itti_message(itti_uplink_nas_data_ind& nas_data_ind) {
  long amf_ue_ngap_id     = nas_data_ind.amf_ue_ngap_id;
  uint32_t ran_ue_ngap_id = nas_data_ind.ran_ue_ngap_id;

  std::string nas_context_key =
      "app_ue_ranid_" + to_string(ran_ue_ngap_id) + ":amfid_" +
      to_string(amf_ue_ngap_id);  // key for nas_context, option 1

  std::string snn = {};
  if (nas_data_ind.mnc.length() == 2)  // TODO: remove hardcoded value
    snn = "5G:mnc0" + nas_data_ind.mnc + ".mcc" + nas_data_ind.mcc +
          ".3gppnetwork.org";
  else
    snn = "5G:mnc" + nas_data_ind.mnc + ".mcc" + nas_data_ind.mcc +
          ".3gppnetwork.org";
  Logger::amf_n1().debug("Serving network name %s", snn.c_str());

  plmn_t plmn = {};
  plmn.mnc    = nas_data_ind.mnc;
  plmn.mcc    = nas_data_ind.mcc;

  bstring recved_nas_msg = nas_data_ind.nas_msg;
  bstring decoded_plain_msg;

  std::shared_ptr<nas_context> nc = {};
  if (nas_data_ind.is_guti_valid) {
    std::string guti = nas_data_ind.guti;
    Logger::amf_n1().debug("GUTI valid %s", guti.c_str());
    if (is_guti_2_nas_context(guti)) {
      Logger::amf_n1().debug(
          "Existing nas_context with GUTI %s, update "
          "amf_ue_ngap_id/ran_ue_ngap_id",
          guti.c_str());
      nc = guti_2_nas_context(guti);
      // Update Nas Context
      nc->amf_ue_ngap_id = nas_data_ind.amf_ue_ngap_id;
      nc->ran_ue_ngap_id = nas_data_ind.ran_ue_ngap_id;

      set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
      set_supi_2_amf_id("imsi-" + nc.get()->imsi, amf_ue_ngap_id);
      set_supi_2_ran_id("imsi-" + nc.get()->imsi, ran_ue_ngap_id);
      set_imsi_2_nas_context("imsi-" + nc.get()->imsi, nc);

    } else {
      Logger::amf_n1().error(
          "No existing nas_context with GUTI %s", nas_data_ind.guti.c_str());
      // return;
    }
  } else {
    if (is_amf_ue_id_2_nas_context(amf_ue_ngap_id)) {
      Logger::amf_n1().debug(
          "Existing nas_context with amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT,
          amf_ue_ngap_id);
      nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);
    } else
      Logger::amf_n1().warn(
          "No existing nas_context with amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT,
          amf_ue_ngap_id);
  }

  SecurityHeaderType_t type = {};
  if (!check_security_header_type(
          type, (uint8_t*) bdata(recved_nas_msg), blength(recved_nas_msg))) {
    Logger::amf_n1().error("Not 5GS MOBILITY MANAGEMENT message");
    return;
  }

  comUt::print_buffer(
      "amf_n1", "Received Uplink NAS Message", (uint8_t*) bdata(recved_nas_msg),
      blength(recved_nas_msg));

  uint8_t ulCount = 0;

  switch (type) {
    case PlainNasMsg: {
      Logger::amf_n1().debug("Received plain NAS message");
      decoded_plain_msg = recved_nas_msg;
    } break;

    case IntegrityProtected: {
      Logger::amf_n1().debug("Received integrity protected NAS message");
      ulCount = *((uint8_t*) bdata(recved_nas_msg) + 6);
      Logger::amf_n1().info(
          "Integrity protected message: ulCount(%d)", ulCount);
      decoded_plain_msg = blk2bstr(
          (uint8_t*) bdata(recved_nas_msg) + 7, blength(recved_nas_msg) - 7);
    } break;

    case IntegrityProtectedAndCiphered: {
      Logger::amf_n1().debug(
          "Received integrity protected and ciphered NAS message");
    }
    case IntegrityProtectedWithNew5GNASSecurityContext: {
      Logger::amf_n1().debug(
          "Received integrity protected with new security context NAS message");
    }
    case IntegrityProtectedAndCipheredWithNew5GNASSecurityContext: {
      Logger::amf_n1().debug(
          "Received integrity protected and ciphered with new security context "
          "NAS message");
      if (nc.get() == nullptr) {
        Logger::amf_n1().debug(
            "Abnormal condition: NAS context does not exist ...");
        return;
      }
      if (!nc.get()->security_ctx) {
        Logger::amf_n1().error("No Security Context found");
        return;
      }

      uint32_t mac32 = 0;
      if (!nas_message_integrity_protected(
              nc.get()->security_ctx, NAS_MESSAGE_UPLINK,
              (uint8_t*) bdata(recved_nas_msg) + 6, blength(recved_nas_msg) - 6,
              mac32)) {
        Logger::amf_n1().debug("IA0_5G");
      } else {
        bool isMatched      = false;
        uint8_t* buf        = (uint8_t*) bdata(recved_nas_msg);
        int buf_len         = blength(recved_nas_msg);
        uint32_t mac32_recv = ntohl((((uint32_t*) (buf + 2))[0]));
        Logger::amf_n1().debug(
            "Received mac32 (0x%x) from the message", mac32_recv);
        if (mac32 == mac32_recv) {
          isMatched = true;
          Logger::amf_n1().debug("Integrity matched");
          // nc.get()->security_ctx->ul_count.seq_num ++;
        }
        if (!isMatched) {
          Logger::amf_n1().error("Received message not integrity matched");
          return;
        }
      }

      bstring ciphered = blk2bstr(
          (uint8_t*) bdata(recved_nas_msg) + 7, blength(recved_nas_msg) - 7);
      if (!nas_message_cipher_protected(
              nc.get()->security_ctx, NAS_MESSAGE_UPLINK, ciphered,
              decoded_plain_msg)) {
        Logger::amf_n1().error("Decrypt NAS message failure");
        return;
      }
    } break;
    default: {
      Logger::amf_n1().error("Unknown NAS Message Type");
      return;
    }
  }

  comUt::print_buffer(
      "amf_n1", "Decoded Plain Message", (uint8_t*) bdata(decoded_plain_msg),
      blength(decoded_plain_msg));

  if (nas_data_ind.is_nas_signalling_estab_req) {
    Logger::amf_n1().debug("Received NAS Signalling Establishment request...");
    comUt::print_buffer(
        "amf_n1", "Decoded plain NAS Message buffer",
        (uint8_t*) bdata(decoded_plain_msg), blength(decoded_plain_msg));
    nas_signalling_establishment_request_handle(
        type, nc, nas_data_ind.ran_ue_ngap_id, nas_data_ind.amf_ue_ngap_id,
        decoded_plain_msg, snn, ulCount);
  } else {
    Logger::amf_n1().debug("Received uplink NAS message...");
    comUt::print_buffer(
        "amf_n1", "Decoded NAS message buffer",
        (uint8_t*) bdata(decoded_plain_msg), blength(decoded_plain_msg));
    uplink_nas_msg_handle(
        nas_data_ind.ran_ue_ngap_id, nas_data_ind.amf_ue_ngap_id,
        decoded_plain_msg, plmn);
  }
}

//------------------------------------------------------------------------------
void amf_n1::nas_signalling_establishment_request_handle(
    SecurityHeaderType_t type, std::shared_ptr<nas_context> nc,
    uint32_t ran_ue_ngap_id, long amf_ue_ngap_id, bstring plain_msg,
    std::string snn, uint8_t ulCount) {
  // Create NAS Context, or Update if existed
  if (!nc.get()) {
    Logger::amf_n1().debug(
        "No existing nas_context with amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT
        " --> Create a new one",
        amf_ue_ngap_id);
    nc = std::shared_ptr<nas_context>(new nas_context);
    if (!nc.get()) {
      Logger::amf_n1().error(
          "Cannot allocate memory for new nas_context, exit...");
      return;
    }
    set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
    nc.get()->ctx_avaliability_ind = false;
    // change UE connection status CM-IDLE -> CM-CONNECTED
    nc.get()->nas_status      = CM_CONNECTED;
    nc.get()->amf_ue_ngap_id  = amf_ue_ngap_id;
    nc.get()->ran_ue_ngap_id  = ran_ue_ngap_id;
    nc.get()->serving_network = snn;
    // Stop Mobile Reachable Timer/Implicit Deregistration Timer
    itti_inst->timer_remove(nc.get()->mobile_reachable_timer);
    itti_inst->timer_remove(nc.get()->implicit_deregistration_timer);

    // Trigger UE Reachability Status Notify
    if (!nc.get()->imsi.empty()) {
      string supi = "imsi-" + nc.get()->imsi;
      Logger::amf_n1().debug(
          "Signal the UE Reachability Status Event notification for SUPI %s",
          supi.c_str());
      event_sub.ue_reachability_status(supi, CM_CONNECTED, 1);
    }
  } else {
    Logger::amf_n1().debug(
        "Existing nas_context with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT ")",
        amf_ue_ngap_id);
    set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
  }

  // TODO: rewrite this
  uint8_t* buf         = (uint8_t*) bdata(plain_msg);
  uint8_t message_type = *(buf + 2);
  Logger::amf_n1().debug("NAS message type 0x%x", message_type);

  switch (message_type) {
    case REGISTRATION_REQUEST: {
      Logger::amf_n1().debug(
          "Received Registration Request message, handling...");
      registration_request_handle(
          nc, ran_ue_ngap_id, amf_ue_ngap_id, snn, plain_msg);
    } break;

    case SERVICE_REQUEST: {
      Logger::amf_n1().debug("Received Service Request message, handling...");
      if (!nc.get()) {
        Logger::amf_n1().error("No NAS Context found");
        return;
      }
      if (!nc.get()->security_ctx) {
        Logger::amf_n1().error("No Security Context found");
        return;
      }
      if (nc.get() && nc.get()->security_ctx)
        nc.get()->security_ctx->ul_count.seq_num = ulCount;
      service_request_handle(nc, ran_ue_ngap_id, amf_ue_ngap_id, plain_msg);
    } break;

    case UE_INIT_DEREGISTER: {
      Logger::amf_n1().debug(
          "Received InitialUeMessage De-registration Request message, "
          "handling...");
    } break;

    default:
      Logger::amf_n1().error("No handler for NAS message 0x%x", message_type);
  }
}

//------------------------------------------------------------------------------
void amf_n1::uplink_nas_msg_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id, bstring plain_msg,
    const plmn_t& plmn) {
  // TODO: avoid this
  uint8_t* buf         = (uint8_t*) bdata(plain_msg);
  uint8_t message_type = *(buf + 2);
  switch (message_type) {
    case AUTHENTICATION_RESPONSE: {
      Logger::amf_n1().debug(
          "Received Authentication Response message, handling...");
      authentication_response_handle(ran_ue_ngap_id, amf_ue_ngap_id, plain_msg);
    } break;
    case AUTHENTICATION_FAILURE: {
      Logger::amf_n1().debug(
          "Received Authentication Failure message, handling...");
      authentication_failure_handle(ran_ue_ngap_id, amf_ue_ngap_id, plain_msg);
    } break;
    case SECURITY_MODE_COMPLETE: {
      Logger::amf_n1().debug(
          "Received Security Mode Complete message, handling...");
      security_mode_complete_handle(ran_ue_ngap_id, amf_ue_ngap_id, plain_msg);
    } break;
    case SECURITY_MODE_REJECT: {
      Logger::amf_n1().debug(
          "Received Security Mode Reject message, handling...");
      security_mode_reject_handle(ran_ue_ngap_id, amf_ue_ngap_id, plain_msg);
    } break;
    case UL_NAS_TRANSPORT: {
      Logger::amf_n1().debug("Received UL NAS Transport message, handling...");
      ul_nas_transport_handle(ran_ue_ngap_id, amf_ue_ngap_id, plain_msg, plmn);
    } break;
    case UE_INIT_DEREGISTER: {
      Logger::amf_n1().debug(
          "Received De-registration Request message, handling...");
      ue_initiate_de_registration_handle(
          ran_ue_ngap_id, amf_ue_ngap_id, plain_msg);
    } break;
    case IDENTITY_RESPONSE: {
      Logger::amf_n1().debug("Received Identity Response message, handling...");
      identity_response_handle(ran_ue_ngap_id, amf_ue_ngap_id, plain_msg);
    } break;
    case REGISTRATION_COMPLETE: {
      Logger::amf_n1().debug(
          "Received Registration Complete message, handling...");
      registration_complete_handle(ran_ue_ngap_id, amf_ue_ngap_id, plain_msg);
    } break;
    default: {
      Logger::amf_n1().debug("Received Unknown message type, ignoring...");
    }
  }
}

//------------------------------------------------------------------------------
bool amf_n1::check_security_header_type(
    SecurityHeaderType_t& type, const uint8_t* buffer, const uint32_t length) {
  // Length should be greater than 2 for SecurityHeaderType
  if (length < 2) {
    return false;
  }
  uint8_t octet        = 0;
  uint8_t decoded_size = 0;
  octet                = *(buffer + decoded_size);
  decoded_size++;
  if (octet != 0x7e) return false;
  octet = *(buffer + decoded_size);
  decoded_size++;
  // TODO: remove hardcoded value
  switch (octet & 0x0f) {
    case 0x0:
      type = PlainNasMsg;
      break;
    case 0x1:
      type = IntegrityProtected;
      break;
    case 0x2:
      type = IntegrityProtectedAndCiphered;
      break;
    case 0x3:
      type = IntegrityProtectedWithNew5GNASSecurityContext;
      break;
    case 0x4:
      type = IntegrityProtectedAndCipheredWithNew5GNASSecurityContext;
      break;
  }
  return true;
}

//------------------------------------------------------------------------------
void amf_n1::identity_response_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
    bstring plain_msg) {
  auto identity_response = std::make_unique<IdentityResponse>();
  if (!identity_response->decodefrombuffer(
          NULL, (uint8_t*) bdata(plain_msg), blength(plain_msg))) {
    Logger::amf_n1().error("Decode Identity Response error");
    return;
  }
  string supi = {};
  if (identity_response->ie_mobility_id) {
    nas::SUCI_imsi_t imsi;
    identity_response->ie_mobility_id->getSuciWithSupiImsi(imsi);
    supi = imsi.mcc + imsi.mnc + imsi.msin;
    Logger::amf_n1().debug("Identity Response: SUCI (%s)", supi.c_str());
  }

  string ue_context_key = "app_ue_ranid_" + to_string(ran_ue_ngap_id) +
                          ":amfid_" + to_string(amf_ue_ngap_id);

  if (amf_app_inst->is_ran_amf_id_2_ue_context(ue_context_key)) {
    std::shared_ptr<ue_context> uc = {};
    uc = amf_app_inst->ran_amf_id_2_ue_context(ue_context_key);
    // Update UE context
    if (uc.get() != nullptr) {
      uc.get()->supi = "imsi-" + supi;
      // associate SUPI with UC
      amf_app_inst->set_supi_2_ue_context(uc.get()->supi, uc);
      Logger::amf_n1().debug(
          "Update UC context, SUPI %s", uc.get()->supi.c_str());
    }
  }

  std::shared_ptr<nas_context> nc = {};
  if (is_amf_ue_id_2_nas_context(amf_ue_ngap_id)) {
    nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);
    Logger::amf_n1().debug(
        "Find nas_context (%p) by amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT ")",
        nc.get(), amf_ue_ngap_id);
  } else {
    nc = std::shared_ptr<nas_context>(new nas_context);
    set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
    nc.get()->ctx_avaliability_ind = false;
  }

  // Update Nas Context
  nc.get()->ctx_avaliability_ind = true;
  nc.get()->nas_status           = CM_CONNECTED;
  nc.get()->amf_ue_ngap_id       = amf_ue_ngap_id;
  nc.get()->ran_ue_ngap_id       = ran_ue_ngap_id;
  nc.get()->is_imsi_present      = true;
  nc.get()->imsi                 = supi;
  set_supi_2_amf_id("imsi-" + nc.get()->imsi, amf_ue_ngap_id);
  set_supi_2_ran_id("imsi-" + nc.get()->imsi, ran_ue_ngap_id);
  // Stop Mobile Reachable Timer/Implicit Deregistration Timer
  itti_inst->timer_remove(nc.get()->mobile_reachable_timer);
  itti_inst->timer_remove(nc.get()->implicit_deregistration_timer);

  // Continue the Registration Procedure
  if (nc.get()->to_be_register_by_new_suci) {
    if (!nc.get()->is_stacs_available) {
      ue_info_t ueItem;
      ueItem.connStatus = "5GMM-CONNECTED";  //"CM-CONNECTED";
      ueItem.registerStatus =
          "5GMM-REG-INITIATED";  // 5GMM-COMMON-PROCEDURE-INITIATED
      ueItem.ranid = ran_ue_ngap_id;
      ueItem.amfid = amf_ue_ngap_id;
      ueItem.imsi  = nc.get()->imsi;

      // Find UE context
      std::shared_ptr<ue_context> uc = {};
      if (!find_ue_context(ran_ue_ngap_id, amf_ue_ngap_id, uc)) {
        Logger::amf_n1().warn("Cannot find the UE context");
      } else {
        ueItem.mcc    = uc.get()->cgi.mcc;
        ueItem.mnc    = uc.get()->cgi.mnc;
        ueItem.cellId = uc.get()->cgi.nrCellID;
      }

      stacs.update_ue_info(ueItem);
      set_5gmm_state(nc, _5GMM_COMMON_PROCEDURE_INITIATED);
      // stacs.display();

      string supi = "imsi-" + nc.get()->imsi;
      Logger::amf_n1().debug(
          "Signal the UE Registration State Event notification for SUPI %s",
          supi.c_str());
      event_sub.ue_registration_state(
          supi, _5GMM_COMMON_PROCEDURE_INITIATED, 1);

      nc.get()->is_stacs_available = true;
    }
    // TODO: Trigger UE Location Report

    run_registration_procedure(nc);
  }
}

//------------------------------------------------------------------------------
void amf_n1::service_request_handle(
    std::shared_ptr<nas_context> nc, const uint32_t ran_ue_ngap_id,
    const long amf_ue_ngap_id, bstring nas) {
  std::shared_ptr<ue_context> uc = {};

  if (!find_ue_context(ran_ue_ngap_id, amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  if (!nc.get() or !uc.get()) {
    Logger::amf_n1().debug(
        "Cannot find NAS/UE context, send Service Reject to UE");
    // TODO: service reject
    uint8_t nas[4];
    nas[0] = EPD_5GS_MM_MSG;
    nas[1] = PLAIN_5GS_MSG;
    nas[2] = SERVICE_REJECT;
    nas[3] = _5GMM_CAUSE_UE_IDENTITY_CANNOT_BE_DERIVED;

    std::shared_ptr<itti_dl_nas_transport> dnt =
        std::make_shared<itti_dl_nas_transport>(TASK_AMF_N1, TASK_AMF_N2);
    dnt->nas            = blk2bstr(nas, 4);
    dnt->amf_ue_ngap_id = amf_ue_ngap_id;
    dnt->ran_ue_ngap_id = ran_ue_ngap_id;

    int ret = itti_inst->send_msg(dnt);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N2",
          dnt->get_msg_name());
    }
    return;
  }

  set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
  nas_secu_ctx* secu = nc.get()->security_ctx;
  if (!secu) {
    Logger::amf_n1().error("No Security Context found");
    return;
  }
  std::unique_ptr<ServiceRequest> service_request =
      std::make_unique<ServiceRequest>();
  service_request->decodefrombuffer(
      nullptr, (uint8_t*) bdata(nas), blength(nas));
  bdestroy(nas);
  std::unique_ptr<ServiceAccept> service_accept =
      std::make_unique<ServiceAccept>();
  service_accept->setHeader(PLAIN_5GS_MSG);
  string supi    = "imsi-" + nc.get()->imsi;
  uc.get()->supi = supi;
  set_supi_2_amf_id(supi, amf_ue_ngap_id);
  set_supi_2_ran_id(supi, ran_ue_ngap_id);

  Logger::amf_n1().debug(
      "amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT
      ", ran_ue_ngap_id " GNB_UE_NGAP_ID_FMT,
      amf_ue_ngap_id, ran_ue_ngap_id);
  Logger::amf_n1().debug("Key for PDU Session context: SUPI %s", supi.c_str());

  // Get the status of PDU Session context
  std::shared_ptr<pdu_session_context> old_psc = {};
  if (amf_app_inst->is_supi_2_ue_context(supi)) {
    std::shared_ptr<ue_context> old_uc = {};
    old_uc                             = amf_app_inst->supi_2_ue_context(supi);
    uc->copy_pdu_sessions(old_uc);
    amf_app_inst->set_supi_2_ue_context(supi, uc);
  }
  /*
    //Update AMF UE NGAP ID
    std::shared_ptr<ue_ngap_context> unc = {};
    if (!amf_n2_inst->is_ran_ue_id_2_ue_ngap_context(ran_ue_ngap_id)) {
      Logger::amf_n1().error(
          "Could not find UE NGAP Context with ran_ue_ngap_id ("
    GNB_UE_NGAP_ID_FMT ")", ran_ue_ngap_id); } else { unc.get()->amf_ue_ngap_id
    = amf_ue_ngap_id;
    }
  */
  // Associate SUPI with UC
  amf_app_inst->set_supi_2_ue_context(supi, uc);

  // Get PDU session status from Service Request
  uint16_t pdu_session_status =
      (uint16_t) service_request->getPduSessionStatus();
  if (pdu_session_status == 0) {
    // Get PDU Session Status from NAS Message Container if available
    bstring plain_msg;
    if (service_request->getNasMessageContainer(plain_msg)) {
      uint8_t* buf_nas     = (uint8_t*) bdata(plain_msg);
      uint8_t message_type = *(buf_nas + 2);
      Logger::amf_n1().debug("NAS message type 0x%x", message_type);

      switch (message_type) {
        case REGISTRATION_REQUEST: {
          Logger::nas_mm().debug(
              "TODO: NAS Message Container contains a Registration Request");
        } break;

        case SERVICE_REQUEST: {
          Logger::nas_mm().debug(
              "NAS Message Container contains a Service Request, handling ...");
          std::unique_ptr<ServiceRequest> service_request_nas =
              std::make_unique<ServiceRequest>();
          service_request_nas->decodefrombuffer(
              nullptr, (uint8_t*) bdata(plain_msg), blength(plain_msg));
          bdestroy(plain_msg);
          if (service_request_nas->getPduSessionStatus() > 0) {
            pdu_session_status =
                (uint16_t) service_request_nas->getPduSessionStatus();
          }
        } break;

        default:
          Logger::nas_mm().error(
              "NAS Message Container, unknown NAS message 0x%x", message_type);
      }
    }
  }

  std::vector<uint8_t> pdu_session_to_be_activated = {};
  get_pdu_session_to_be_activated(
      pdu_session_status, pdu_session_to_be_activated);

  // No PDU Sessions To Be Activated
  if (pdu_session_to_be_activated.size() == 0) {
    Logger::amf_n1().debug("There is no PDU session to be activated");
    service_accept->setPDU_session_status(0x0000);
    uint8_t buffer[BUFFER_SIZE_256];
    int encoded_size = service_accept->encode2buffer(buffer, BUFFER_SIZE_256);
    bstring protectedNas;
    encode_nas_message_protected(
        secu, false, INTEGRITY_PROTECTED_AND_CIPHERED, NAS_MESSAGE_DOWNLINK,
        buffer, encoded_size, protectedNas);
    uint8_t* kamf = nc.get()->kamf[secu->vector_pointer];
    uint8_t kgnb[32];
    uint32_t ulcount = secu->ul_count.seq_num | (secu->ul_count.overflow << 8);
    Logger::amf_n1().debug("uplink count(%d)", secu->ul_count.seq_num);
    comUt::print_buffer("amf_n1", "kamf", kamf, 32);
    Authentication_5gaka::derive_kgnb(ulcount, 0x01, kamf, kgnb);
    bstring kgnb_bs = blk2bstr(kgnb, 32);

    std::shared_ptr<itti_initial_context_setup_request> itti_msg =
        std::make_shared<itti_initial_context_setup_request>(
            TASK_AMF_N1, TASK_AMF_N2);
    itti_msg->ran_ue_ngap_id = ran_ue_ngap_id;
    itti_msg->amf_ue_ngap_id = amf_ue_ngap_id;
    itti_msg->nas            = protectedNas;
    itti_msg->kgnb           = kgnb_bs;
    itti_msg->is_sr          = true;  // Service Request indicator
    itti_msg->is_pdu_exist   = false;

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N2",
          itti_msg->get_msg_name());
    }
    return;

  } else {
    // TODO: Contact SMF to activate UP for these sessions
    // TODO: modify itti_initial_context_setup_request for supporting multiple
    // PDU sessions

    std::shared_ptr<pdu_session_context> psc = {};

    service_accept->setPDU_session_status(
        service_request->getPduSessionStatus());
    service_accept->setPDU_session_reactivation_result(0x0000);

    uint8_t pdu_session_id = pdu_session_to_be_activated.at(0);
    if (!amf_app_inst->find_pdu_session_context(supi, pdu_session_id, psc)) {
      Logger::amf_n1().error(
          "Cannot get pdu_session_context with SUPI (%s)", supi.c_str());
      return;
    }

    uint8_t buffer[BUFFER_SIZE_256];
    int encoded_size = service_accept->encode2buffer(buffer, BUFFER_SIZE_256);
    bstring protectedNas;
    encode_nas_message_protected(
        secu, false, INTEGRITY_PROTECTED_AND_CIPHERED, NAS_MESSAGE_DOWNLINK,
        buffer, encoded_size, protectedNas);
    uint8_t* kamf = nc.get()->kamf[secu->vector_pointer];
    uint8_t kgnb[32];
    uint32_t ulcount = secu->ul_count.seq_num | (secu->ul_count.overflow << 8);
    Logger::amf_n1().debug("uplink count(%d)", secu->ul_count.seq_num);
    comUt::print_buffer("amf_n1", "kamf", kamf, 32);
    Authentication_5gaka::derive_kgnb(ulcount, 0x01, kamf, kgnb);
    bstring kgnb_bs = blk2bstr(kgnb, 32);

    std::shared_ptr<itti_initial_context_setup_request> itti_msg =
        std::make_shared<itti_initial_context_setup_request>(
            TASK_AMF_N1, TASK_AMF_N2);
    itti_msg->ran_ue_ngap_id = ran_ue_ngap_id;
    itti_msg->amf_ue_ngap_id = amf_ue_ngap_id;
    itti_msg->nas            = protectedNas;
    itti_msg->kgnb           = kgnb_bs;
    itti_msg->is_sr          = true;  // Service Request indicator
    itti_msg->pdu_session_id = pdu_session_id;
    itti_msg->is_pdu_exist   = true;
    if (psc.get()->isn2sm_avaliable) {
      itti_msg->n2sm             = psc.get()->n2sm;
      itti_msg->isn2sm_avaliable = true;
    } else {
      itti_msg->isn2sm_avaliable = false;
      Logger::amf_n1().error("Cannot get PDU session information");
    }

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N2",
          itti_msg->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_n1::registration_request_handle(
    std::shared_ptr<nas_context>& nc, const uint32_t ran_ue_ngap_id,
    const long amf_ue_ngap_id, const std::string& snn, bstring reg) {
  // Decode registration request message
  std::unique_ptr<RegistrationRequest> registration_request =
      std::make_unique<RegistrationRequest>();

  registration_request->decodefrombuffer(
      nullptr, (uint8_t*) bdata(reg), blength(reg));
  nc->registration_request = blk2bstr((uint8_t*) bdata(reg), blength(reg));
  nc->registration_request_is_set = true;
  bdestroy(reg);  // free buffer

  // Find UE context
  std::shared_ptr<ue_context> uc = {};

  if (!find_ue_context(ran_ue_ngap_id, amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  // Check 5gs Mobility Identity (Mandatory IE)
  std::string guti         = {};
  uint8_t mobility_id_type = registration_request->getMobilityIdentityType();
  switch (mobility_id_type) {
    case SUCI: {
      nas::SUCI_imsi_t imsi;
      if (!registration_request->getSuciSupiFormatImsi(imsi)) {
        Logger::amf_n1().warn("No SUCI and IMSI for SUPI Format");
      } else {
        if (!nc.get()) {
          Logger::amf_n1().debug(
              "No existing nas_context with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT
              ") --> Create new one",
              amf_ue_ngap_id);
          nc = std::shared_ptr<nas_context>(new nas_context);
          set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
          nc.get()->ctx_avaliability_ind = false;
          // change UE connection status CM-IDLE -> CM-CONNECTED
          nc.get()->nas_status      = CM_CONNECTED;
          nc.get()->amf_ue_ngap_id  = amf_ue_ngap_id;
          nc.get()->ran_ue_ngap_id  = ran_ue_ngap_id;
          nc.get()->serving_network = snn;
          // Stop Mobile Reachable Timer/Implicit Deregistration Timer
          itti_inst->timer_remove(nc.get()->mobile_reachable_timer);
          itti_inst->timer_remove(nc.get()->implicit_deregistration_timer);
        }

        nc.get()->is_imsi_present = true;
        nc.get()->imsi            = imsi.mcc + imsi.mnc + imsi.msin;
        Logger::amf_n1().debug("Received IMSI %s", nc.get()->imsi.c_str());

        // Trigger UE Reachability Status Notify
        if (!nc.get()->imsi.empty()) {
          string supi = "imsi-" + nc.get()->imsi;
          Logger::amf_n1().debug(
              "Signal the UE Reachability Status Event notification for SUPI "
              "%s",
              supi.c_str());
          event_sub.ue_reachability_status(supi, CM_CONNECTED, 1);
        }

        set_supi_2_amf_id("imsi-" + nc.get()->imsi, amf_ue_ngap_id);
        set_supi_2_ran_id("imsi-" + nc.get()->imsi, ran_ue_ngap_id);

        // Try to find old nas_context and release
        std::shared_ptr<nas_context> old_nc = {};
        old_nc = imsi_2_nas_context("imsi-" + nc.get()->imsi);
        // release
        if (old_nc.get()) {
          old_nc.reset();
        }
        set_imsi_2_nas_context("imsi-" + nc.get()->imsi, nc);
        Logger::amf_n1().info(
            "Associating IMSI (%s) with nas_context (%p)",
            ("imsi-" + nc.get()->imsi).c_str(), nc.get());
        if (!nc.get()->is_stacs_available) {
          ue_info_t ueItem;
          ueItem.connStatus = "5GMM-CONNECTED";  //"CM-CONNECTED";
          ueItem.registerStatus =
              "5GMM-REG-INITIATED";  // 5GMM-COMMON-PROCEDURE-INITIATED
          ueItem.ranid = ran_ue_ngap_id;
          ueItem.amfid = amf_ue_ngap_id;
          ueItem.imsi  = nc.get()->imsi;
          if (uc.get() != nullptr) {
            ueItem.mcc    = uc.get()->cgi.mcc;
            ueItem.mnc    = uc.get()->cgi.mnc;
            ueItem.cellId = uc.get()->cgi.nrCellID;
          }

          stacs.update_ue_info(ueItem);
          set_5gmm_state(nc, _5GMM_COMMON_PROCEDURE_INITIATED);
          // stacs.display();

          string supi = "imsi-" + nc.get()->imsi;
          Logger::amf_n1().debug(
              "Signal the UE Registration State Event notification for SUPI %s",
              supi.c_str());
          event_sub.ue_registration_state(
              supi, _5GMM_COMMON_PROCEDURE_INITIATED, 1);

          nc.get()->is_stacs_available = true;
        }
      }
    } break;

    case _5G_GUTI: {
      guti = registration_request->get_5g_guti();
      Logger::amf_n1().debug(
          "Decoded GUTI from registration request message %s", guti.c_str());
      if (guti.empty()) {
        Logger::amf_n1().warn("No GUTI IE");
      }

      if (nc.get()) {
        Logger::amf_n1().debug("Exiting nas_context");
        nc.get()->is_5g_guti_present         = true;
        nc.get()->to_be_register_by_new_suci = true;
      } else if (is_guti_2_nas_context(guti)) {
        Logger::amf_n1().debug(
            "nas_context existed with GUTI %s", guti.c_str());
        nc = guti_2_nas_context(guti);
        set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
        // Update Nas Context
        nc->amf_ue_ngap_id = amf_ue_ngap_id;
        nc->ran_ue_ngap_id = ran_ue_ngap_id;
        set_supi_2_amf_id("imsi-" + nc.get()->imsi, amf_ue_ngap_id);
        set_supi_2_ran_id("imsi-" + nc.get()->imsi, ran_ue_ngap_id);
        nc.get()->is_auth_vectors_present       = false;
        nc.get()->is_current_security_available = false;
        if (nc.get()->security_ctx)
          nc.get()->security_ctx->sc_type = SECURITY_CTX_TYPE_NOT_AVAILABLE;
      } else {
        Logger::amf_n1().debug(
            "No existing nas_context with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT
            ") --> Create new one",
            amf_ue_ngap_id);
        nc = std::shared_ptr<nas_context>(new nas_context);
        if (!nc.get()) {
          Logger::amf_n1().error(
              "Cannot allocate memory for new nas_context, exit...");
          return;
        }
        Logger::amf_n1().info(
            "Created new nas_context (%p) associated with amf_ue_ngap_id "
            "(" AMF_UE_NGAP_ID_FMT
            ") "
            "for nas_signalling_establishment_request",
            nc.get(), amf_ue_ngap_id);
        set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
        nc.get()->ctx_avaliability_ind = false;
        // change UE connection status CM-IDLE -> CM-CONNECTED
        nc.get()->nas_status                 = CM_CONNECTED;
        nc.get()->amf_ue_ngap_id             = amf_ue_ngap_id;
        nc.get()->ran_ue_ngap_id             = ran_ue_ngap_id;
        nc.get()->serving_network            = snn;
        nc.get()->is_5g_guti_present         = true;
        nc.get()->to_be_register_by_new_suci = true;
        nc.get()->ngKsi                      = 100 & 0xf;
        // nc.get()->imsi =
        // set_supi_2_amf_id("imsi-" + nc.get()->imsi, amf_ue_ngap_id);
        // set_supi_2_ran_id("imsi-" + nc.get()->imsi, ran_ue_ngap_id);

        // Stop Mobile Reachable Timer/Implicit Deregistration Timer
        itti_inst->timer_remove(nc.get()->mobile_reachable_timer);
        itti_inst->timer_remove(nc.get()->implicit_deregistration_timer);
      }
    } break;

    default: {
      Logger::amf_n1().warn("Unknown UE Mobility Identity");
    }
  }

  // Create NAS context
  if (nc.get() == nullptr) {
    // try to get the GUTI -> nas_context
    if (is_guti_2_nas_context(guti)) {
      nc = guti_2_nas_context(guti);
      set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
      nc->amf_ue_ngap_id = amf_ue_ngap_id;
      nc->ran_ue_ngap_id = ran_ue_ngap_id;
      set_supi_2_amf_id("imsi-" + nc.get()->imsi, amf_ue_ngap_id);
      set_supi_2_ran_id("imsi-" + nc.get()->imsi, ran_ue_ngap_id);

      nc.get()->is_auth_vectors_present       = false;
      nc.get()->is_current_security_available = false;
      if (nc.get()->security_ctx)
        nc.get()->security_ctx->sc_type = SECURITY_CTX_TYPE_NOT_AVAILABLE;
    } else {
      Logger::amf_n1().error("No nas_context with GUTI (%s)", guti.c_str());
      send_registration_reject_msg(
          _5GMM_CAUSE_UE_IDENTITY_CANNOT_BE_DERIVED, ran_ue_ngap_id,
          amf_ue_ngap_id);
      // release ue_ngap_context and ue_context
      if (uc.get()) uc.reset();

      if (!amf_n2_inst->is_ran_ue_id_2_ue_ngap_context(ran_ue_ngap_id)) {
        Logger::amf_n1().error(
            "No UE NGAP context with ran_ue_ngap_id (" GNB_UE_NGAP_ID_FMT ")",
            ran_ue_ngap_id);
        return;
      }

      std::shared_ptr<ue_ngap_context> unc =
          amf_n2_inst->ran_ue_id_2_ue_ngap_context(ran_ue_ngap_id);
      if (unc.get()) unc.reset();
      return;
    }
  } else {
    Logger::amf_n1().debug("Existing nas_context --> Update");
    set_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id, nc);
  }

  // Update NAS Context
  nc.get()->ran_ue_ngap_id  = ran_ue_ngap_id;
  nc.get()->amf_ue_ngap_id  = amf_ue_ngap_id;
  nc.get()->serving_network = snn;

  if (nc.get()->security_ctx)
    nc.get()->security_ctx->sc_type = SECURITY_CTX_TYPE_NOT_AVAILABLE;

  // Update UE context
  if (uc.get() != nullptr) {
    std::string supi = "imsi-" + nc.get()->imsi;
    uc.get()->supi   = supi;
    // associate SUPI with UC
    amf_app_inst->set_supi_2_ue_context(supi, uc);
    Logger::amf_n1().debug("Update UC context, SUPI %s", supi.c_str());
  }

  // Check 5GS_Registration_type IE (Mandatory IE)
  uint8_t reg_type              = 0;
  bool is_follow_on_req_pending = false;
  if (!registration_request->get5GSRegistrationType(
          is_follow_on_req_pending, reg_type)) {
    Logger::amf_n1().error("Missing Mandatory IE 5GS Registration type...");
    send_registration_reject_msg(
        _5GMM_CAUSE_INVALID_MANDATORY_INFO, ran_ue_ngap_id, amf_ue_ngap_id);
    return;
  }
  nc.get()->registration_type         = reg_type;
  nc.get()->follow_on_req_pending_ind = is_follow_on_req_pending;

  // Check ngKSI (Mandatory IE)
  uint8_t ngKSI = 0;
  if (!registration_request->getngKSI(ngKSI)) {
    Logger::amf_n1().error("Missing Mandatory IE ngKSI...");
    send_registration_reject_msg(
        _5GMM_CAUSE_INVALID_MANDATORY_INFO, ran_ue_ngap_id, amf_ue_ngap_id);
    free_wrapper((void**) &registration_request);
    return;
  }
  nc.get()->ngKsi = ngKSI;

  // Get non-current native NAS key set identity (Optional IE), used for
  // inter-system change from S1 to N1 Get 5GMM Capability IE (optional), not
  // included for periodic registration updating procedure
  uint8_t _5g_mm_cap = registration_request->get5GMMCapability();
  if (_5g_mm_cap == -1) {
    Logger::amf_n1().warn("No Optional IE 5GMMCapability available");
  }
  nc.get()->mmCapability = _5g_mm_cap;

  // Get UE Security Capability IE (optional), not included for periodic
  // registration updating procedure
  uint8_t encrypt_alg      = {0};
  uint8_t integrity_alg    = {0};
  uint8_t security_cap_eea = {0};
  uint8_t security_cap_eia = {0};

  if (!registration_request->getUeSecurityCapability(
          encrypt_alg, integrity_alg, security_cap_eea, security_cap_eia)) {
    Logger::amf_n1().warn("No Optional IE UESecurityCapability available");
  } else {
    nc.get()->ueSecurityCaplen =
        registration_request->ie_ue_security_capability->getLength();
  }

  nc.get()->ueSecurityCapEnc = encrypt_alg;
  nc.get()->ueSecurityCapInt = integrity_alg;

  nc.get()->ueSecurityCapEEA = security_cap_eea;
  nc.get()->ueSecurityCapEIA = security_cap_eia;

  // Get Requested NSSAI (Optional IE), if provided
  if (!registration_request->getRequestedNssai(nc.get()->requestedNssai)) {
    Logger::amf_n1().debug("No Optional IE RequestedNssai available");
  }

  for (auto r : nc.get()->requestedNssai) {
    Logger::nas_mm().debug(
        "Requested NSSAI SST (0x%x) SD (0x%x) hplmnSST (0x%x) hplmnSD (%d)",
        r.sst, r.sd, r.mHplmnSst, r.mHplmnSd);
  }

  nc.get()->ctx_avaliability_ind = true;

  // Get Last visited registered TAI(OPtional IE), if provided
  // Get S1 Ue network capability(OPtional IE), if ue supports S1 mode
  // Get uplink data status(Optional IE), if UE has uplink user data to be sent
  // Get pdu session status(OPtional IE), associated and active pdu sessions
  // available in UE

  bstring nas_msg;
  bool is_messagecontainer =
      registration_request->getNasMessageContainer(nas_msg);

  if (is_messagecontainer) {
    std::unique_ptr<RegistrationRequest> registration_request_msg_container =
        std::make_unique<RegistrationRequest>();
    registration_request_msg_container->decodefrombuffer(
        nullptr, (uint8_t*) bdata(nas_msg), blength(nas_msg));

    if (!registration_request_msg_container->getRequestedNssai(
            nc.get()->requestedNssai)) {
      Logger::amf_n1().debug(
          "No Optional IE RequestedNssai available in NAS Container");
    } else {
      for (auto s : nc.get()->requestedNssai) {
        Logger::amf_n1().debug(
            "Requested NSSAI inside the NAS container: SST (0x%x) SD (0x%x) "
            "hplmnSST (0x%x) hplmnSD "
            "(%d)",
            s.sst, s.sd, s.mHplmnSst, s.mHplmnSd);
      }
    }
  } else {
    Logger::amf_n1().debug(
        "No Optional NAS Container inside Registration Request message");
  }

  // Trigger UE Location Report
  if (!amf_n2_inst->is_ran_ue_id_2_ue_ngap_context(ran_ue_ngap_id)) {
    Logger::amf_n1().warn(
        "No UE NGAP context with ran_ue_ngap_id (" GNB_UE_NGAP_ID_FMT ")",
        ran_ue_ngap_id);
  } else {
    std::shared_ptr<ue_ngap_context> unc =
        amf_n2_inst->ran_ue_id_2_ue_ngap_context(ran_ue_ngap_id);

    std::shared_ptr<gnb_context> gc = {};
    if (!amf_n2_inst->is_assoc_id_2_gnb_context(unc.get()->gnb_assoc_id)) {
      Logger::amf_n1().error(
          "No existed gNB context with assoc_id (%d)", unc.get()->gnb_assoc_id);
    } else {
      gc = amf_n2_inst->assoc_id_2_gnb_context(unc.get()->gnb_assoc_id);
      if (gc.get() && uc.get()) {
        oai::amf::model::UserLocation user_location = {};
        oai::amf::model::NrLocation nr_location     = {};

        oai::amf::model::Tai tai  = {};
        nlohmann::json tai_json   = {};
        tai_json["plmnId"]["mcc"] = uc.get()->cgi.mcc;
        tai_json["plmnId"]["mnc"] = uc.get()->cgi.mnc;
        tai_json["tac"]           = std::to_string(uc.get()->tai.tac);

        try {
          from_json(tai_json, tai);
        } catch (std::exception& e) {
          Logger::amf_n1().error("Exception with Json: %s", e.what());
          return;
        }
        // uc.get()->cgi.nrCellID;
        nr_location.setTai(tai);

        nlohmann::json global_ran_node_id_json        = {};
        global_ran_node_id_json["plmnId"]["mcc"]      = uc.get()->cgi.mcc;
        global_ran_node_id_json["plmnId"]["mnc"]      = uc.get()->cgi.mnc;
        global_ran_node_id_json["gNbId"]["bitLength"] = 32;
        global_ran_node_id_json["gNbId"]["gNBValue"] =
            std::to_string(gc->globalRanNodeId);
        oai::amf::model::GlobalRanNodeId global_ran_node_id = {};
        try {
          from_json(global_ran_node_id_json, global_ran_node_id);
        } catch (std::exception& e) {
          Logger::amf_n1().error("Exception with Json: %s", e.what());
          return;
        }

        nr_location.setGlobalGnbId(global_ran_node_id);

        user_location.setNrLocation(nr_location);

        // Trigger UE Location Report
        string supi = uc.get()->supi;
        Logger::amf_n1().debug(
            "Signal the UE Location Report Event notification for SUPI %s",
            supi.c_str());
        event_sub.ue_location_report(supi, user_location, 1);
      }
    }
  }

  // Store NAS information into nas_context
  // Run the corresponding registration procedure
  switch (reg_type) {
    case INITIAL_REGISTRATION: {
      run_registration_procedure(nc);
    } break;

    case MOBILITY_REGISTRATION_UPDATING: {
      Logger::amf_n1().debug("Handling Mobility Registration Update...");
      run_mobility_registration_update_procedure(
          nc, registration_request->getUplinkDataStatus(),
          registration_request->getPduSessionStatus());
    } break;

    case PERIODIC_REGISTRATION_UPDATING: {
      Logger::amf_n1().debug("Handling Periodic Registration Update...");
      if (is_messagecontainer)
        run_periodic_registration_update_procedure(nc, nas_msg);
      else
        run_periodic_registration_update_procedure(
            nc, registration_request->getPduSessionStatus());
    } break;

    case EMERGENCY_REGISTRATION: {
      if (!amf_cfg.is_emergency_support.compare("false")) {
        Logger::amf_n1().error(
            "Network does not support emergency registration, reject ...");
        send_registration_reject_msg(
            _5GMM_CAUSE_ILLEGAL_UE, ran_ue_ngap_id,
            amf_ue_ngap_id);  // cause?
        return;
      }
    } break;
    default: {
      Logger::amf_n1().error("Unknown registration type ...");
      // TODO:
      return;
    }
  }
}

//------------------------------------------------------------------------------
bool amf_n1::is_amf_ue_id_2_nas_context(const long& amf_ue_ngap_id) const {
  std::shared_lock lock(m_amfueid2nas_context);
  if (amfueid2nas_context.count(amf_ue_ngap_id) > 0) {
    if (amfueid2nas_context.at(amf_ue_ngap_id).get() != nullptr) {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
std::shared_ptr<nas_context> amf_n1::amf_ue_id_2_nas_context(
    const long& amf_ue_ngap_id) const {
  std::shared_lock lock(m_amfueid2nas_context);
  return amfueid2nas_context.at(amf_ue_ngap_id);
}

//------------------------------------------------------------------------------
void amf_n1::set_amf_ue_ngap_id_2_nas_context(
    const long& amf_ue_ngap_id, std::shared_ptr<nas_context> nc) {
  std::unique_lock lock(m_amfueid2nas_context);
  amfueid2nas_context[amf_ue_ngap_id] = nc;
}

//------------------------------------------------------------------------------
bool amf_n1::remove_amf_ue_ngap_id_2_nas_context(const long& amf_ue_ngap_id) {
  std::unique_lock lock(m_amfueid2nas_context);
  if (amfueid2nas_context.count(amf_ue_ngap_id) > 0) {
    amfueid2nas_context.erase(amf_ue_ngap_id);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void amf_n1::set_supi_2_amf_id(
    const std::string& supi, const long& amf_ue_ngap_id) {
  std::unique_lock lock(m_nas_context);
  supi2amfId[supi] = amf_ue_ngap_id;
}

//------------------------------------------------------------------------------
bool amf_n1::supi_2_amf_id(const std::string& supi, long& amf_ue_ngap_id) {
  std::shared_lock lock(m_nas_context);
  if (supi2amfId.count(supi) > 0) {
    amf_ue_ngap_id = supi2amfId.at(supi);
    return true;
  } else {
    return false;
  }
}

//------------------------------------------------------------------------------
bool amf_n1::remove_supi_2_amf_id(const std::string& supi) {
  std::unique_lock lock(m_nas_context);
  if (supi2amfId.count(supi) > 0) {
    supi2amfId.erase(supi);
    return true;
  } else {
    return false;
  }
}

//------------------------------------------------------------------------------
void amf_n1::set_supi_2_ran_id(
    const std::string& supi, const uint32_t& ran_ue_ngap_id) {
  std::unique_lock lock(m_nas_context);
  supi2ranId[supi] = ran_ue_ngap_id;
}

//------------------------------------------------------------------------------
bool amf_n1::supi_2_ran_id(const std::string& supi, uint32_t& ran_ue_ngap_id) {
  std::shared_lock lock(m_nas_context);
  if (supi2amfId.count(supi) > 0) {
    ran_ue_ngap_id = supi2ranId.at(supi);
    return true;
  } else {
    return false;
  }
}

//------------------------------------------------------------------------------
bool amf_n1::remove_supi_2_ran_id(const std::string& supi) {
  std::unique_lock lock(m_nas_context);
  if (supi2ranId.count(supi) > 0) {
    supi2ranId.erase(supi);
    return true;
  } else {
    return false;
  }
}

//------------------------------------------------------------------------------
bool amf_n1::is_guti_2_nas_context(const std::string& guti) const {
  std::shared_lock lock(m_guti2nas_context);
  if (guti2nas_context.count(guti) > 0) {
    if (guti2nas_context.at(guti).get() != nullptr) {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
std::shared_ptr<nas_context> amf_n1::guti_2_nas_context(
    const std::string& guti) const {
  std::shared_lock lock(m_guti2nas_context);
  return guti2nas_context.at(guti);
}

//------------------------------------------------------------------------------
void amf_n1::set_guti_2_nas_context(
    const std::string& guti, const std::shared_ptr<nas_context>& nc) {
  std::unique_lock lock(m_guti2nas_context);
  guti2nas_context[guti] = nc;
}

//------------------------------------------------------------------------------
bool amf_n1::remove_guti_2_nas_context(const std::string& guti) {
  std::unique_lock lock(m_guti2nas_context);
  if (guti2nas_context.count(guti) > 0) {
    guti2nas_context.erase(guti);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
std::shared_ptr<nas_context> amf_n1::imsi_2_nas_context(
    const std::string& imsi) const {
  std::shared_lock lock(m_nas_context);
  if (imsi2nas_context.count(imsi) > 0) {
    return imsi2nas_context.at(imsi);
  } else {
    return nullptr;
  }
}

//------------------------------------------------------------------------------
void amf_n1::set_imsi_2_nas_context(
    const std::string& imsi, const std::shared_ptr<nas_context>& nc) {
  std::unique_lock lock(m_nas_context);
  imsi2nas_context[imsi] = nc;
}

//------------------------------------------------------------------------------
bool amf_n1::remove_imsi_2_nas_context(const std::string& imsi) {
  std::unique_lock lock(m_nas_context);
  if (imsi2nas_context.count(imsi) > 0) {
    imsi2nas_context.erase(imsi);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void amf_n1::itti_send_dl_nas_buffer_to_task_n2(
    bstring& nas_msg, const uint32_t ran_ue_ngap_id,
    const long amf_ue_ngap_id) {
  std::shared_ptr<itti_dl_nas_transport> msg =
      std::make_shared<itti_dl_nas_transport>(TASK_AMF_N1, TASK_AMF_N2);
  msg->ran_ue_ngap_id = ran_ue_ngap_id;
  msg->amf_ue_ngap_id = amf_ue_ngap_id;
  msg->nas            = nas_msg;

  int ret = itti_inst->send_msg(msg);
  if (0 != ret) {
    Logger::amf_n1().error(
        "Could not send ITTI message %s to task TASK_AMF_N2",
        msg->get_msg_name());
  }
}

//------------------------------------------------------------------------------
void amf_n1::send_registration_reject_msg(
    uint8_t cause_value, const uint32_t ran_ue_ngap_id,
    const long amf_ue_ngap_id) {
  Logger::amf_n1().debug("Create Registration Reject and send to UE");
  std::unique_ptr<RegistrationReject> registration_reject =
      std::make_unique<RegistrationReject>();
  registration_reject->setHeader(PLAIN_5GS_MSG);
  registration_reject->set_5GMM_Cause(cause_value);
  uint8_t buffer[BUFFER_SIZE_1024] = {0};
  int encoded_size =
      registration_reject->encode2buffer(buffer, BUFFER_SIZE_1024);
  comUt::print_buffer(
      "amf_n1", "Registration-Reject message buffer", buffer, encoded_size);
  if (!encoded_size) {
    Logger::amf_n1().error("Encode Registration-Reject message error");
    return;
  }

  bstring b = blk2bstr(buffer, encoded_size);
  itti_send_dl_nas_buffer_to_task_n2(b, ran_ue_ngap_id, amf_ue_ngap_id);
}

//------------------------------------------------------------------------------
void amf_n1::run_registration_procedure(std::shared_ptr<nas_context>& nc) {
  Logger::amf_n1().debug("Start to run Registration Procedure");
  if (!nc.get()->ctx_avaliability_ind) {
    Logger::amf_n1().error("NAS context is not available");
    return;
  }

  nc.get()->is_specific_procedure_for_registration_running = true;
  if (nc.get()->is_imsi_present) {
    Logger::amf_n1().debug("SUCI SUPI format IMSI is available");
    if (!nc.get()->is_auth_vectors_present) {
      Logger::amf_n1().debug(
          "Authentication vector in nas_context is not available");
      if (auth_vectors_generator(nc)) {  // all authentication in one (AMF)
        ngksi_t ngksi = 0;
        if (nc.get()->security_ctx &&
            nc.get()->ngKsi != NAS_KEY_SET_IDENTIFIER_NOT_AVAILABLE) {
          // ngksi = (nc.get()->ngKsi + 1) % (NGKSI_MAX_VALUE + 1);
          ngksi = (nc.get()->amf_ue_ngap_id + 1);  // % (NGKSI_MAX_VALUE + 1);
        }
        nc.get()->ngKsi = ngksi;
      } else {
        Logger::amf_n1().error("Request Authentication Vectors failure");
        send_registration_reject_msg(
            _5GMM_CAUSE_ILLEGAL_UE, nc.get()->ran_ue_ngap_id,
            nc.get()->amf_ue_ngap_id);  // cause?
        return;
      }
    } else {
      Logger::amf_n1().debug(
          "Authentication Vector in nas_context is available");
      ngksi_t ngksi = 0;
      if (nc.get()->security_ctx &&
          nc.get()->ngKsi != NAS_KEY_SET_IDENTIFIER_NOT_AVAILABLE) {
        // ngksi = (nc.get()->ngKsi + 1) % (NGKSI_MAX_VALUE + 1);
        ngksi = (nc.get()->amf_ue_ngap_id + 1);  // % (NGKSI_MAX_VALUE + 1);
        Logger::amf_n1().debug("New ngKSI (%d)", ngksi);
        // TODO: How to handle?
      }
      nc.get()->ngKsi = ngksi;
    }

    handle_auth_vector_successful_result(nc);

  } else if (nc.get()->is_5g_guti_present) {
    Logger::amf_n1().debug("Start to run UE Identification Request procedure");
    nc.get()->is_auth_vectors_present = false;
    std::unique_ptr<IdentityRequest> identity_request =
        std::make_unique<IdentityRequest>();
    identity_request->setHeader(PLAIN_5GS_MSG);
    identity_request->set_5GS_Identity_Type(SUCI);
    uint8_t buffer[BUFFER_SIZE_256];
    int encoded_size = identity_request->encode2buffer(buffer, BUFFER_SIZE_256);

    std::shared_ptr<itti_dl_nas_transport> dnt =
        std::make_shared<itti_dl_nas_transport>(TASK_AMF_N1, TASK_AMF_N2);
    dnt->nas            = blk2bstr(buffer, encoded_size);
    dnt->amf_ue_ngap_id = nc.get()->amf_ue_ngap_id;
    dnt->ran_ue_ngap_id = nc.get()->ran_ue_ngap_id;

    int ret = itti_inst->send_msg(dnt);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N2",
          dnt->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
// Get authentication vectors either from AUSF(UDM) or from AMF (generate
// locally)
bool amf_n1::auth_vectors_generator(std::shared_ptr<nas_context>& nc) {
  Logger::amf_n1().debug("Start to generate Authentication Vectors");
  if (amf_cfg.support_features.enable_external_ausf) {
    // get authentication vectors from AUSF
    if (!get_authentication_vectors_from_ausf(nc)) return false;
  } else {
    authentication_vectors_generator_in_udm(nc);
    authentication_vectors_generator_in_ausf(nc);
    Logger::amf_n1().debug("Deriving kamf");
    for (int i = 0; i < MAX_5GS_AUTH_VECTORS; i++) {
      Authentication_5gaka::derive_kamf(
          nc.get()->imsi, nc.get()->_5g_av[i].kseaf, nc.get()->kamf[i],
          0x0000);  // second parameter: abba
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::get_authentication_vectors_from_ausf(
    std::shared_ptr<nas_context>& nc) {
  Logger::amf_n1().debug("Get Authentication Vectors from AUSF");

  UEAuthenticationCtx ueauthenticationctx = {};
  AuthenticationInfo authenticationinfo   = {};
  authenticationinfo.setSupiOrSuci(nc.get()->imsi);
  authenticationinfo.setServingNetworkName(nc.get()->serving_network);
  ResynchronizationInfo resynchronizationInfo = {};
  uint8_t auts_len                            = blength(nc.get()->auts);
  uint8_t* auts_value                 = (uint8_t*) bdata(nc.get()->auts);
  std::string authenticationinfo_auts = {};
  std::string authenticationinfo_rand = {};
  if (auts_value) {
    Logger::amf_n1().debug("has AUTS");
    char* auts_s = (char*) malloc(auts_len * 2 + 1);
    memset(auts_s, 0, auts_len * 2);

    Logger::amf_n1().debug("AUTS len (%d)", auts_len);
    for (int i = 0; i < auts_len; i++) {
      sprintf(&auts_s[i * 2], "%02X", auts_value[i]);
    }

    authenticationinfo_auts = auts_s;
    comUt::print_buffer("amf_n1", "AUTS", auts_value, auts_len);
    Logger::amf_n1().info("ausf_s (%s)", auts_s);
    // generate_random(rand_value, RAND_LENGTH);
    std::map<std::string, std::string>::iterator iter;
    iter = rand_record.find(nc.get()->imsi);
    if (iter != rand_record.end()) {
      authenticationinfo_rand = iter->second;
      Logger::amf_n1().info("rand_s (%s)", authenticationinfo_rand.c_str());
    } else {
      Logger::amf_n1().error("There's no last RAND");
    }

    resynchronizationInfo.setAuts(authenticationinfo_auts);
    resynchronizationInfo.setRand(authenticationinfo_rand);
    authenticationinfo.setResynchronizationInfo(resynchronizationInfo);
    free_wrapper((void**) &auts_s);
    //    free_wrapper((void**) &rand_s);
  }
  uint8_t http_version = 1;
  if (amf_cfg.support_features.use_http2) http_version = 2;

  if (amf_n11_inst->send_ue_authentication_request(
          authenticationinfo, ueauthenticationctx, http_version)) {
    unsigned char* r5gauthdata_rand = conv::format_string_as_hex(
        ueauthenticationctx.getR5gAuthData().getRand());
    memcpy(nc.get()->_5g_av[0].rand, r5gauthdata_rand, 16);
    rand_record[nc.get()->imsi] =
        ueauthenticationctx.getR5gAuthData().getRand();
    comUt::print_buffer("amf_n1", "5G AV: RAND", nc.get()->_5g_av[0].rand, 16);
    free_wrapper((void**) &r5gauthdata_rand);

    unsigned char* r5gauthdata_autn = conv::format_string_as_hex(
        ueauthenticationctx.getR5gAuthData().getAutn());
    memcpy(nc.get()->_5g_av[0].autn, r5gauthdata_autn, 16);
    comUt::print_buffer("amf_n1", "5G AV: AUTN", nc.get()->_5g_av[0].autn, 16);
    free_wrapper((void**) &r5gauthdata_autn);

    unsigned char* r5gauthdata_hxresstar = conv::format_string_as_hex(
        ueauthenticationctx.getR5gAuthData().getHxresStar());
    memcpy(nc.get()->_5g_av[0].hxresStar, r5gauthdata_hxresstar, 16);
    comUt::print_buffer(
        "amf_n1", "5G AV: hxres*", nc.get()->_5g_av[0].hxresStar, 16);
    free_wrapper((void**) &r5gauthdata_hxresstar);

    std::map<std::string, LinksValueSchema>::iterator iter;
    iter = ueauthenticationctx.getLinks().find("5G_AKA");

    if (iter != ueauthenticationctx.getLinks().end()) {
      nc.get()->Href = iter->second.getHref();
      Logger::amf_n1().info("Links is: %s", nc.get()->Href.c_str());
    } else {
      Logger::amf_n1().error("Not found 5G_AKA");
    }
  } else {
    Logger::amf_n1().info("Could not get expected response from AUSF");
    // TODO: error handling
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::_5g_aka_confirmation_from_ausf(
    std::shared_ptr<nas_context>& nc, bstring resStar) {
  Logger::amf_n1().debug("5G AKA Confirmation from AUSF");
  std::string remoteUri = nc.get()->Href;

  std::string msgBody        = {};
  nlohmann::json response    = {};
  std::string resStar_string = {};

  std::map<std::string, std::string>::iterator iter;
  iter = rand_record.find(nc.get()->imsi);
  rand_record.erase(iter);
  // convert_string_2_hex(resStar, resStar_string);
  uint8_t resStar_len    = blength(resStar);
  uint8_t* resStar_value = (uint8_t*) bdata(resStar);
  char* resStar_s        = (char*) malloc(resStar_len * 2 + 1);

  for (int i = 0; i < resStar_len; i++) {
    sprintf(&resStar_s[i * 2], "%02X", resStar_value[i]);
  }
  resStar_string = resStar_s;
  comUt::print_buffer("amf_n1", "resStar", resStar_value, resStar_len);
  Logger::amf_n1().info("resStar_s (%s)", resStar_s);

  nlohmann::json confirmationdata_j = {};
  ConfirmationData confirmationdata = {};
  confirmationdata.setResStar(resStar_string);

  to_json(confirmationdata_j, confirmationdata);
  msgBody = confirmationdata_j.dump();

  // TODO: Should be updated
  uint8_t http_version   = 1;
  uint32_t response_code = 0;
  if (amf_cfg.support_features.use_http2) http_version = 2;

  amf_n11_inst->curl_http_client(
      remoteUri, "PUT", msgBody, response, response_code, http_version);

  free_wrapper((void**) &resStar_s);
  try {
    ConfirmationDataResponse confirmationdataresponse;
    response.get_to(confirmationdataresponse);
    unsigned char* kseaf_hex =
        conv::format_string_as_hex(confirmationdataresponse.getKseaf());
    memcpy(nc.get()->_5g_av[0].kseaf, kseaf_hex, 32);
    comUt::print_buffer(
        "amf_n1", "5G AV: kseaf", nc.get()->_5g_av[0].kseaf, 32);
    free_wrapper((void**) &kseaf_hex);

    Logger::amf_n1().debug("Deriving kamf");
    for (int i = 0; i < MAX_5GS_AUTH_VECTORS; i++) {
      Authentication_5gaka::derive_kamf(
          nc.get()->imsi, nc.get()->_5g_av[i].kseaf, nc.get()->kamf[i],
          0x0000);  // second parameter: abba
      comUt::print_buffer("amf_n1", "kamf", nc.get()->kamf[i], 32);
    }
  } catch (nlohmann::json::exception& e) {
    Logger::amf_n1().info("Could not get JSON content from AUSF response");
    // TODO: error handling
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::authentication_vectors_generator_in_ausf(
    std::shared_ptr<nas_context>& nc) {  // A.5, 3gpp ts33.501
  Logger::amf_n1().debug(
      "Generate Authentication Vectors in AUSF");  // Actually, done locally in
                                                   // AMF
  uint8_t inputString[MAX_5GS_AUTH_VECTORS][40];
  uint8_t* xresStar[MAX_5GS_AUTH_VECTORS];
  uint8_t* rand[MAX_5GS_AUTH_VECTORS];
  for (int i = 0; i < MAX_5GS_AUTH_VECTORS; i++) {
    xresStar[i] = nc.get()->_5g_he_av[i].xresStar;
    rand[i]     = nc.get()->_5g_he_av[i].rand;
    memcpy(&inputString[i][0], rand[i], 16);
    memcpy(&inputString[i][16], xresStar[i], 16);
    unsigned char sha256Out[Sha256::DIGEST_SIZE];
    sha256((unsigned char*) inputString[i], 32, sha256Out);
    for (int j = 0; j < 16; j++)
      nc.get()->_5g_av[i].hxresStar[j] = (uint8_t) sha256Out[j];
    memcpy(nc.get()->_5g_av[i].rand, nc.get()->_5g_he_av[i].rand, 16);
    memcpy(nc.get()->_5g_av[i].autn, nc.get()->_5g_he_av[i].autn, 16);
    uint8_t kseaf[32];
    Authentication_5gaka::derive_kseaf(
        nc.get()->serving_network, nc.get()->_5g_he_av[i].kausf, kseaf);
    memcpy(nc.get()->_5g_av[i].kseaf, kseaf, 32);
  }
  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::authentication_vectors_generator_in_udm(
    std::shared_ptr<nas_context>& nc) {
  Logger::amf_n1().debug("Generate Authentication Vectors");
  uint8_t* sqn        = nullptr;
  uint8_t* auts       = (uint8_t*) bdata(nc.get()->auts);
  _5G_HE_AV_t* vector = nc.get()->_5g_he_av;
  // Access to MySQL to fetch UE-related information
  if (!connect_to_mysql()) {
    Logger::amf_n1().error("Cannot connect to MySQL DB");
    return false;
  }
  Logger::amf_n1().debug("Connected to MySQL successfully");
  mysql_auth_info_t mysql_resp = {};
  if (get_mysql_auth_info(nc.get()->imsi, mysql_resp)) {
    if (auts) {
      sqn = Authentication_5gaka::sqn_ms_derive(
          mysql_resp.opc, mysql_resp.key, auts, mysql_resp.rand);
      if (sqn) {
        generate_random(vector[0].rand, RAND_LENGTH);
        mysql_push_rand_sqn(nc.get()->imsi, vector[0].rand, sqn);
        mysql_increment_sqn(nc.get()->imsi);
        free_wrapper((void**) &sqn);
      }
      if (!get_mysql_auth_info(nc.get()->imsi, mysql_resp)) {
        Logger::amf_n1().error("Cannot get data from MySQL");
        return false;
      }
      sqn = mysql_resp.sqn;
      for (int i = 0; i < MAX_5GS_AUTH_VECTORS; i++) {
        generate_random(vector[i].rand, RAND_LENGTH);
        comUt::print_buffer(
            "amf_n1", "Generated random rand (5G HE AV)", vector[i].rand, 16);
        generate_5g_he_av_in_udm(
            mysql_resp.opc, nc.get()->imsi, mysql_resp.key, sqn,
            nc.get()->serving_network,
            vector[i]);  // serving network name
      }
      mysql_push_rand_sqn(
          nc.get()->imsi, vector[MAX_5GS_AUTH_VECTORS - 1].rand, sqn);
    } else {
      Logger::amf_n1().debug("No AUTS ...");
      Logger::amf_n1().debug(
          "Receive information from MySQL with IMSI %s",
          nc.get()->imsi.c_str());
      for (int i = 0; i < MAX_5GS_AUTH_VECTORS; i++) {
        generate_random(vector[i].rand, RAND_LENGTH);
        sqn = mysql_resp.sqn;
        generate_5g_he_av_in_udm(
            mysql_resp.opc, nc.get()->imsi, mysql_resp.key, sqn,
            nc.get()->serving_network,
            vector[i]);  // serving network name
      }
      mysql_push_rand_sqn(
          nc.get()->imsi, vector[MAX_5GS_AUTH_VECTORS - 1].rand, sqn);
    }
    mysql_increment_sqn(nc.get()->imsi);
  } else {
    Logger::amf_n1().error("Failed to fetch user data from MySQL");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void amf_n1::generate_random(uint8_t* random_p, ssize_t length) {
  gmp_randinit_default(random_state.state);
  gmp_randseed_ui(random_state.state, time(NULL));
  if (!amf_cfg.auth_para.random.compare("true")) {
    Logger::amf_n1().debug("AMF config random -> true");
    random_t random_nb;
    mpz_init(random_nb);
    mpz_init_set_ui(random_nb, 0);
    pthread_mutex_lock(&random_state.lock);
    mpz_urandomb(random_nb, random_state.state, 8 * length);
    pthread_mutex_unlock(&random_state.lock);
    mpz_export(random_p, NULL, 1, length, 0, 0, random_nb);
    int r = 0, mask = 0, shift;
    for (int i = 0; i < length; i++) {
      if ((i % sizeof(i)) == 0) r = rand();
      shift       = 8 * (i % sizeof(i));
      mask        = 0xFF << shift;
      random_p[i] = (r & mask) >> shift;
    }
  } else {
    Logger::amf_n1().error("AMF config random -> false");
    pthread_mutex_lock(&random_state.lock);
    for (int i = 0; i < length; i++) {
      random_p[i] = i + no_random_delta;
    }
    no_random_delta += 1;
    pthread_mutex_unlock(&random_state.lock);
  }
}

//------------------------------------------------------------------------------
void amf_n1::generate_5g_he_av_in_udm(
    const uint8_t opc[16], const string& imsi, uint8_t key[16], uint8_t sqn[6],
    std::string& serving_network, _5G_HE_AV_t& vector) {
  Logger::amf_n1().debug("Generate 5g_he_av as in UDM");
  uint8_t amf[] = {0x80, 0x00};
  uint8_t mac_a[8];
  uint8_t ck[16];
  uint8_t ik[16];
  uint8_t ak[6];
  uint64_t _imsi = fromString<uint64_t>(imsi);

  Authentication_5gaka::f1(
      opc, key, vector.rand, sqn, amf,
      mac_a);  // to compute MAC, Figure 7, ts33.102
  // comUt::print_buffer("amf_n1", "Result For F1-Alg: mac_a", mac_a, 8);
  Authentication_5gaka::f2345(
      opc, key, vector.rand, vector.xres, ck, ik,
      ak);  // to compute XRES, CK, IK, AK
  annex_a_4_33501(
      ck, ik, vector.xres, vector.rand, serving_network, vector.xresStar);
  // comUt::print_buffer("amf_n1", "Result For KDF: xres*(5G HE AV)",
  // vector.xresStar, 16);
  Authentication_5gaka::generate_autn(
      sqn, ak, amf, mac_a,
      vector.autn);  // generate AUTN
  // comUt::print_buffer("amf_n1", "Generated autn(5G HE AV)", vector.autn, 16);
  Authentication_5gaka::derive_kausf(
      ck, ik, serving_network, sqn, ak,
      vector.kausf);  // derive Kausf
  // comUt::print_buffer("amf_n1", "Result For KDF: Kausf(5G HE AV)",
  // vector.kausf, 32);
  Logger::amf_n1().debug("Generate_5g_he_av_in_udm finished!");
  return;
}

//------------------------------------------------------------------------------
void amf_n1::annex_a_4_33501(
    uint8_t ck[16], uint8_t ik[16], uint8_t* input, uint8_t rand[16],
    std::string& serving_network, uint8_t* output) {
  OCTET_STRING_t netName;
  OCTET_STRING_fromBuf(
      &netName, serving_network.c_str(), serving_network.length());
  uint8_t S[100];
  S[0] = 0x6B;
  memcpy(&S[1], netName.buf, netName.size);
  S[1 + netName.size] = (netName.size & 0xff00) >> 8;
  S[2 + netName.size] = (netName.size & 0x00ff);
  for (int i = 0; i < 16; i++) S[3 + netName.size + i] = rand[i];
  S[19 + netName.size] = 0x00;
  S[20 + netName.size] = 0x10;
  for (int i = 0; i < 8; i++) S[21 + netName.size + i] = input[i];
  S[29 + netName.size] = 0x00;
  S[30 + netName.size] = 0x08;

  uint8_t plmn[3] = {0x46, 0x0f, 0x11};
  uint8_t oldS[100];
  oldS[0] = 0x6B;
  memcpy(&oldS[1], plmn, 3);
  oldS[4] = 0x00;
  oldS[5] = 0x03;
  for (int i = 0; i < 16; i++) oldS[6 + i] = rand[i];
  oldS[22] = 0x00;
  oldS[23] = 0x10;
  for (int i = 0; i < 8; i++) oldS[24 + i] = input[i];
  oldS[32] = 0x00;
  oldS[33] = 0x08;
  comUt::print_buffer("amf_n1", "Input string: ", S, 31 + netName.size);
  uint8_t key[32];
  memcpy(&key[0], ck, 16);
  memcpy(&key[16], ik, 16);  // KEY
  // Authentication_5gaka::kdf(key, 32, oldS, 33, output, 16);
  uint8_t out[32];
  Authentication_5gaka::kdf(key, 32, S, 31 + netName.size, out, 32);
  for (int i = 0; i < 16; i++) output[i] = out[16 + i];
  comUt::print_buffer("amf_n1", "XRES*(new)", out, 32);
}

//------------------------------------------------------------------------------
void amf_n1::handle_auth_vector_successful_result(
    std::shared_ptr<nas_context>& nc) {
  Logger::amf_n1().debug(
      "Received Security Vectors, try to setup security with the UE");
  nc.get()->is_auth_vectors_present = true;
  ngksi_t ngksi                     = 0;
  if (!nc.get()->security_ctx) {
    nc.get()->security_ctx          = new nas_secu_ctx();
    nc.get()->security_ctx->sc_type = SECURITY_CTX_TYPE_NOT_AVAILABLE;
    if (nc.get()->security_ctx &&
        nc.get()->ngKsi != NAS_KEY_SET_IDENTIFIER_NOT_AVAILABLE)
      ngksi = (nc.get()->amf_ue_ngap_id + 1) % (NGKSI_MAX_VALUE + 1);
    // ensure which vector is available?
    nc.get()->ngKsi = ngksi;
  }
  int vindex = nc.get()->security_ctx->vector_pointer;
  if (!start_authentication_procedure(nc, vindex, nc.get()->ngKsi)) {
    Logger::amf_n1().error("Start Authentication Procedure Failure, reject...");
    Logger::amf_n1().error(
        "Ran_ue_ngap_id " GNB_UE_NGAP_ID_FMT, nc.get()->ran_ue_ngap_id);
    send_registration_reject_msg(
        _5GMM_CAUSE_INVALID_MANDATORY_INFO, nc.get()->ran_ue_ngap_id,
        nc.get()->amf_ue_ngap_id);  // cause?
  } else {
    // update mm state -> COMMON-PROCEDURE-INITIATED
  }
}

//------------------------------------------------------------------------------
bool amf_n1::start_authentication_procedure(
    std::shared_ptr<nas_context>& nc, int vindex, uint8_t ngksi) {
  Logger::amf_n1().debug("Starting Authentication procedure");
  if (check_nas_common_procedure_on_going(nc)) {
    Logger::amf_n1().error("Existed NAS common procedure on going, reject...");
    send_registration_reject_msg(
        _5GMM_CAUSE_INVALID_MANDATORY_INFO, nc.get()->ran_ue_ngap_id,
        nc.get()->amf_ue_ngap_id);  // cause?
    return false;
  }

  nc.get()->is_common_procedure_for_authentication_running = true;
  std::unique_ptr<AuthenticationRequest> auth_request =
      std::make_unique<AuthenticationRequest>();
  auth_request->setHeader(PLAIN_5GS_MSG);
  auth_request->setngKSI(NAS_KEY_SET_IDENTIFIER_NATIVE, ngksi);
  uint8_t abba[2];
  abba[0] = 0x00;
  abba[1] = 0x00;
  auth_request->setABBA(2, abba);
  uint8_t* rand = nc.get()->_5g_av[vindex].rand;
  if (rand) auth_request->setAuthentication_Parameter_RAND(rand);
  Logger::amf_n1().debug("Sending Authentication Request with RAND");
  printf("0x");
  for (int i = 0; i < 16; i++) printf("%x", rand[i]);
  printf("\n");

  uint8_t* autn = nc.get()->_5g_av[vindex].autn;
  if (autn) auth_request->setAuthentication_Parameter_AUTN(autn);
  uint8_t buffer[1024] = {0};
  int encoded_size     = auth_request->encode2buffer(buffer, 1024);
  if (!encoded_size) {
    Logger::nas_mm().error("Encode Authentication Request message error");
    return false;
  }

  bstring b = blk2bstr(buffer, encoded_size);
  comUt::print_buffer(
      "amf_n1", "Authentication-Request message buffer", (uint8_t*) bdata(b),
      blength(b));
  Logger::amf_n1().debug(
      "amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT, nc.get()->amf_ue_ngap_id);
  itti_send_dl_nas_buffer_to_task_n2(
      b, nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id);
  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::check_nas_common_procedure_on_going(
    std::shared_ptr<nas_context>& nc) {
  if (nc.get()->is_common_procedure_for_authentication_running) {
    Logger::amf_n1().debug("Existed Authentication procedure is running");
    return true;
  }
  if (nc.get()->is_common_procedure_for_identification_running) {
    Logger::amf_n1().debug("Existed Identification procedure is running");
    return true;
  }
  if (nc.get()->is_common_procedure_for_security_mode_control_running) {
    Logger::amf_n1().debug("Existed SMC procedure is running");
    return true;
  }
  if (nc.get()->is_common_procedure_for_nas_transport_running) {
    Logger::amf_n1().debug("Existed NAS transport procedure is running");
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void amf_n1::authentication_response_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
    bstring plain_msg) {
  std::shared_ptr<nas_context> nc = {};

  if (!is_amf_ue_id_2_nas_context(amf_ue_ngap_id)) {
    Logger::amf_n1().error(
        "No existed NAS context for UE with amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT,
        amf_ue_ngap_id);
    send_registration_reject_msg(
        _5GMM_CAUSE_ILLEGAL_UE, ran_ue_ngap_id,
        amf_ue_ngap_id);  // cause?
    return;
  }
  nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);
  Logger::amf_n1().info(
      "Found nas_context (%p) with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT ")",
      nc.get(), amf_ue_ngap_id);
  // Stop timer? common procedure finished!
  nc.get()->is_common_procedure_for_authentication_running = false;
  // MM state: COMMON-PROCEDURE-INITIATED -> DEREGISTRED
  // Decode AUTHENTICATION RESPONSE message
  auto auth_response = std::make_unique<AuthenticationResponse>();

  auth_response->decodefrombuffer(
      nullptr, (uint8_t*) bdata(plain_msg), blength(plain_msg));
  bstring resStar;
  bool isAuthOk = true;
  // Get response RES*
  if (!auth_response->getAuthenticationResponseParameter(resStar)) {
    Logger::amf_n1().warn(
        "Cannot receive AuthenticationResponseParameter (RES*)");
  } else {
    if (amf_cfg.support_features.enable_external_ausf) {
      // std::string data = bdata(resStar);
      if (!_5g_aka_confirmation_from_ausf(nc, resStar)) isAuthOk = false;
    } else {
      // Get stored XRES*
      int secu_index = 0;
      if (nc.get()->security_ctx)
        secu_index = nc.get()->security_ctx->vector_pointer;

      uint8_t* hxresStar = nc.get()->_5g_av[secu_index].hxresStar;
      // Calculate HRES* from received RES*, then compare with XRES stored in
      // nas_context
      if (hxresStar) {
        uint8_t inputstring[32];
        uint8_t* res = (uint8_t*) bdata(resStar);
        Logger::amf_n1().debug("Start to calculate HRES* from received RES*");
        memcpy(&inputstring[0], nc.get()->_5g_av[secu_index].rand, 16);
        memcpy(&inputstring[16], res, blength(resStar));
        unsigned char sha256Out[Sha256::DIGEST_SIZE];
        sha256((unsigned char*) inputstring, 16 + blength(resStar), sha256Out);
        uint8_t hres[16];
        for (int i = 0; i < 16; i++) hres[i] = (uint8_t) sha256Out[i];
        comUt::print_buffer(
            "amf_n1", "Received RES* From Authentication-Response", res, 16);
        comUt::print_buffer(
            "amf_n1", "Stored XRES* in 5G HE AV",
            nc.get()->_5g_he_av[secu_index].xresStar, 16);
        comUt::print_buffer(
            "amf_n1", "Stored XRES in 5G HE AV",
            nc.get()->_5g_he_av[secu_index].xres, 8);
        comUt::print_buffer("amf_n1", "Computed HRES* from RES*", hres, 16);
        comUt::print_buffer(
            "amf_n1", "Computed HXRES* from XRES*", hxresStar, 16);
        for (int i = 0; i < 16; i++) {
          if (hxresStar[i] != hres[i]) isAuthOk = false;
        }
      } else {
        isAuthOk = false;
      }
    }
  }
  // If success, start SMC procedure; else if failure, response registration
  // reject message with corresponding cause
  if (!isAuthOk) {
    Logger::amf_n1().error(
        "Authentication failed for UE with amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT,
        amf_ue_ngap_id);
    send_registration_reject_msg(
        _5GMM_CAUSE_ILLEGAL_UE, ran_ue_ngap_id,
        amf_ue_ngap_id);  // cause?
    return;
  } else {
    Logger::amf_n1().debug("Authentication successful by network!");
    // Fix to work with ng4T
    // TODO: To verify UE/AMF behavior according to 3GPP TS 24.501
    // if (!nc.get()->is_current_security_available) {
    if (!start_security_mode_control_procedure(nc)) {
      Logger::amf_n1().error("Start SMC procedure failure");
    }
  }
}

//------------------------------------------------------------------------------
void amf_n1::authentication_failure_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
    bstring plain_msg) {
  std::shared_ptr<nas_context> nc = {};
  if (!is_amf_ue_id_2_nas_context(amf_ue_ngap_id)) {
    Logger::amf_n1().error(
        "No existed NAS context for UE with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT
        ")",
        amf_ue_ngap_id);
    send_registration_reject_msg(
        _5GMM_CAUSE_ILLEGAL_UE, ran_ue_ngap_id,
        amf_ue_ngap_id);  // cause?
    return;
  }
  nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);
  nc.get()->is_common_procedure_for_authentication_running = false;
  // 1. decode AUTHENTICATION FAILURE message
  auto auth_failure = std::make_unique<AuthenticationFailure>();

  auth_failure->decodefrombuffer(
      NULL, (uint8_t*) bdata(plain_msg), blength(plain_msg));
  uint8_t mm_cause = auth_failure->get5GMmCause();
  if (mm_cause == -1) {
    Logger::amf_n1().error("Missing mandatory IE 5G_MM_CAUSE");
    send_registration_reject_msg(
        _5GMM_CAUSE_INVALID_MANDATORY_INFO, ran_ue_ngap_id,
        amf_ue_ngap_id);  // cause?
    return;
  }
  switch (mm_cause) {
    case _5GMM_CAUSE_SYNCH_FAILURE: {
      Logger::amf_n1().debug("Initial new Authentication procedure");
      bstring auts;
      if (!auth_failure->getAutsInAuthFailPara(auts)) {
        Logger::amf_n1().warn(
            "IE Authentication Failure Parameter (AUTS) not received");
      }
      nc.get()->auts = auts;
      printf("Received AUTS: 0x ");
      for (int i = 0; i < blength(auts); i++)
        printf("%x ", ((uint8_t*) bdata(auts))[i]);
      printf("\n");
      if (auth_vectors_generator(nc)) {  // all authentication in one(AMF)
        handle_auth_vector_successful_result(nc);
      } else {
        Logger::amf_n1().error("Request Authentication Vectors failure");
        send_registration_reject_msg(
            _5GMM_CAUSE_ILLEGAL_UE, nc.get()->ran_ue_ngap_id,
            nc.get()->amf_ue_ngap_id);  // cause?
      }
      // authentication_failure_synch_failure_handle(nc, auts);
    } break;
    case _5GMM_CAUSE_NGKSI_ALREADY_IN_USE: {
      Logger::amf_n1().debug(
          "ngKSI already in use, select a new ngKSI and restart the "
          "Authentication procedure!");
      // select new ngKSI and resend Authentication Request
      ngksi_t ngksi =
          (nc.get()->ngKsi + 1) % (NGKSI_MAX_VALUE + 1);  // To be verified
      nc.get()->ngKsi = ngksi;
      int vindex      = nc.get()->security_ctx->vector_pointer;
      if (!start_authentication_procedure(nc, vindex, nc.get()->ngKsi)) {
        Logger::amf_n1().error(
            "Start Authentication procedure failure, reject...");
        Logger::amf_n1().error(
            "Ran_ue_ngap_id " GNB_UE_NGAP_ID_FMT, nc.get()->ran_ue_ngap_id);
        send_registration_reject_msg(
            _5GMM_CAUSE_INVALID_MANDATORY_INFO, nc.get()->ran_ue_ngap_id,
            nc.get()->amf_ue_ngap_id);
      } else {
        // update mm state -> COMMON-PROCEDURE-INITIATED
      }

    } break;
  }
}

//------------------------------------------------------------------------------
bool amf_n1::start_security_mode_control_procedure(
    std::shared_ptr<nas_context>& nc) {
  Logger::amf_n1().debug("Start Security Mode Control procedure");
  nc.get()->is_common_procedure_for_security_mode_control_running = true;
  bool security_context_is_new                                    = false;
  uint8_t amf_nea                                                 = EA0_5G;
  uint8_t amf_nia                                                 = IA0_5G;
  // decide which ea/ia alg used by UE, which is supported by network
  security_data_t* data = (security_data_t*) calloc(1, sizeof(security_data_t));
  nas_secu_ctx* secu_ctx = nc.get()->security_ctx;
  if (!data) {
    Logger::amf_n1().error("Cannot allocate memory for security_data_t");
    return false;
  }
  if (!secu_ctx) {
    Logger::amf_n1().error("No Security Context found");
    free_wrapper((void**) &data);
    return false;
  }

  if (secu_ctx->sc_type == SECURITY_CTX_TYPE_NOT_AVAILABLE &&
      nc.get()->is_common_procedure_for_security_mode_control_running) {
    Logger::amf_n1().debug(
        "Using INTEGRITY_PROTECTED_WITH_NEW_SECU_CTX for SecurityModeControl "
        "message");
    data->saved_selected_nea =
        secu_ctx->nas_algs
            .encryption;  // emm_ctx->_security.selected_algorithms.encryption;
    data->saved_selected_nia = secu_ctx->nas_algs.integrity;
    data->saved_ngksi        = secu_ctx->ngksi;
    data->saved_overflow =
        secu_ctx->dl_count.overflow;  // emm_ctx->_security.dl_count.overflow;
    data->saved_seq_num         = secu_ctx->dl_count.seq_num;
    data->saved_sc_type         = secu_ctx->sc_type;
    secu_ctx->ngksi             = nc.get()->ngKsi;
    secu_ctx->dl_count.overflow = 0;
    secu_ctx->dl_count.seq_num  = 0;
    secu_ctx->ul_count.overflow = 0;
    secu_ctx->ul_count.seq_num  = 0;
    security_select_algorithms(
        nc.get()->ueSecurityCapEnc, nc.get()->ueSecurityCapInt, amf_nea,
        amf_nia);
    secu_ctx->nas_algs.integrity  = amf_nia;
    secu_ctx->nas_algs.encryption = amf_nea;
    secu_ctx->sc_type             = SECURITY_CTX_TYPE_FULL_NATIVE;
    Authentication_5gaka::derive_knas(
        NAS_INT_ALG, secu_ctx->nas_algs.integrity,
        nc.get()->kamf[secu_ctx->vector_pointer], secu_ctx->knas_int);
    Authentication_5gaka::derive_knas(
        NAS_ENC_ALG, secu_ctx->nas_algs.encryption,
        nc.get()->kamf[secu_ctx->vector_pointer], secu_ctx->knas_enc);
    security_context_is_new                 = true;
    nc.get()->is_current_security_available = true;
  }

  std::unique_ptr<SecurityModeCommand> smc =
      std::make_unique<SecurityModeCommand>();
  smc->setHeader(PLAIN_5GS_MSG);
  smc->setNAS_Security_Algorithms(amf_nea, amf_nia);
  Logger::amf_n1().debug("Encoded ngKSI 0x%x", nc.get()->ngKsi);
  smc->setngKSI(NAS_KEY_SET_IDENTIFIER_NATIVE, nc.get()->ngKsi & 0x07);
  if (nc.get()->ueSecurityCaplen >= 4) {
    smc->setUE_Security_Capability(
        nc.get()->ueSecurityCapEnc, nc.get()->ueSecurityCapInt,
        nc.get()->ueSecurityCapEEA, nc.get()->ueSecurityCapEIA);
  } else {
    smc->setUE_Security_Capability(
        nc.get()->ueSecurityCapEnc, nc.get()->ueSecurityCapInt);
  }

  if (smc->ie_ue_security_capability != NULL) {
    smc->ie_ue_security_capability->setLength(nc.get()->ueSecurityCaplen);
  } else {
    Logger::amf_n1().error("UE Security Capability is missing");
  }

  smc->setIMEISV_Request(0xe1);  // TODO: remove hardcoded value
  smc->setAdditional_5G_Security_Information(true, false);
  uint8_t buffer[BUFFER_SIZE_1024];
  int encoded_size = smc->encode2buffer(buffer, BUFFER_SIZE_1024);
  comUt::print_buffer(
      "amf_n1", "Security-Mode-Command message buffer", buffer, encoded_size);

  std::string str = security_context_is_new ? "true" : "false";
  Logger::amf_n1().debug("Security Context status (is new:  %s)", str.c_str());

  bstring intProtctedNas;
  encode_nas_message_protected(
      secu_ctx, security_context_is_new, INTEGRITY_PROTECTED_WITH_NEW_SECU_CTX,
      NAS_MESSAGE_DOWNLINK, buffer, encoded_size, intProtctedNas);
  comUt::print_buffer(
      "amf_n1", "Encrypted Security-Mode-Command message buffer",
      (uint8_t*) bdata(intProtctedNas), blength(intProtctedNas));
  itti_send_dl_nas_buffer_to_task_n2(
      intProtctedNas, nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id);
  // secu_ctx->dl_count.seq_num ++;
  free_wrapper((void**) &data);
  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::security_select_algorithms(
    uint8_t nea, uint8_t nia, uint8_t& amf_nea, uint8_t& amf_nia) {
  bool found_nea = false;
  bool found_nia = false;
  for (int i = 0; i < 8; i++) {
    if (nea & (0x80 >> amf_cfg.nas_cfg.prefered_ciphering_algorithm[i])) {
      amf_nea = amf_cfg.nas_cfg.prefered_ciphering_algorithm[i];
      printf("amf_nea: 0x%x\n", amf_nea);
      found_nea = true;
      break;
    }
  }
  for (int i = 0; i < 8; i++) {
    if (nia & (0x80 >> amf_cfg.nas_cfg.prefered_integrity_algorithm[i])) {
      amf_nia = amf_cfg.nas_cfg.prefered_integrity_algorithm[i];
      printf("amf_nia: 0x%x\n", amf_nia);
      found_nia = true;
      break;
    }
  }
  return (found_nea && found_nia);
}

//------------------------------------------------------------------------------
void amf_n1::security_mode_complete_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id, bstring nas_msg) {
  Logger::amf_n1().debug("Handling Security Mode Complete ...");

  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(ran_ue_ngap_id, amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  std::shared_ptr<nas_context> nc = {};
  if (is_amf_ue_id_2_nas_context(amf_ue_ngap_id))
    nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);
  else {
    Logger::amf_n1().warn(
        "No existed nas_context with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT ")",
        amf_ue_ngap_id);
    return;
  }
  // Decode Security Mode Complete
  auto security_mode_complete = std::make_unique<SecurityModeComplete>();
  security_mode_complete->decodefrombuffer(
      nullptr, (uint8_t*) bdata(nas_msg), blength(nas_msg));

  comUt::print_buffer(
      "amf_n1", "Security Mode Complete message buffer",
      (uint8_t*) bdata(nas_msg), blength(nas_msg));

  bstring nas_msg_container;
  if (security_mode_complete->getNasMessageContainer(nas_msg_container)) {
    comUt::print_buffer(
        "amf_n1", "NAS Message Container", (uint8_t*) bdata(nas_msg_container),
        blength(nas_msg_container));

    uint8_t* buf_nas     = (uint8_t*) bdata(nas_msg_container);
    uint8_t message_type = *(buf_nas + 2);
    Logger::amf_n1().debug(
        "NAS Message Container, Message Type 0x%x", message_type);
    if (message_type == REGISTRATION_REQUEST) {
      Logger::amf_n1().debug("Registration Request in NAS Message Container");
      // Decode registration request message
      std::unique_ptr<RegistrationRequest> registration_request =
          std::make_unique<RegistrationRequest>();
      registration_request->decodefrombuffer(
          nullptr, (uint8_t*) bdata(nas_msg_container),
          blength(nas_msg_container));
      bdestroy(nas_msg_container);  // free buffer

      // Get Requested NSSAI (Optional IE), if provided
      if (registration_request->getRequestedNssai(nc.get()->requestedNssai)) {
        for (auto s : nc.get()->requestedNssai) {
          Logger::amf_n1().debug(
              "Requested NSSAI SST (0x%x) SD (0x%x) hplmnSST (0x%x) hplmnSD "
              "(%d)",
              s.sst, s.sd, s.mHplmnSst, s.mHplmnSd);
        }
      } else {
        Logger::amf_n1().debug("No Optional IE RequestedNssai available");
      }
    }
  }

  // If current AMF can't handle this UE, reroute the Registration Request to
  // a target AMF
  bool reroute_result = true;
  if (reroute_registration_request(nc, reroute_result)) {
    return;
  }

  // If AMF can't handle this and there's an error when trying to handling the
  // UE to the target AMFs, thus encoding REGISTRATION REJECT
  if (!reroute_result) {
    uint8_t cause_value = 7;  // 5GS services not allowed - TO BE VERIFIED
    send_registration_reject_msg(cause_value, ran_ue_ngap_id, amf_ue_ngap_id);
    return;
  }

  // Otherwise encoding REGISTRATION ACCEPT
  auto registration_accept = std::make_unique<RegistrationAccept>();
  initialize_registration_accept(registration_accept, nc);

  std::string mcc = {};
  std::string mnc = {};
  uint32_t tmsi   = 0;
  if (!amf_app_inst->generate_5g_guti(
          ran_ue_ngap_id, amf_ue_ngap_id, mcc, mnc, tmsi)) {
    Logger::amf_n1().error("Generate 5G GUTI error, exit!");
    // TODO:
    return;
  }
  registration_accept->set5G_GUTI(
      mcc, mnc, amf_cfg.guami.regionID, amf_cfg.guami.AmfSetID,
      amf_cfg.guami.AmfPointer, tmsi);

  std::string guti = mcc + mnc + amf_cfg.guami.regionID +
                     amf_cfg.guami.AmfSetID + amf_cfg.guami.AmfPointer +
                     conv::tmsi_to_string(tmsi);
  Logger::amf_n1().debug("Allocated GUTI %s", guti.c_str());

  // TODO: remove hardcoded values
  registration_accept->set_5GS_Network_Feature_Support(0x01, 0x00);
  // registration_accept->setT3512_Value(0x5, T3512_TIMER_VALUE_MIN);
  uint8_t buffer[BUFFER_SIZE_1024] = {0};
  int encoded_size =
      registration_accept->encode2buffer(buffer, BUFFER_SIZE_1024);
  comUt::print_buffer(
      "amf_n1", "Registration-Accept message buffer", buffer, encoded_size);
  if (!encoded_size) {
    Logger::nas_mm().error("Encode Registration-Accept message error");
    return;
  }

  Logger::amf_n1().info(
      "UE (IMSI %s, GUTI %s, current RAN ID %d, current AMF ID %d) has been "
      "registered to the network",
      nc.get()->imsi.c_str(), guti.c_str(), ran_ue_ngap_id, amf_ue_ngap_id);
  if (nc.get()->is_stacs_available) {
    stacs.update_5gmm_state(nc.get()->imsi, "5GMM-REGISTERED");
  } else {
    nc.get()->is_stacs_available = true;
  }
  set_5gmm_state(nc, _5GMM_REGISTERED);
  stacs.display();

  string supi = "imsi-" + nc.get()->imsi;
  Logger::amf_n1().debug(
      "Signal the UE Registration State Event notification for SUPI %s",
      supi.c_str());
  event_sub.ue_registration_state(supi, _5GMM_REGISTERED, 1);

  set_guti_2_nas_context(guti, nc);
  nc.get()->is_common_procedure_for_security_mode_control_running = false;
  nas_secu_ctx* secu = nc.get()->security_ctx;
  if (!secu) {
    Logger::amf_n1().error("No Security Context found");
    return;
  }

  bstring protectedNas;
  encode_nas_message_protected(
      secu, false, INTEGRITY_PROTECTED_AND_CIPHERED, NAS_MESSAGE_DOWNLINK,
      buffer, encoded_size, protectedNas);

  if (!uc.get()->isUeContextRequest) {
    Logger::amf_n1().debug(
        "UE Context is not requested, UE with "
        "ran_ue_ngap_id " GNB_UE_NGAP_ID_FMT
        ", "
        "amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT " attached",
        ran_ue_ngap_id, amf_ue_ngap_id);

    // TODO: Use DownlinkNasTransport to convey Registration Accept
    // IE: UEAggregateMaximumBitRate
    // AllowedNSSAI

    std::shared_ptr<itti_dl_nas_transport> dnt =
        std::make_shared<itti_dl_nas_transport>(TASK_AMF_N1, TASK_AMF_N2);
    dnt->nas            = protectedNas;
    dnt->amf_ue_ngap_id = amf_ue_ngap_id;
    dnt->ran_ue_ngap_id = ran_ue_ngap_id;

    int ret = itti_inst->send_msg(dnt);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N2",
          dnt->get_msg_name());
    }

  } else {
    // use InitialContextSetupRequest (NGAP message) to convey Registration
    // Accept

    uint8_t* kamf = nc.get()->kamf[secu->vector_pointer];
    uint8_t kgnb[32];
    uint32_t ulcount = secu->ul_count.seq_num | (secu->ul_count.overflow << 8);
    Authentication_5gaka::derive_kgnb(0, 0x01, kamf, kgnb);
    comUt::print_buffer("amf_n1", "kamf", kamf, 32);
    // Authentication_5gaka::derive_kgnb(ulcount, 0x01, kamf, kgnb);
    bstring kgnb_bs = blk2bstr(kgnb, 32);

    std::shared_ptr<itti_initial_context_setup_request> itti_msg =
        std::make_shared<itti_initial_context_setup_request>(
            TASK_AMF_N1, TASK_AMF_N2);
    itti_msg->ran_ue_ngap_id = ran_ue_ngap_id;
    itti_msg->amf_ue_ngap_id = amf_ue_ngap_id;
    itti_msg->kgnb           = kgnb_bs;
    itti_msg->nas            = protectedNas;
    itti_msg->is_pdu_exist   = false;  // no pdu context
    itti_msg->is_sr          = false;  // TODO: for Service Request procedure

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N2",
          itti_msg->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_n1::security_mode_reject_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id, bstring nas_msg) {
  Logger::amf_n1().debug(
      "Receiving Security Mode Reject message, handling ...");
  // TODO:
  return;
}

//------------------------------------------------------------------------------
void amf_n1::registration_complete_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id, bstring nas_msg) {
  Logger::amf_n1().debug(
      "Receiving Registration Complete, encoding Configuration Update Command");
  // TODO:
  /*
    time_t tt;
    time(&tt);
    tt    = tt + 8 * 3600;  // transform the time zone
    tm* t = gmtime(&tt);

    uint8_t conf[45]       = {0};
    uint8_t header[3]      = {0x7e, 0x00, 0x54};
    uint8_t full_name[18]  = {0x43, 0x10, 0x81, 0xc1, 0x76, 0x58,
                             0x9e, 0x9e, 0xbf, 0xcd, 0x74, 0x90,
                             0xb3, 0x4c, 0xbf, 0xbf, 0xe5, 0x6b};
    uint8_t short_name[11] = {0x45, 0x09, 0x81, 0xc1, 0x76, 0x58,
                              0x9e, 0x9e, 0xbf, 0xcd, 0x74};
    uint8_t time_zone[2]   = {0x46, 0x23};
    uint8_t time[8]        = {0};
    time[0]                = 0x47;
    time[1]                = 0x12;
    time[2] = ((t->tm_mon + 1) & 0x0f) << 4 | ((t->tm_mon + 1) & 0xf0) >> 4;
    time[3] = ((t->tm_mday + 1) & 0x0f) << 4 | ((t->tm_mday + 1) & 0xf0) >> 4;
    time[4] = ((t->tm_hour + 1) & 0x0f) << 4 | ((t->tm_hour + 1) & 0xf0) >> 4;
    time[5] = ((t->tm_min + 1) & 0x0f) << 4 | ((t->tm_min + 1) & 0xf0) >> 4;
    time[6] = ((t->tm_sec + 1) & 0x0f) << 4 | ((t->tm_sec + 1) & 0xf0) >> 4;
    time[7] = 0x23;
    uint8_t daylight[3] = {0x49, 0x01, 0x00};
    memcpy(conf, header, 3);
    memcpy(conf + 3, full_name, 18);
    memcpy(conf + 21, short_name, 11);
    memcpy(conf + 32, time_zone, 2);
    memcpy(conf + 34, time, 8);
    memcpy(conf + 42, daylight, 3);

    std::shared_ptr<nas_context> nc;
    if (is_amf_ue_id_2_nas_context(amf_ue_ngap_id))
      nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);
    else {
      Logger::amf_n1().warn(
          "No existed nas_context with amf_ue_ngap_id(" AMF_UE_NGAP_ID_FMT ")",
    amf_ue_ngap_id); return;
    }
    nas_secu_ctx* secu = nc.get()->security_ctx;
    // protect nas message
    bstring protectedNas;
    encode_nas_message_protected(
        secu, false, INTEGRITY_PROTECTED_AND_CIPHERED, NAS_MESSAGE_DOWNLINK,
    conf, 45, protectedNas);

    itti_send_dl_nas_buffer_to_task_n2(
        protectedNas, ran_ue_ngap_id, amf_ue_ngap_id);
        */
}

//------------------------------------------------------------------------------
void amf_n1::encode_nas_message_protected(
    nas_secu_ctx* nsc, bool is_secu_ctx_new, uint8_t security_header_type,
    uint8_t direction, uint8_t* input_nas_buf, int input_nas_len,
    bstring& protected_nas) {
  Logger::amf_n1().debug("Encoding nas_message_protected...");
  uint8_t protected_nas_buf[1024];
  int encoded_size = 0;

  switch (security_header_type & 0x0f) {
    case INTEGRITY_PROTECTED: {
    } break;

    case INTEGRITY_PROTECTED_AND_CIPHERED: {
      bstring input = blk2bstr(input_nas_buf, input_nas_len);
      bstring ciphered;
      // balloc(ciphered, blength(input));
      nas_message_cipher_protected(nsc, NAS_MESSAGE_DOWNLINK, input, ciphered);
      protected_nas_buf[0] = EPD_5GS_MM_MSG;
      protected_nas_buf[1] = INTEGRITY_PROTECTED_AND_CIPHERED;
      protected_nas_buf[6] = (uint8_t) nsc->dl_count.seq_num;

      uint8_t* buf_tmp = (uint8_t*) bdata(ciphered);
      if (buf_tmp != nullptr)
        memcpy(&protected_nas_buf[7], (uint8_t*) buf_tmp, blength(ciphered));

      uint32_t mac32 = 0;
      if (!(nas_message_integrity_protected(
              nsc, NAS_MESSAGE_DOWNLINK, protected_nas_buf + 6,
              input_nas_len + 1, mac32))) {
        memcpy(protected_nas_buf, input_nas_buf, input_nas_len);
        encoded_size = input_nas_len;
      } else {
        *(uint32_t*) (protected_nas_buf + 2) = htonl(mac32);
        encoded_size                         = 7 + input_nas_len;
      }
    } break;

    case INTEGRITY_PROTECTED_WITH_NEW_SECU_CTX: {
      if ((nsc == nullptr) || !is_secu_ctx_new) {
        Logger::amf_n1().error("Security context is too old");
        return;
      }
      protected_nas_buf[0] = EPD_5GS_MM_MSG;
      protected_nas_buf[1] = INTEGRITY_PROTECTED_WITH_NEW_SECU_CTX;
      protected_nas_buf[6] = (uint8_t) nsc->dl_count.seq_num;
      memcpy(&protected_nas_buf[7], input_nas_buf, input_nas_len);
      uint32_t mac32;
      if (!(nas_message_integrity_protected(
              nsc, NAS_MESSAGE_DOWNLINK, protected_nas_buf + 6,
              input_nas_len + 1, mac32))) {
        memcpy(protected_nas_buf, input_nas_buf, input_nas_len);
        encoded_size = input_nas_len;
      } else {
        Logger::amf_n1().debug("mac32: 0x%x", mac32);
        *(uint32_t*) (protected_nas_buf + 2) = htonl(mac32);
        encoded_size                         = 7 + input_nas_len;
      }
    } break;

    case INTEGRITY_PROTECTED_AND_CIPHERED_WITH_NEW_SECU_CTX: {
    } break;
  }
  protected_nas = blk2bstr(protected_nas_buf, encoded_size);
  nsc->dl_count.seq_num++;
}

//------------------------------------------------------------------------------
bool amf_n1::nas_message_integrity_protected(
    nas_secu_ctx* nsc, uint8_t direction, uint8_t* input_nas, int input_nas_len,
    uint32_t& mac32) {
  if (nsc == nullptr) return false;
  uint32_t count = 0x00000000;
  if (direction) {
    count = 0x00000000 | ((nsc->dl_count.overflow & 0x0000ffff) << 8) |
            ((nsc->dl_count.seq_num & 0x000000ff));
  } else {
    count = 0x00000000 | ((nsc->ul_count.overflow & 0x0000ffff) << 8) |
            ((nsc->ul_count.seq_num & 0x000000ff));
  }
  nas_stream_cipher_t stream_cipher = {0};
  uint8_t mac[4];
  stream_cipher.key = nsc->knas_int;
  comUt::print_buffer(
      "amf_n1", "Parameters for NIA: knas_int", nsc->knas_int,
      AUTH_KNAS_INT_SIZE);
  stream_cipher.key_length = AUTH_KNAS_INT_SIZE;
  stream_cipher.count      = *(input_nas);
  // stream_cipher.count = count;
  if (!direction) {
    nsc->ul_count.seq_num = stream_cipher.count;
    Logger::amf_n1().debug("Uplink count in uplink: %d", nsc->ul_count.seq_num);
  }
  Logger::amf_n1().debug("Parameters for NIA, count: 0x%x", count);
  stream_cipher.bearer = 0x01;  // 33.501 section 8.1.1
  Logger::amf_n1().debug(
      "Parameters for NIA, bearer: 0x%x", stream_cipher.bearer);
  stream_cipher.direction = direction;  // "1" for downlink
  Logger::amf_n1().debug("Parameters for NIA, direction: 0x%x", direction);
  stream_cipher.message = (uint8_t*) input_nas;
  comUt::print_buffer(
      "amf_n1", "Parameters for NIA, message: ", input_nas, input_nas_len);
  stream_cipher.blength = input_nas_len * 8;

  switch (nsc->nas_algs.integrity & 0x0f) {
    case IA0_5G: {
      Logger::amf_n1().debug("Integrity with algorithms: 5G-IA0");
      return false;  // plain msg
    } break;

    case IA1_128_5G: {
      Logger::amf_n1().debug("Integrity with algorithms: 128-5G-IA1");
      nas_algorithms::nas_stream_encrypt_nia1(&stream_cipher, mac);
      comUt::print_buffer("amf_n1", "Result for NIA1, mac: ", mac, 4);
      mac32 = ntohl(*((uint32_t*) mac));
      Logger::amf_n1().debug("Result for NIA1, mac32: 0x%x", mac32);
      return true;
    } break;

    case IA2_128_5G: {
      Logger::amf_n1().debug("Integrity with algorithms: 128-5G-IA2");
      nas_algorithms::nas_stream_encrypt_nia2(&stream_cipher, mac);
      comUt::print_buffer("amf_n1", "Result for NIA2, mac: ", mac, 4);
      mac32 = ntohl(*((uint32_t*) mac));
      Logger::amf_n1().debug("Result for NIA2, mac32: 0x%x", mac32);
      return true;
    } break;
  }
  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::nas_message_cipher_protected(
    nas_secu_ctx* nsc, uint8_t direction, bstring input_nas,
    bstring& output_nas) {
  uint8_t* buf   = (uint8_t*) bdata(input_nas);
  int buf_len    = blength(input_nas);
  uint32_t count = 0x00000000;
  if (direction) {
    count = 0x00000000 | ((nsc->dl_count.overflow & 0x0000ffff) << 8) |
            ((nsc->dl_count.seq_num & 0x000000ff));
  } else {
    Logger::amf_n1().debug("nsc->ul_count.overflow %x", nsc->ul_count.overflow);
    count = 0x00000000 | ((nsc->ul_count.overflow & 0x0000ffff) << 8) |
            ((nsc->ul_count.seq_num & 0x000000ff));
  }
  nas_stream_cipher_t stream_cipher = {0};
  uint8_t mac[4];
  stream_cipher.key        = nsc->knas_enc;
  stream_cipher.key_length = AUTH_KNAS_ENC_SIZE;
  stream_cipher.count      = count;
  stream_cipher.bearer     = 0x01;       // 33.501 section 8.1.1
  stream_cipher.direction  = direction;  // "1" for downlink
  stream_cipher.message    = (uint8_t*) bdata(input_nas);
  stream_cipher.blength    = blength(input_nas) << 3;

  switch (nsc->nas_algs.encryption & 0x0f) {
    case EA0_5G: {
      Logger::amf_n1().debug("Cipher protected with EA0_5G");
      output_nas = blk2bstr(buf, buf_len);
      return true;
    } break;

    case EA1_128_5G: {
      Logger::amf_n1().debug("Cipher protected with EA1_128_5G");
      Logger::amf_n1().debug("stream_cipher.blength %d", stream_cipher.blength);
      Logger::amf_n1().debug(
          "stream_cipher.message %x", stream_cipher.message[0]);
      comUt::print_buffer(
          "amf_n1", "stream_cipher.key ", stream_cipher.key, 16);
      Logger::amf_n1().debug("stream_cipher.count %x", stream_cipher.count);

      uint8_t* ciphered =
          (uint8_t*) malloc(((stream_cipher.blength + 31) / 32) * 4);

      nas_algorithms::nas_stream_encrypt_nea1(&stream_cipher, ciphered);
      output_nas = blk2bstr(ciphered, ((stream_cipher.blength + 31) / 32) * 4);
      free(ciphered);
    } break;

    case EA2_128_5G: {
      Logger::amf_n1().debug("Cipher protected with EA2_128_5G");

      uint32_t len = stream_cipher.blength >> 3;
      if ((stream_cipher.blength & 0x7) > 0) len += 1;
      uint8_t* ciphered = (uint8_t*) malloc(len);
      nas_algorithms::nas_stream_encrypt_nea2(&stream_cipher, ciphered);
      output_nas = blk2bstr(ciphered, len);
      free(ciphered);
    } break;
  }
  return true;
}

//------------------------------------------------------------------------------
void amf_n1::ue_initiate_de_registration_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id, bstring nas) {
  Logger::amf_n1().debug("Handling UE-initiated De-registration Request");

  std::shared_ptr<nas_context> nc = {};
  if (is_amf_ue_id_2_nas_context(amf_ue_ngap_id))
    nc = amf_ue_id_2_nas_context(amf_ue_ngap_id);
  else {
    Logger::amf_n1().warn(
        "No existed nas_context with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT ")",
        amf_ue_ngap_id);
    return;
  }

  // Decode NAS message
  auto dereg_request = std::make_unique<DeregistrationRequest>();
  dereg_request->decodefrombuffer(NULL, (uint8_t*) bdata(nas), blength(nas));

  // TODO: validate 5G Mobile Identity
  uint8_t mobile_id_type = 0;
  dereg_request->getMobilityIdentityType(mobile_id_type);
  Logger::amf_n1().debug("5G Mobile Identity %X", mobile_id_type);
  switch (mobile_id_type) {
    case _5G_GUTI: {
      Logger::amf_n1().debug(
          "5G Mobile Identity, GUTI %s", dereg_request->get_5g_guti().c_str());
    } break;
    default: {
    }
  }

  // Send request to SMF to release the established PDU sessions if needed
  // Get list of PDU sessions
  std::vector<std::shared_ptr<pdu_session_context>> sessions_ctx;
  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(nc, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  if (uc.get() != nullptr) {
    if (uc->get_pdu_sessions_context(sessions_ctx)) {
      // Send Nsmf_PDUSession_ReleaseSMContext to SMF to release the PDU session

      std::map<uint32_t, boost::shared_future<uint32_t>> smf_responses;
      for (auto session : sessions_ctx) {
        std::shared_ptr<itti_nsmf_pdusession_release_sm_context> itti_msg =
            std::make_shared<itti_nsmf_pdusession_release_sm_context>(
                TASK_AMF_N1, TASK_AMF_N11);

        // Generate a promise and associate this promise to the ITTI message
        uint32_t promise_id = amf_app_inst->generate_promise_id();
        Logger::amf_n1().debug("Promise ID generated %d", promise_id);

        boost::shared_ptr<boost::promise<uint32_t>> p =
            boost::make_shared<boost::promise<uint32_t>>();
        boost::shared_future<uint32_t> f = p->get_future();

        // Store the future to be processed later
        smf_responses.emplace(promise_id, f);
        amf_app_inst->add_promise(promise_id, p);

        itti_msg->supi             = uc->supi;
        itti_msg->pdu_session_id   = session->pdu_session_id;
        itti_msg->promise_id       = promise_id;
        itti_msg->context_location = session->smf_context_location;

        int ret = itti_inst->send_msg(itti_msg);
        if (0 != ret) {
          Logger::amf_n1().error(
              "Could not send ITTI message %s to task TASK_AMF_N11",
              itti_msg->get_msg_name());
        }
      }

      // Wait for the response from SMF
      while (!smf_responses.empty()) {
        boost::future_status status;
        // wait for timeout or ready
        status = smf_responses.begin()->second.wait_for(
            boost::chrono::milliseconds(FUTURE_STATUS_TIMEOUT_MS));
        if (status == boost::future_status::ready) {
          assert(smf_responses.begin()->second.is_ready());
          assert(smf_responses.begin()->second.has_value());
          assert(!smf_responses.begin()->second.has_exception());
          // Wait for the result from APP and send reply to AMF
          uint32_t http_response_code = smf_responses.begin()->second.get();
          // TODO: process response code
        }
        smf_responses.erase(smf_responses.begin());
      }

    } else {
      Logger::amf_n1().debug("No PDU session available");
    }
  }

  // Check Deregistration type
  uint8_t deregType = 0;
  dereg_request->getDeregistrationType(deregType);
  Logger::amf_n1().debug("De-registration Type 0x%x", deregType);

  // If UE switch-off, don't need to send Deregistration Accept
  if ((deregType & 0b00001000) == 0) {
    // Prepare DeregistrationAccept
    auto dereg_accept = std::make_unique<DeregistrationAccept>();
    dereg_accept->setHeader(PLAIN_5GS_MSG);

    uint8_t buffer[BUFFER_SIZE_512] = {0};
    int encoded_size = dereg_accept->encode2buffer(buffer, BUFFER_SIZE_512);

    comUt::print_buffer(
        "amf_n1", "De-registration Accept message buffer", buffer,
        encoded_size);
    if (encoded_size < 1) {
      Logger::nas_mm().error("Encode De-registration Accept message error!");
      return;
    }

    bstring b = blk2bstr(buffer, encoded_size);
    itti_send_dl_nas_buffer_to_task_n2(b, ran_ue_ngap_id, amf_ue_ngap_id);
  }

  set_5gmm_state(nc, _5GMM_DEREGISTERED);
  stacs.display();

  string supi = "imsi-" + nc.get()->imsi;
  Logger::amf_n1().debug(
      "Signal the UE Registration State Event notification for SUPI %s",
      supi.c_str());
  event_sub.ue_registration_state(supi, _5GMM_DEREGISTERED, 1);

  if (nc.get()->is_stacs_available) {
    stacs.update_5gmm_state(nc.get()->imsi, "5GMM-DEREGISTERED");
  }

  // Remove NC context
  if (remove_amf_ue_ngap_id_2_nas_context(amf_ue_ngap_id)) {
    Logger::amf_n1().debug(
        "Deleted nas_context associated with "
        "amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT,
        amf_ue_ngap_id);
  } else {
    Logger::amf_n1().debug(
        "Could not delete nas_context associated with "
        "amf_ue_ngap_id " AMF_UE_NGAP_ID_FMT,
        amf_ue_ngap_id);
  }

  if (remove_imsi_2_nas_context(supi)) {
    Logger::amf_n1().debug(
        "Deleted nas_context associated SUPI %s ", supi.c_str());
  } else {
    Logger::amf_n1().debug(
        "Could not delete nas_context associated SUPI %s ", supi.c_str());
  }

  if (remove_guti_2_nas_context(dereg_request->get_5g_guti())) {
    Logger::amf_n1().debug(
        "Deleted nas_context associated GUTI %s ",
        dereg_request->get_5g_guti().c_str());
  } else {
    Logger::amf_n1().debug(
        "Could not delete nas_context associated GUTI %s ",
        dereg_request->get_5g_guti().c_str());
  }

  // TODO: AMF to AN: N2 UE Context Release Request
  // AMF sends N2 UE Release command to NG-RAN with Cause set to Deregistration
  // to release N2 signalling connection

  Logger::amf_n1().debug(
      "Sending ITTI UE Context Release Command to TASK_AMF_N2");

  std::shared_ptr<itti_ue_context_release_command> itti_msg =
      std::make_shared<itti_ue_context_release_command>(
          TASK_AMF_N1, TASK_AMF_N2);
  itti_msg->amf_ue_ngap_id = amf_ue_ngap_id;
  itti_msg->ran_ue_ngap_id = ran_ue_ngap_id;
  itti_msg->cause.setChoiceOfCause(Ngap_Cause_PR_nas);
  itti_msg->cause.setValue(
      2);  // TODO: remove hardcoded value cause nas(2)--deregister

  int ret = itti_inst->send_msg(itti_msg);
  if (0 != ret) {
    Logger::amf_n1().error(
        "Could not send ITTI message %s to task TASK_AMF_N2",
        itti_msg->get_msg_name());
  }
}

//------------------------------------------------------------------------------
void amf_n1::ul_nas_transport_handle(
    const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id, bstring nas,
    const plmn_t& plmn) {
  // Decode UL_NAS_TRANSPORT message
  Logger::amf_n1().debug("Handling UL NAS Transport");
  auto ul_nas = std::make_unique<ULNASTransport>();
  ul_nas->decodefrombuffer(NULL, (uint8_t*) bdata(nas), blength(nas));
  uint8_t payload_type   = ul_nas->getPayloadContainerType();
  uint8_t pdu_session_id = ul_nas->getPduSessionId();
  uint8_t request_type   = ul_nas->getRequestType();

  // SNSSAI
  SNSSAI_t snssai = {};
  if (!ul_nas->getSnssai(snssai)) {  // If no SNSSAI in this message, use the
                                     // one in Registration Request
    Logger::amf_n1().debug(
        "No Requested NSSAI available in ULNASTransport, use NSSAI from "
        "Requested NSSAI!");

    std::shared_ptr<nas_context> nc = {};
    if (amf_n1_inst->is_amf_ue_id_2_nas_context(amf_ue_ngap_id))
      nc = amf_n1_inst->amf_ue_id_2_nas_context(amf_ue_ngap_id);
    else {
      Logger::amf_n1().warn(
          "No existed nas_context with amf_ue_ngap_id(0x%x)", amf_ue_ngap_id);
      return;
    }

    // TODO: Only use the first one for now if there's multiple requested NSSAI
    // since we don't know which slice associated with this PDU session
    if (nc.get()->requestedNssai.size() > 0)
      snssai = nc.get()->requestedNssai[0];
  }

  Logger::amf_n1().debug(
      "S_NSSAI for this PDU Session SST (0x%x) SD (0x%x) hplmnSST (0x%x) "
      "hplmnSD (0x%x)",
      snssai.sst, snssai.sd, snssai.mHplmnSst, snssai.mHplmnSd);

  bstring dnn = bfromcstr("default");
  bstring sm_msg;
  if (ul_nas->getDnn(dnn)) {
  } else {
    dnn = bfromcstr("default");
  }
  comUt::print_buffer(
      "amf_n1", "Decoded DNN Bit String", (uint8_t*) bdata(dnn), blength(dnn));
  switch (payload_type) {
    case N1_SM_INFORMATION: {
      if (!ul_nas->getPayloadContainer(sm_msg)) {
        Logger::amf_n1().error("Cannot decode Payload Container");
        return;
      }

      std::shared_ptr<itti_nsmf_pdusession_create_sm_context> itti_msg =
          std::make_shared<itti_nsmf_pdusession_create_sm_context>(
              TASK_AMF_N1, TASK_AMF_N11);
      itti_msg->ran_ue_ngap_id = ran_ue_ngap_id;
      itti_msg->amf_ue_ngap_id = amf_ue_ngap_id;
      itti_msg->req_type       = request_type;
      itti_msg->pdu_sess_id    = pdu_session_id;
      itti_msg->dnn            = dnn;
      itti_msg->sm_msg         = sm_msg;
      itti_msg->snssai.sST     = snssai.sst;
      itti_msg->snssai.sD      = std::to_string(snssai.sd);
      itti_msg->plmn.mnc       = plmn.mnc;
      itti_msg->plmn.mcc       = plmn.mcc;

      int ret = itti_inst->send_msg(itti_msg);
      if (0 != ret) {
        Logger::amf_n1().error(
            "Could not send ITTI message %s to task TASK_AMF_N11",
            itti_msg->get_msg_name());
      }

    } break;
  }
}

//------------------------------------------------------------------------------
void amf_n1::sha256(
    unsigned char* message, int msg_len, unsigned char* output) {
  memset(output, 0, Sha256::DIGEST_SIZE);
  Sha256 ctx = {};
  ctx.init();
  ctx.update(message, msg_len);
  ctx.finalResult(output);
}

//------------------------------------------------------------------------------
void amf_n1::run_mobility_registration_update_procedure(
    std::shared_ptr<nas_context>& nc, uint16_t uplink_data_status,
    uint16_t pdu_session_status) {
  // Encoding REGISTRATION ACCEPT
  auto reg_accept = std::make_unique<RegistrationAccept>();
  initialize_registration_accept(reg_accept);
  reg_accept->set_5GS_Network_Feature_Support(
      0x00, 0x00);  // TODO: remove hardcoded values

  std::shared_ptr<pdu_session_context> psc = {};
  std::shared_ptr<ue_context> uc           = {};
  if (!find_ue_context(nc, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  reg_accept->set5G_GUTI(
      amf_cfg.guami.mcc, amf_cfg.guami.mnc, amf_cfg.guami.regionID,
      amf_cfg.guami.AmfSetID, amf_cfg.guami.AmfPointer, uc.get()->tmsi);

  uint8_t buffer[BUFFER_SIZE_1024] = {0};
  int encoded_size = reg_accept->encode2buffer(buffer, BUFFER_SIZE_1024);
  comUt::print_buffer(
      "amf_n1", "Registration-Accept Message Buffer", buffer, encoded_size);
  if (!encoded_size) {
    Logger::nas_mm().error("Encode Registration-Accept message error");
    return;
  }

  nas_secu_ctx* secu = nc.get()->security_ctx;
  if (!secu) {
    Logger::amf_n1().error("No Security Context found");
    return;
  }

  // protect nas message
  bstring protectedNas;
  encode_nas_message_protected(
      secu, false, INTEGRITY_PROTECTED_AND_CIPHERED, NAS_MESSAGE_DOWNLINK,
      buffer, encoded_size, protectedNas);

  // get PDU session status
  std::vector<uint8_t> pdu_session_to_be_activated = {};
  get_pdu_session_to_be_activated(
      pdu_session_status, pdu_session_to_be_activated);

  if (pdu_session_to_be_activated.size() > 0) {
    // get PDU session context for 1 PDU session for now
    // TODO: multiple PDU sessions
    uc->find_pdu_session_context(pdu_session_to_be_activated[0], psc);
  }

  uint8_t* kamf = nc.get()->kamf[secu->vector_pointer];
  if (!kamf) {
    Logger::amf_n1().error("No Kamf found");
    return;
  }

  uint8_t kgnb[32];
  uint32_t ulcount = secu->ul_count.seq_num | (secu->ul_count.overflow << 8);
  Authentication_5gaka::derive_kgnb(ulcount, 0x01, kamf, kgnb);
  comUt::print_buffer("amf_n1", "kamf", kamf, 32);
  bstring kgnb_bs = blk2bstr(kgnb, 32);

  std::shared_ptr<itti_initial_context_setup_request> itti_msg =
      std::make_shared<itti_initial_context_setup_request>(
          TASK_AMF_N1, TASK_AMF_N2);
  itti_msg->ran_ue_ngap_id = nc.get()->ran_ue_ngap_id;
  itti_msg->amf_ue_ngap_id = nc.get()->amf_ue_ngap_id;
  itti_msg->kgnb           = kgnb_bs;
  itti_msg->nas            = protectedNas;
  itti_msg->is_sr          = true;  // service request indicator, to be verified

  if (psc.get() != nullptr) {
    itti_msg->pdu_session_id = psc.get()->pdu_session_id;
    itti_msg->n2sm           = psc.get()->n2sm;
  }

  int ret = itti_inst->send_msg(itti_msg);
  if (0 != ret) {
    Logger::amf_n1().error(
        "Could not send ITTI message %s to task TASK_AMF_N2",
        itti_msg->get_msg_name());
  }
}

//------------------------------------------------------------------------------
void amf_n1::run_periodic_registration_update_procedure(
    std::shared_ptr<nas_context>& nc, uint16_t pdu_session_status) {
  // Experimental procedure
  // Encoding REGISTRATION ACCEPT
  auto reg_accept = std::make_unique<RegistrationAccept>();
  initialize_registration_accept(reg_accept);

  // Get UE context
  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(nc, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  reg_accept->set5G_GUTI(
      amf_cfg.guami.mcc, amf_cfg.guami.mnc, amf_cfg.guami.regionID,
      amf_cfg.guami.AmfSetID, amf_cfg.guami.AmfPointer, uc.get()->tmsi);

  if (pdu_session_status == 0x0000) {
    reg_accept->setPDU_session_status(0x0000);
  } else {
    reg_accept->setPDU_session_status(pdu_session_status);
    Logger::amf_n1().debug(
        "PDU Session Status 0x%02x", htonl(pdu_session_status));
  }

  reg_accept->set_5GS_Network_Feature_Support(0x01, 0x00);
  uint8_t buffer[BUFFER_SIZE_1024] = {0};
  int encoded_size = reg_accept->encode2buffer(buffer, BUFFER_SIZE_1024);
  comUt::print_buffer(
      "amf_n1", "Registration-Accept Message Buffer", buffer, encoded_size);
  if (!encoded_size) {
    Logger::nas_mm().error("Encode Registration-Accept message error");
    return;
  }

  nas_secu_ctx* secu = nc.get()->security_ctx;
  if (!secu) {
    Logger::amf_n1().error("No Security Context found");
    return;
  }

  bstring protectedNas;
  encode_nas_message_protected(
      secu, false, INTEGRITY_PROTECTED_AND_CIPHERED, NAS_MESSAGE_DOWNLINK,
      buffer, encoded_size, protectedNas);

  std::shared_ptr<itti_dl_nas_transport> itti_msg =
      std::make_shared<itti_dl_nas_transport>(TASK_AMF_N1, TASK_AMF_N2);
  itti_msg->ran_ue_ngap_id = nc.get()->ran_ue_ngap_id;
  itti_msg->amf_ue_ngap_id = nc.get()->amf_ue_ngap_id;
  itti_msg->nas            = protectedNas;

  int ret = itti_inst->send_msg(itti_msg);
  if (0 != ret) {
    Logger::amf_n1().error(
        "Could not send ITTI message %s to task TASK_AMF_N2",
        itti_msg->get_msg_name());
  }
}
//------------------------------------------------------------------------------
void amf_n1::run_periodic_registration_update_procedure(
    std::shared_ptr<nas_context>& nc, bstring& nas_msg) {
  // Experimental procedure

  // decoding REGISTRATION request
  std::unique_ptr<RegistrationRequest> registration_request =
      std::make_unique<RegistrationRequest>();
  registration_request->decodefrombuffer(
      nullptr, (uint8_t*) bdata(nas_msg), blength(nas_msg));
  bdestroy(nas_msg);  // free buffer

  // Encoding REGISTRATION ACCEPT
  auto reg_accept = std::make_unique<RegistrationAccept>();
  initialize_registration_accept(reg_accept);

  // Get UE context
  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(nc, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  reg_accept->set5G_GUTI(
      amf_cfg.guami.mcc, amf_cfg.guami.mnc, amf_cfg.guami.regionID,
      amf_cfg.guami.AmfSetID, amf_cfg.guami.AmfPointer, uc.get()->tmsi);

  uint16_t pdu_session_status = 0xffff;
  pdu_session_status          = registration_request->getPduSessionStatus();
  if (pdu_session_status == 0x0000) {
    reg_accept->setPDU_session_status(0x0000);
  } else {
    reg_accept->setPDU_session_status(pdu_session_status);
    Logger::amf_n1().debug(
        "PDU Session Status 0x%02x", htonl(pdu_session_status));
  }

  reg_accept->set_5GS_Network_Feature_Support(0x01, 0x00);
  uint8_t buffer[BUFFER_SIZE_1024] = {0};
  int encoded_size = reg_accept->encode2buffer(buffer, BUFFER_SIZE_1024);
  comUt::print_buffer(
      "amf_n1", "Registration-Accept Message Buffer", buffer, encoded_size);
  if (!encoded_size) {
    Logger::nas_mm().error("Encode Registration-Accept message error");
    return;
  }

  nas_secu_ctx* secu = nc.get()->security_ctx;
  if (!secu) {
    Logger::amf_n1().error("No Security Context found");
    return;
  }

  bstring protectedNas;
  encode_nas_message_protected(
      secu, false, INTEGRITY_PROTECTED_AND_CIPHERED, NAS_MESSAGE_DOWNLINK,
      buffer, encoded_size, protectedNas);

  std::shared_ptr<itti_dl_nas_transport> itti_msg =
      std::make_shared<itti_dl_nas_transport>(TASK_AMF_N1, TASK_AMF_N2);
  itti_msg->ran_ue_ngap_id = nc.get()->ran_ue_ngap_id;
  itti_msg->amf_ue_ngap_id = nc.get()->amf_ue_ngap_id;
  itti_msg->nas            = protectedNas;

  int ret = itti_inst->send_msg(itti_msg);
  if (0 != ret) {
    Logger::amf_n1().error(
        "Could not send ITTI message %s to task TASK_AMF_N2",
        itti_msg->get_msg_name());
  }
}

//------------------------------------------------------------------------------
void amf_n1::set_5gmm_state(
    std::shared_ptr<nas_context>& nc, const _5gmm_state_t& state) {
  Logger::amf_n1().debug(
      "Set 5GMM state to %s", _5gmm_state_e2str[state].c_str());
  std::unique_lock lock(m_nas_context);
  nc.get()->_5gmm_state = state;
}

//------------------------------------------------------------------------------
void amf_n1::get_5gmm_state(
    const std::shared_ptr<nas_context>& nc, _5gmm_state_t& state) const {
  std::shared_lock lock(m_nas_context);
  state = nc.get()->_5gmm_state;
}

//------------------------------------------------------------------------------
void amf_n1::set_5gcm_state(
    std::shared_ptr<nas_context>& nc, const cm_state_t& state) {
  std::shared_lock lock(m_nas_context);
  nc.get()->nas_status = state;
}

//------------------------------------------------------------------------------
void amf_n1::get_5gcm_state(
    const std::shared_ptr<nas_context>& nc, cm_state_t& state) const {
  std::shared_lock lock(m_nas_context);
  state = nc.get()->nas_status;
}

//------------------------------------------------------------------------------
void amf_n1::handle_ue_location_change(
    std::string supi, oai::amf::model::UserLocation user_location,
    uint8_t http_version) {
  Logger::amf_n1().debug(
      "Send request to SBI to trigger UE Location Report (SUPI "
      "%s )",
      supi.c_str());
  std::vector<std::shared_ptr<amf_subscription>> subscriptions = {};
  amf_app_inst->get_ee_subscriptions(
      amf_event_type_t::LOCATION_REPORT, subscriptions);

  if (subscriptions.size() > 0) {
    // Send request to SBI to trigger the notification to the subscribed event
    Logger::amf_n1().debug(
        "Send ITTI msg to AMF SBI to trigger the event notification");

    std::shared_ptr<itti_sbi_notify_subscribed_event> itti_msg =
        std::make_shared<itti_sbi_notify_subscribed_event>(
            TASK_AMF_N1, TASK_AMF_N11);

    itti_msg->http_version = 1;

    for (auto i : subscriptions) {
      event_notification ev_notif = {};
      ev_notif.set_notify_correlation_id(i.get()->notify_correlation_id);
      ev_notif.set_notify_uri(i.get()->notify_uri);  // Direct subscription
      // ev_notif.set_subs_change_notify_correlation_id(i.get()->notify_uri);

      oai::amf::model::AmfEventReport event_report = {};
      oai::amf::model::AmfEventType amf_event_type = {};
      amf_event_type.set_value("LOCATION_REPORT");
      event_report.setType(amf_event_type);

      oai::amf::model::AmfEventState amf_event_state = {};
      amf_event_state.setActive(true);
      event_report.setState(amf_event_state);

      event_report.setLocation(user_location);

      event_report.setSupi(supi);
      ev_notif.add_report(event_report);

      itti_msg->event_notifs.push_back(ev_notif);
    }

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N11",
          itti_msg->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_n1::handle_ue_reachability_status_change(
    std::string supi, uint8_t status, uint8_t http_version) {
  Logger::amf_n1().debug(
      "Send request to SBI to trigger UE Reachability Report (SUPI "
      "%s )",
      supi.c_str());

  std::vector<std::shared_ptr<amf_subscription>> subscriptions = {};
  amf_app_inst->get_ee_subscriptions(
      amf_event_type_t::REACHABILITY_REPORT, subscriptions);

  if (subscriptions.size() > 0) {
    // Send request to SBI to trigger the notification to the subscribed event
    Logger::amf_n1().debug(
        "Send ITTI msg to AMF SBI to trigger the event notification");

    std::shared_ptr<itti_sbi_notify_subscribed_event> itti_msg =
        std::make_shared<itti_sbi_notify_subscribed_event>(
            TASK_AMF_N1, TASK_AMF_N11);

    itti_msg->http_version = 1;

    for (auto i : subscriptions) {
      event_notification ev_notif = {};
      ev_notif.set_notify_correlation_id(i.get()->notify_correlation_id);
      ev_notif.set_notify_uri(i.get()->notify_uri);  // Direct subscription
      // ev_notif.set_subs_change_notify_correlation_id(i.get()->notify_uri);

      oai::amf::model::AmfEventReport event_report = {};
      oai::amf::model::AmfEventType amf_event_type = {};
      amf_event_type.set_value("REACHABILITY_REPORT");
      event_report.setType(amf_event_type);

      oai::amf::model::AmfEventState amf_event_state = {};
      amf_event_state.setActive(true);
      event_report.setState(amf_event_state);

      oai::amf::model::UeReachability ue_reachability = {};
      if (status == CM_CONNECTED)
        ue_reachability.set_value("REACHABLE");
      else
        ue_reachability.set_value("UNREACHABLE");

      event_report.setReachability(ue_reachability);
      event_report.setSupi(supi);
      ev_notif.add_report(event_report);

      itti_msg->event_notifs.push_back(ev_notif);
    }

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N11",
          itti_msg->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_n1::handle_ue_registration_state_change(
    std::string supi, uint8_t status, uint8_t http_version) {
  Logger::amf_n1().debug(
      "Send request to SBI to trigger UE Registration State Report (SUPI "
      "%s )",
      supi.c_str());

  std::vector<std::shared_ptr<amf_subscription>> subscriptions = {};
  amf_app_inst->get_ee_subscriptions(
      amf_event_type_t::REGISTRATION_STATE_REPORT, subscriptions);

  if (subscriptions.size() > 0) {
    // Send request to SBI to trigger the notification to the subscribed event
    Logger::amf_n1().debug(
        "Send ITTI msg to AMF SBI to trigger the event notification");

    std::shared_ptr<itti_sbi_notify_subscribed_event> itti_msg =
        std::make_shared<itti_sbi_notify_subscribed_event>(
            TASK_AMF_N1, TASK_AMF_N11);

    itti_msg->http_version = 1;

    for (auto i : subscriptions) {
      event_notification ev_notif = {};
      ev_notif.set_notify_correlation_id(i.get()->notify_correlation_id);
      ev_notif.set_notify_uri(i.get()->notify_uri);  // Direct subscription
      // ev_notif.set_subs_change_notify_correlation_id(i.get()->notify_uri);

      oai::amf::model::AmfEventReport event_report = {};

      oai::amf::model::AmfEventType amf_event_type = {};
      amf_event_type.set_value("REGISTRATION_STATE_REPORT");
      event_report.setType(amf_event_type);

      oai::amf::model::AmfEventState amf_event_state = {};
      amf_event_state.setActive(true);
      event_report.setState(amf_event_state);

      std::vector<oai::amf::model::RmInfo> rm_infos;
      oai::amf::model::RmInfo rm_info   = {};
      oai::amf::model::RmState rm_state = {};
      rm_state.set_value("REGISTERED");
      rm_info.setRmState(rm_state);

      oai::amf::model::AccessType access_type = {};
      access_type.setValue(AccessType::eAccessType::_3GPP_ACCESS);
      rm_info.setAccessType(access_type);

      rm_infos.push_back(rm_info);
      event_report.setRmInfoList(rm_infos);

      event_report.setSupi(supi);
      ev_notif.add_report(event_report);

      itti_msg->event_notifs.push_back(ev_notif);
    }

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N11",
          itti_msg->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_n1::handle_ue_connectivity_state_change(
    std::string supi, uint8_t status, uint8_t http_version) {
  Logger::amf_n1().debug(
      "Send request to SBI to trigger UE Connectivity State Report (SUPI "
      "%s )",
      supi.c_str());

  std::vector<std::shared_ptr<amf_subscription>> subscriptions = {};
  amf_app_inst->get_ee_subscriptions(
      amf_event_type_t::CONNECTIVITY_STATE_REPORT, subscriptions);

  if (subscriptions.size() > 0) {
    // Send request to SBI to trigger the notification to the subscribed event
    Logger::amf_n1().debug(
        "Send ITTI msg to AMF SBI to trigger the event notification");

    std::shared_ptr<itti_sbi_notify_subscribed_event> itti_msg =
        std::make_shared<itti_sbi_notify_subscribed_event>(
            TASK_AMF_N1, TASK_AMF_N11);

    itti_msg->http_version = 1;

    for (auto i : subscriptions) {
      event_notification ev_notif = {};
      ev_notif.set_notify_correlation_id(i.get()->notify_correlation_id);
      ev_notif.set_notify_uri(i.get()->notify_uri);  // Direct subscription
      // ev_notif.set_subs_change_notify_correlation_id(i.get()->notify_uri);

      oai::amf::model::AmfEventReport event_report = {};

      oai::amf::model::AmfEventType amf_event_type = {};
      amf_event_type.set_value("CONNECTIVITY_STATE_REPORT");
      event_report.setType(amf_event_type);

      oai::amf::model::AmfEventState amf_event_state = {};
      amf_event_state.setActive(true);
      event_report.setState(amf_event_state);

      std::vector<oai::amf::model::CmInfo> cm_infos;
      oai::amf::model::CmInfo cm_info   = {};
      oai::amf::model::CmState cm_state = {};
      cm_state.set_value("CONNECTED");
      cm_info.setCmState(cm_state);

      oai::amf::model::AccessType access_type = {};
      access_type.setValue(AccessType::eAccessType::_3GPP_ACCESS);
      cm_info.setAccessType(access_type);
      cm_infos.push_back(cm_info);
      event_report.setCmInfoList(cm_infos);

      event_report.setSupi(supi);
      ev_notif.add_report(event_report);

      itti_msg->event_notifs.push_back(ev_notif);
    }

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N11",
          itti_msg->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_n1::get_pdu_session_to_be_activated(
    const uint16_t pdu_session_status,
    std::vector<uint8_t>& pdu_session_to_be_activated) {
  std::bitset<16> pdu_session_status_bits(pdu_session_status);

  for (int i = 0; i < 15; i++) {
    if (pdu_session_status_bits.test(i)) {
      if (i <= 7)
        pdu_session_to_be_activated.push_back(8 + i);
      else if (i > 8)
        pdu_session_to_be_activated.push_back(i - 8);
    }
  }

  if (pdu_session_to_be_activated.size() > 0) {
    for (auto i : pdu_session_to_be_activated)
      Logger::amf_n1().debug("PDU session to be activated %d", i);
  }
}

//------------------------------------------------------------------------------
void amf_n1::initialize_registration_accept(
    std::unique_ptr<nas::RegistrationAccept>& registration_accept) {
  registration_accept->setHeader(PLAIN_5GS_MSG);
  registration_accept->set_5GS_Registration_Result(
      false, false, false, 0x01);  // 3GPP Access
  registration_accept->setT3512_Value(0x5, T3512_TIMER_VALUE_MIN);

  std::vector<p_tai_t> tai_list;
  for (auto p : amf_cfg.plmn_list) {
    p_tai_t item    = {};
    item.type       = 0x00;
    nas_plmn_t plmn = {};
    plmn.mcc        = p.mcc;
    plmn.mnc        = p.mnc;
    item.plmn_list.push_back(plmn);
    item.tac_list.push_back(p.tac);
    tai_list.push_back(item);
  }
  registration_accept->setTaiList(tai_list);

  // TODO: get the list of common SST, SD between UE/gNB and AMF
  std::vector<struct SNSSAI_s> nssai;
  for (auto p : amf_cfg.plmn_list) {
    for (auto s : p.slice_list) {
      SNSSAI_t snssai = {};
      snssai.sst      = s.sst;
      snssai.sd       = s.sd;
      nssai.push_back(snssai);
    }
  }
  registration_accept->setALLOWED_NSSAI(nssai);
  return;
}

//------------------------------------------------------------------------------
void amf_n1::initialize_registration_accept(
    std::unique_ptr<nas::RegistrationAccept>& registration_accept,
    const std::shared_ptr<nas_context>& nc) {
  registration_accept->setHeader(PLAIN_5GS_MSG);
  registration_accept->set_5GS_Registration_Result(
      false, false, false, 0x01);  // 3GPP Access
  registration_accept->setT3512_Value(0x5, T3512_TIMER_VALUE_MIN);

  // Find UE Context
  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(
          nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  std::vector<p_tai_t> tai_list;
  for (auto p : amf_cfg.plmn_list) {
    p_tai_t item    = {};
    item.type       = 0x00;
    nas_plmn_t plmn = {};
    plmn.mcc        = p.mcc;
    plmn.mnc        = p.mnc;
    item.plmn_list.push_back(plmn);
    item.tac_list.push_back(p.tac);
    tai_list.push_back(item);
  }
  registration_accept->setTaiList(tai_list);

  // TODO: get the list of common SST, SD between UE and AMF
  std::vector<struct SNSSAI_s> nssai;
  for (auto p : amf_cfg.plmn_list) {
    if ((p.mcc.compare(uc.get()->tai.mcc) == 0) and
        (p.mnc.compare(uc.get()->tai.mnc) == 0) and
        (p.tac == uc.get()->tai.tac)) {
      for (auto s : p.slice_list) {
        SNSSAI_t snssai = {};
        snssai.sst      = s.sst;
        snssai.sd       = s.sd;
        nssai.push_back(snssai);
        // TODO: Check with the requested NSSAI from UE
        /*  for (auto rn : nc.get()->requestedNssai) {
             if ((rn.sst == snssai.sst) and (rn.sd == snssai.sd)) {
               nssai.push_back(snssai);
               break;
             }
           }
          */
      }
    }
  }
  registration_accept->setALLOWED_NSSAI(nssai);
  return;
}

//------------------------------------------------------------------------------
bool amf_n1::find_ue_context(
    const std::shared_ptr<nas_context>& nc, std::shared_ptr<ue_context>& uc) {
  string supi = "imsi-" + nc.get()->imsi;
  Logger::amf_n1().debug("Key for PDU Session Context SUPI (%s)", supi.c_str());

  string ue_context_key = "app_ue_ranid_" + to_string(nc->ran_ue_ngap_id) +
                          ":amfid_" + to_string(nc->amf_ue_ngap_id);

  if (!amf_app_inst->is_ran_amf_id_2_ue_context(ue_context_key)) {
    Logger::amf_n1().error("No UE context with key %s", ue_context_key.c_str());
    return false;
  }

  uc = amf_app_inst->ran_amf_id_2_ue_context(ue_context_key);

  if (uc.get() == nullptr) {
    Logger::amf_n1().warn(
        "Cannot find the UE context with key %s", ue_context_key.c_str());
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::find_ue_context(
    uint32_t ran_ue_ngap_id, long amf_ue_ngap_id,
    std::shared_ptr<ue_context>& uc) {
  string ue_context_key = "app_ue_ranid_" + to_string(ran_ue_ngap_id) +
                          ":amfid_" + to_string(amf_ue_ngap_id);

  if (!amf_app_inst->is_ran_amf_id_2_ue_context(ue_context_key)) {
    Logger::amf_n1().error("No UE context with key %s", ue_context_key.c_str());
    return false;
  }

  uc = amf_app_inst->ran_amf_id_2_ue_context(ue_context_key);

  if (uc.get() == nullptr) {
    Logger::amf_n1().warn(
        "Cannot find the UE context with key %s", ue_context_key.c_str());
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void amf_n1::mobile_reachable_timer_timeout(
    timer_id_t& timer_id, const uint64_t amf_ue_ngap_id) {
  std::shared_ptr<nas_context> nc;
  if (amf_n1_inst->is_amf_ue_id_2_nas_context(amf_ue_ngap_id))
    nc = amf_n1_inst->amf_ue_id_2_nas_context(amf_ue_ngap_id);
  else {
    Logger::amf_n1().warn(
        "No existed nas_context with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT ")",
        amf_ue_ngap_id);
    return;
  }
  set_mobile_reachable_timer_timeout(nc, true);

  // TODO: Start the implicit de-registration timer
  timer_id_t tid = itti_inst->timer_setup(
      IMPLICIT_DEREGISTRATION_TIMER_MIN * 60, 0, TASK_AMF_N1,
      TASK_AMF_IMPLICIT_DEREGISTRATION_TIMER_EXPIRE, amf_ue_ngap_id);
  Logger::amf_n1().startup(
      "Started Implicit De-Registration Timer (tid %d)", tid);

  set_implicit_deregistration_timer(nc, tid);
}

//------------------------------------------------------------------------------
void amf_n1::implicit_deregistration_timer_timeout(
    timer_id_t timer_id, uint64_t amf_ue_ngap_id) {
  std::shared_ptr<nas_context> nc;
  if (amf_n1_inst->is_amf_ue_id_2_nas_context(amf_ue_ngap_id))
    nc = amf_n1_inst->amf_ue_id_2_nas_context(amf_ue_ngap_id);
  else {
    Logger::amf_n1().warn(
        "No existed nas_context with amf_ue_ngap_id (" AMF_UE_NGAP_ID_FMT ")",
        amf_ue_ngap_id);
    return;
  }
  // Implicitly de-register UE
  // TODO (4.2.2.3.3 Network-initiated Deregistration @3GPP TS 23.502V16.0.0):
  // If the UE is in CM-CONNECTED state, the AMF may explicitly deregister the
  // UE by sending a Deregistration Request message (Deregistration type, Access
  // Type) to the UE

  // Send PDU Session Release SM Context Request to SMF for each PDU Session
  std::shared_ptr<ue_context> uc = {};

  if (!find_ue_context(
          nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return;
  }

  std::vector<std::shared_ptr<pdu_session_context>> pdu_sessions;
  if (!uc.get()->get_pdu_sessions_context(pdu_sessions)) return;

  for (auto p : pdu_sessions) {
    std::shared_ptr<itti_nsmf_pdusession_release_sm_context> itti_msg =
        std::make_shared<itti_nsmf_pdusession_release_sm_context>(
            TASK_AMF_N1, TASK_AMF_N11);
    itti_msg->supi           = uc->supi;
    itti_msg->pdu_session_id = p->pdu_session_id;

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N11",
          itti_msg->get_msg_name());
    }
  }

  // Send N2 UE Release command to NG-RAN if there is a N2 signalling connection
  // to NG-RAN
  Logger::amf_n1().debug(
      "Sending ITTI UE Context Release Command to TASK_AMF_N2");

  std::shared_ptr<itti_ue_context_release_command> itti_msg_cxt_release =
      std::make_shared<itti_ue_context_release_command>(
          TASK_AMF_N1, TASK_AMF_N2);
  itti_msg_cxt_release->amf_ue_ngap_id = nc.get()->amf_ue_ngap_id;
  itti_msg_cxt_release->ran_ue_ngap_id = nc.get()->ran_ue_ngap_id;
  itti_msg_cxt_release->cause.setChoiceOfCause(Ngap_Cause_PR_nas);
  itti_msg_cxt_release->cause.setValue(Ngap_CauseNas_deregister);

  int ret = itti_inst->send_msg(itti_msg_cxt_release);
  if (0 != ret) {
    Logger::amf_n1().error(
        "Could not send ITTI message %s to task TASK_AMF_N2",
        itti_msg_cxt_release->get_msg_name());
  }
}

//------------------------------------------------------------------------------
void amf_n1::set_implicit_deregistration_timer(
    std::shared_ptr<nas_context>& nc, const timer_id_t& t) {
  std::unique_lock lock(m_nas_context);
  nc.get()->implicit_deregistration_timer = t;
}
//------------------------------------------------------------------------------
void amf_n1::set_mobile_reachable_timer(
    std::shared_ptr<nas_context>& nc, const timer_id_t& t) {
  std::unique_lock lock(m_nas_context);
  nc.get()->mobile_reachable_timer = t;
}

//------------------------------------------------------------------------------
void amf_n1::set_mobile_reachable_timer_timeout(
    std::shared_ptr<nas_context>& nc, const bool& b) {
  std::unique_lock lock(m_nas_context);
  nc.get()->is_mobile_reachable_timer_timeout = b;
}

//------------------------------------------------------------------------------
void amf_n1::get_mobile_reachable_timer_timeout(
    const std::shared_ptr<nas_context>& nc, bool& b) const {
  std::shared_lock lock(m_nas_context);
  b = nc.get()->is_mobile_reachable_timer_timeout;
}

//------------------------------------------------------------------------------
bool amf_n1::get_mobile_reachable_timer_timeout(
    const std::shared_ptr<nas_context>& nc) const {
  std::shared_lock lock(m_nas_context);
  return nc.get()->is_mobile_reachable_timer_timeout;
}

//------------------------------------------------------------------------------
bool amf_n1::reroute_registration_request(
    std::shared_ptr<nas_context>& nc, bool& reroute_result) {
  // Verifying whether this AMF can handle the request (if not, AMF
  // re-allocation procedure will be executed to reroute the Registration "
  // Request to an appropriate AMF

  Logger::amf_n1().debug(
      "Verifying whether this AMF can handle the request...");

  /*
  // Check if the AMF can serve all the requested S-NSSAIs
  if (check_requested_nssai(nc)) {
    Logger::amf_n1().debug(
        "Current AMF can handle all the requested NSSAIs, do not need to "
        "perform AMF Re-allocation procedure");
    return false;
  }
*/

  // Get NSSAI from UDM
  oai::amf::model::Nssai nssai = {};
  if (!get_slice_selection_subscription_data(nc, nssai)) {
    Logger::amf_n1().debug(
        "Could not get the Slice Selection Subscription Data from UDM");
    return false;
  }

  // TODO: Update subscribed NSSAIs

  // Check that AMF can process the Requested NSSAIs or not
  if (check_subscribed_nssai(nc, nssai)) {
    Logger::amf_n1().debug(
        "Current AMF can handle the Requested/Subscribed NSSAIs, no need "
        "to perform AMF Re-allocation procedure");
    return false;
  }

  // If the current AMF can't process the Requested NSSAIs
  // find the appropriate AMFs and let them handle the UE

  // Process NS selection to select the appropriate AMF
  oai::amf::model::SliceInfoForRegistration slice_info = {};
  oai::amf::model::AuthorizedNetworkSliceInfo authorized_network_slice_info =
      {};

  std::vector<SubscribedSnssai> subscribed_snssais;
  for (auto n : nssai.getDefaultSingleNssais()) {
    SubscribedSnssai subscribed_snssai = {};
    subscribed_snssai.setSubscribedSnssai(n);
    subscribed_snssai.setDefaultIndication(true);
    subscribed_snssais.push_back(subscribed_snssai);
  }
  slice_info.setSubscribedNssai(subscribed_snssais);

  // Requested NSSAIs
  std::vector<oai::amf::model::Snssai> requested_nssais;
  for (auto s : nc->requestedNssai) {
    oai::amf::model::Snssai nssai = {};
    nssai.setSst(s.sst);
    nssai.setSd(std::to_string(s.sd));
    requested_nssais.push_back(nssai);
  }
  slice_info.setRequestedNssai(requested_nssais);

  if (!get_network_slice_selection(
          nc, amf_app_inst->get_nf_instance(), slice_info,
          authorized_network_slice_info)) {
    Logger::amf_n1().debug(
        "Could not get the Network Slice Selection information from NSSF");
    reroute_result = false;
    return false;
  }

  // Find the appropriate target AMF and send N1MessageNotify to the AMF
  // otherwise reroute NAS message via AN
  std::string target_amf = {};
  if (get_target_amf(nc, target_amf, authorized_network_slice_info)) {
    Logger::amf_n1().debug("Target AMF %s", target_amf.c_str());

    send_n1_message_notity(nc, target_amf);
    return true;
  } else {
    Logger::amf_n1().debug(
        "Could not find an appropriate target AMF to handle UE");
    // Reroute NAS message via AN
    std::string target_amf_set = {};
    if (authorized_network_slice_info.targetAmfSetIsSet()) {
      target_amf_set = authorized_network_slice_info.getTargetAmfSet();
      Logger::amf_n1().debug("Target AMF Set %s", target_amf_set.c_str());
    } else {
      Logger::amf_n1().debug("No Target AMF Set info available!");
      reroute_result = false;
      return false;
    }

    if (reroute_nas_via_an(nc, target_amf_set)) return true;
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::check_requested_nssai(const std::shared_ptr<nas_context>& nc) {
  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(
          nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return false;
  }

  // If there no requested NSSAIs
  if (nc->requestedNssai.size() == 0) {
    return false;
  }

  bool result = false;
  for (auto p : amf_cfg.plmn_list) {
    // Check PLMN/TAC
    if ((uc.get()->tai.mcc.compare(p.mcc) != 0) or
        (uc.get()->tai.mnc.compare(p.mnc) != 0) or
        (uc.get()->tai.tac != p.tac)) {
      continue;
    }

    result = true;
    // check if AMF can serve all the requested NSSAIs
    for (auto n : nc->requestedNssai) {
      bool found_nssai = false;
      for (auto s : p.slice_list) {
        std::string sd = std::to_string(s.sd);
        if ((s.sst == n.sst) and (s.sd == n.sd)) {
          found_nssai = true;
          Logger::amf_n1().debug("Found S-NSSAI (SST %d, SD %d)", s.sst, n.sd);
          break;
        }
      }
      if (!found_nssai) {
        result = false;
        break;
      }
    }
  }

  return result;
}

//------------------------------------------------------------------------------
bool amf_n1::check_subscribed_nssai(
    const std::shared_ptr<nas_context>& nc, oai::amf::model::Nssai& nssai) {
  // Check if the AMF can serve all the requested/subscribed S-NSSAIs

  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(
          nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return false;
  }

  bool result = false;

  for (auto p : amf_cfg.plmn_list) {
    // Check PLMN/TAC
    if ((uc.get()->tai.mcc.compare(p.mcc) != 0) or
        (uc.get()->tai.mnc.compare(p.mnc) != 0) or
        (uc.get()->tai.tac != p.tac)) {
      continue;
    }

    result = true;

    // Find the common NSSAIs between Requested NSSAIs and Subscribed NSSAIs
    Logger::amf_n1().debug(
        "Find the common NSSAIs between Requested NSSAIs and Subscribed "
        "NSSAIs");
    std::vector<oai::amf::model::Snssai> common_snssais;
    for (auto s : nc->requestedNssai) {
      std::string sd = std::to_string(s.sd);
      // Check with default subscribed NSSAIs
      for (auto n : nssai.getDefaultSingleNssais()) {
        if (s.sst == n.getSst()) {
          if ((n.sdIsSet() and (n.getSd().compare(sd) == 0)) or
              (!n.sdIsSet() and sd.empty())) {
            common_snssais.push_back(n);
            Logger::amf_n1().debug(
                "Common S-NSSAI (SST %d, SD %s)", s.sst, sd.c_str());
            break;
          }
        }
      }

      // check with other subscribed NSSAIs
      for (auto n : nssai.getSingleNssais()) {
        if (s.sst == n.getSst()) {
          if ((n.sdIsSet() and (n.getSd().compare(sd) == 0)) or
              (!n.sdIsSet() and sd.empty())) {
            common_snssais.push_back(n);
            Logger::amf_n1().debug(
                "Common S-NSSAI (SST %d, SD %s)", s.sst, sd.c_str());
            break;
          }
        }
      }
    }

    // If there no requested NSSAIs or no common NSSAIs between requested NSSAIs
    // and Subscribed NSSAIs
    if ((nc->requestedNssai.size() == 0) or (common_snssais.size() == 0)) {
      // Each S-NSSAI in the Default Single NSSAIs must be in the AMF's Slice
      // List
      for (auto n : nssai.getDefaultSingleNssais()) {
        bool found_nssai = false;
        for (auto s : p.slice_list) {
          std::string sd = std::to_string(s.sd);
          if (s.sst == n.getSst()) {
            if ((n.sdIsSet() and (n.getSd().compare(sd) == 0)) or
                (!n.sdIsSet() and sd.empty())) {
              found_nssai = true;
              Logger::amf_n1().debug(
                  "Found S-NSSAI (SST %d, SD %s)", s.sst, n.getSd().c_str());
              break;
            }
          }
        }

        if (!found_nssai) {
          result = false;
          break;
        }
      }
    } else {
      // check if AMF can serve all the common NSSAIs
      for (auto n : common_snssais) {
        bool found_nssai = false;
        for (auto s : p.slice_list) {
          std::string sd = std::to_string(s.sd);
          if (s.sst == n.getSst()) {
            if ((n.sdIsSet() and (n.getSd().compare(sd) == 0)) or
                (!n.sdIsSet() and sd.empty())) {
              found_nssai = true;
              Logger::amf_n1().debug(
                  "Found S-NSSAI (SST %d, SD %s)", s.sst, n.getSd().c_str());
              break;
            }
          }
        }

        if (!found_nssai) {
          result = false;
          break;
        }
      }
    }
  }

  return result;
}

//------------------------------------------------------------------------------
bool amf_n1::get_slice_selection_subscription_data(
    const std::shared_ptr<nas_context>& nc, oai::amf::model::Nssai& nssai) {
  // TODO: UDM selection (from NRF or configuration file)
  if (amf_cfg.support_features.enable_external_udm) {
    Logger::amf_n1().debug(
        "Get the Slice Selection Subscription Data from UDM");

    std::shared_ptr<ue_context> uc = {};
    if (!find_ue_context(
            nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id, uc)) {
      Logger::amf_n1().warn("Cannot find the UE context");
      return false;
    }

    std::shared_ptr<itti_n11_slice_selection_subscription_data> itti_msg =
        std::make_shared<itti_n11_slice_selection_subscription_data>(
            TASK_AMF_N1, TASK_AMF_N11);

    // Generate a promise and associate this promise to the ITTI message
    uint32_t promise_id = amf_app_inst->generate_promise_id();
    Logger::amf_n1().debug("Promise ID generated %d", promise_id);

    boost::shared_ptr<boost::promise<nlohmann::json>> p =
        boost::make_shared<boost::promise<nlohmann::json>>();
    boost::shared_future<nlohmann::json> f = p->get_future();
    amf_app_inst->add_promise(promise_id, p);

    itti_msg->http_version = 1;
    itti_msg->supi       = nc->imsi;  // TODO: use SUPI in UDR, uc.get()->supi;
    itti_msg->plmn.mcc   = uc.get()->cgi.mcc;
    itti_msg->plmn.mnc   = uc.get()->cgi.mnc;
    itti_msg->promise_id = promise_id;

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N11",
          itti_msg->get_msg_name());
    }

    bool result = false;
    boost::future_status status;
    // wait for timeout or ready
    status = f.wait_for(boost::chrono::milliseconds(FUTURE_STATUS_TIMEOUT_MS));
    if (status == boost::future_status::ready) {
      assert(f.is_ready());
      assert(f.has_value());
      assert(!f.has_exception());
      // Wait for the result from UDM
      nlohmann::json nssai_json = f.get();
      if (!nssai_json.empty()) {
        Logger::amf_n1().debug(
            "Got NSSAI from UDM: %s", nssai_json.dump().c_str());
        try {
          from_json(nssai_json, nssai);
        } catch (std::exception& e) {
          return false;
        }
        return true;
      } else {
        return false;
      }

    } else {
      return false;
    }

  } else {
    // TODO: get from the conf file
    return get_slice_selection_subscription_data_from_conf_file(nc, nssai);
  }
  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::get_slice_selection_subscription_data_from_conf_file(
    const std::shared_ptr<nas_context>& nc, oai::amf::model::Nssai& nssai) {
  Logger::amf_n1().debug(
      "Get the Slice Selection Subscription Data from configuration file");

  // For now, use the common NSSAIs, supported by AMF and gNB, as subscribed
  // NSSAIs

  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(
          nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return false;
  }

  // Get gNB Context
  std::shared_ptr<ue_ngap_context> unc =
      amf_n2_inst->ran_ue_id_2_ue_ngap_context(nc->ran_ue_ngap_id);

  std::shared_ptr<gnb_context> gc = {};
  if (!amf_n2_inst->is_assoc_id_2_gnb_context(unc.get()->gnb_assoc_id)) {
    Logger::amf_n1().error(
        "No existed gNB context with assoc_id (%d)", unc.get()->gnb_assoc_id);
    return false;
  }

  gc = amf_n2_inst->assoc_id_2_gnb_context(unc.get()->gnb_assoc_id);

  // Find the common NSSAIs between Requested NSSAIs and Subscribed NSSAIs
  std::vector<oai::amf::model::Snssai> common_snssais;

  for (auto ta : gc->s_ta_list) {
    for (auto p : ta.b_plmn_list) {
      for (auto s : p.slice_list) {
        oai::amf::model::Snssai nssai = {};
        uint8_t sst                   = 0;
        try {
          sst = std::stoi(s.sst);
        } catch (const std::exception& err) {
          Logger::amf_n1().error("Invalid SST");
          return false;
        }
        nssai.setSst(sst);
        nssai.setSd(s.sd);
        Logger::amf_n1().debug(
            "Added S-NSSAI (SST %d, SD %s)", sst, s.sd.c_str());
        common_snssais.push_back(nssai);
      }
    }
  }

  nssai.setDefaultSingleNssais(common_snssais);

  // Print out the list of subscribed NSSAIs
  for (auto n : nssai.getDefaultSingleNssais()) {
    Logger::amf_n1().debug(
        "Default Single NSSAIs: S-NSSAI (SST %d, SD %s)", n.getSst(),
        n.getSd().c_str());
  }

  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::get_network_slice_selection(
    const std::shared_ptr<nas_context>& nc, const std::string& nf_instance_id,
    const oai::amf::model::SliceInfoForRegistration& slice_info,
    oai::amf::model::AuthorizedNetworkSliceInfo&
        authorized_network_slice_info) {
  Logger::amf_n1().debug(
      "Get the Network Slice Selection Information from NSSF");

  std::shared_ptr<ue_context> uc = {};
  if (!find_ue_context(
          nc.get()->ran_ue_ngap_id, nc.get()->amf_ue_ngap_id, uc)) {
    Logger::amf_n1().warn("Cannot find the UE context");
    return false;
  }

  if (amf_cfg.support_features.enable_external_nssf) {
    // Get Authorized Network Slice Info from an  external NSSF
    std::shared_ptr<itti_n11_network_slice_selection_information> itti_msg =
        std::make_shared<itti_n11_network_slice_selection_information>(
            TASK_AMF_N1, TASK_AMF_N11);

    // Generate a promise and associate this promise to the ITTI message
    uint32_t promise_id = amf_app_inst->generate_promise_id();
    Logger::amf_n1().debug("Promise ID generated %d", promise_id);

    boost::shared_ptr<boost::promise<nlohmann::json>> p =
        boost::make_shared<boost::promise<nlohmann::json>>();
    boost::shared_future<nlohmann::json> f = p->get_future();
    amf_app_inst->add_promise(promise_id, p);

    itti_msg->http_version   = 1;
    itti_msg->nf_instance_id = nf_instance_id;
    itti_msg->slice_info     = slice_info;
    itti_msg->promise_id     = promise_id;
    // itti_msg->plmn.mcc       = uc.get()->cgi.mcc;
    // itti_msg->plmn.mnc       = uc.get()->cgi.mnc;
    itti_msg->tai.plmn.mcc = uc.get()->tai.mcc;
    itti_msg->tai.plmn.mnc = uc.get()->tai.mnc;
    itti_msg->tai.tac      = uc.get()->tai.tac;

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N11",
          itti_msg->get_msg_name());
    }

    bool result                 = false;
    boost::future_status status = {};
    // wait for timeout or ready
    status = f.wait_for(boost::chrono::milliseconds(FUTURE_STATUS_TIMEOUT_MS));
    if (status == boost::future_status::ready) {
      assert(f.is_ready());
      assert(f.has_value());
      assert(!f.has_exception());
      // Wait for the result from NSSF
      nlohmann::json network_slice_info = f.get();
      if (!network_slice_info.empty()) {
        Logger::amf_n1().debug(
            "Got Authorized Network Slice Info from NSSF: %s",
            network_slice_info.dump().c_str());
        try {
          from_json(network_slice_info, authorized_network_slice_info);
        } catch (std::exception& e) {
          Logger::amf_n1().warn(
              "Could not parse Authorized Network Slice Info from Json");
          return false;
        }
      } else {
        Logger::amf_n1().debug(
            "Could not get Authorized Network Slice Info from NSSF");
        return false;
      }

    } else {
      Logger::amf_n1().debug(
          "Could not get Authorized Network Slice Info from NSSF");
      return false;
    }

  } else {
    // TODO: Get Authorized Network Slice Info from local configuration file
    return get_network_slice_selection_from_conf_file(
        nf_instance_id, slice_info, authorized_network_slice_info);
  }
  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::get_network_slice_selection_from_conf_file(
    const std::string& nf_instance_id,
    const oai::amf::model::SliceInfoForRegistration& slice_info,
    oai::amf::model::AuthorizedNetworkSliceInfo& authorized_network_slice_info)
    const {
  Logger::amf_n1().debug(
      "Get the Network Slice Selection Information from configuration file");
  // TODO: Get Authorized Network Slice Info from local configuration file

  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::get_target_amf(
    const std::shared_ptr<nas_context>& nc, std::string& target_amf,
    const oai::amf::model::AuthorizedNetworkSliceInfo&
        authorized_network_slice_info) {
  // Get Target AMF from AuthorizedNetworkSliceInfo
  Logger::amf_n1().debug(
      "Get the list of candidates AMFs from the AuthorizedNetworkSliceInfo and "
      "select the appropriate one");
  std::string target_amf_set = {};
  std::string nrf_amf_set = {};  // The URI of NRF NFDiscovery Service to query
                                 // the list of AMF candidates

  if (authorized_network_slice_info.targetAmfSetIsSet()) {
    target_amf_set = authorized_network_slice_info.getTargetAmfSet();
    Logger::amf_n1().debug(
        "Target AMF Set from NSSF %s", target_amf_set.c_str());
    if (authorized_network_slice_info.nrfAmfSetIsSet()) {
      nrf_amf_set = authorized_network_slice_info.getNrfAmfSet();
      Logger::amf_n1().debug("NRF AMF Set from NSSF %s", nrf_amf_set.c_str());
    }
  } else {
    Logger::amf_n1().warn("No Target AMF Set available");
    return false;
  }

  std::vector<std::string> candidate_amf_list;
  if (authorized_network_slice_info.candidateAmfListIsSet()) {
    candidate_amf_list = authorized_network_slice_info.getCandidateAmfList();
    // TODO:
  }

  if (amf_cfg.support_features
          .enable_smf_selection) {  // TODO: define new option for external NRF
    // use NRF's URI from conf file if not available
    if (nrf_amf_set.empty()) {
      nrf_amf_set = amf_cfg.get_nrf_nf_discovery_service_uri();
      Logger::amf_n1().debug(
          "NRF AMF Set from the configuration file %s", nrf_amf_set.c_str());
    }

    // Get list of AMF candidates from NRF
    std::shared_ptr<itti_n11_nf_instance_discovery> itti_msg =
        std::make_shared<itti_n11_nf_instance_discovery>(
            TASK_AMF_N1, TASK_AMF_N11);

    // Generate a promise and associate this promise to the ITTI message
    uint32_t promise_id = amf_app_inst->generate_promise_id();
    Logger::amf_n1().debug("Promise ID generated %d", promise_id);

    boost::shared_ptr<boost::promise<nlohmann::json>> p =
        boost::make_shared<boost::promise<nlohmann::json>>();
    boost::shared_future<nlohmann::json> f = p->get_future();
    amf_app_inst->add_promise(promise_id, p);

    itti_msg->http_version          = 1;
    itti_msg->target_amf_set        = target_amf_set;
    itti_msg->target_amf_set_is_set = true;
    itti_msg->promise_id            = promise_id;
    itti_msg->target_nf_type        = "AMF";
    itti_msg->nrf_amf_set           = nrf_amf_set;

    int ret = itti_inst->send_msg(itti_msg);
    if (0 != ret) {
      Logger::amf_n1().error(
          "Could not send ITTI message %s to task TASK_AMF_N11",
          itti_msg->get_msg_name());
    }

    bool result                 = false;
    boost::future_status status = {};
    // wait for timeout or ready
    status = f.wait_for(boost::chrono::milliseconds(FUTURE_STATUS_TIMEOUT_MS));
    if (status == boost::future_status::ready) {
      assert(f.is_ready());
      assert(f.has_value());
      assert(!f.has_exception());
      // Wait for the result from NRF
      nlohmann::json amf_candidate_list = f.get();
      if (!amf_candidate_list.empty()) {
        Logger::amf_n1().debug(
            "Got List of AMF candidates from NRF: %s",
            amf_candidate_list.dump().c_str());
        // TODO: Select an AMF from the candidate list
        if (!select_target_amf(nc, target_amf, amf_candidate_list)) {
          Logger::amf_n1().debug(
              "Could not select an appropriate AMF from the AMF candidates");
          return false;
        } else {
          Logger::amf_n1().debug("Candidate AMF: %s", target_amf.c_str());
          return true;
        }

      } else {
        Logger::amf_n1().debug("Could not get List of AMF candidates from NRF");
        return false;
      }

    } else {
      Logger::amf_n1().debug("Could not get List of AMF candidates from NRF");
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::select_target_amf(
    const std::shared_ptr<nas_context>& nc, std::string& target_amf,
    const nlohmann::json& amf_candidate_list) {
  Logger::amf_n1().debug(
      "Select the appropriate AMF from the list of candidates");
  bool result = false;
  // Process data to obtain the target AMF
  if (amf_candidate_list.find("nfInstances") != amf_candidate_list.end()) {
    for (auto& it : amf_candidate_list["nfInstances"].items()) {
      nlohmann::json instance_json = it.value();
      // TODO: do we need to check with sNSSAI?
      if (instance_json.find("sNssais") != instance_json.end()) {
        // Each S-NSSAI in the Default Single NSSAIs must be in the AMF's Slice
        // List
        for (auto& s : instance_json["sNssais"].items()) {
          nlohmann::json Snssai = s.value();  // TODO: validate NSSAIs
        }
      }
      // for now, just IP addr of AMF of the first NF instance
      if (instance_json.find("ipv4Addresses") != instance_json.end()) {
        if (instance_json["ipv4Addresses"].size() > 0)
          target_amf = instance_json["ipv4Addresses"].at(0).get<std::string>();
        result = true;
        break;
      }
    }
  }
  return result;
}

//------------------------------------------------------------------------------
void amf_n1::send_n1_message_notity(
    const std::shared_ptr<nas_context>& nc,
    const std::string& target_amf) const {
  Logger::amf_n1().debug(
      "Send a request to N11 to send N1 Message Notify to the target AMF");

  std::shared_ptr<itti_n11_n1_message_notify> itti_msg =
      std::make_shared<itti_n11_n1_message_notify>(TASK_AMF_N1, TASK_AMF_N11);

  itti_msg->http_version = 1;
  if (nc->registration_request_is_set) {
    itti_msg->registration_request = nc->registration_request;
  }
  itti_msg->target_amf_uri = target_amf;
  itti_msg->supi           = nc->imsi;

  int ret = itti_inst->send_msg(itti_msg);
  if (0 != ret) {
    Logger::amf_n1().error(
        "Could not send ITTI message %s to task TASK_AMF_N11",
        itti_msg->get_msg_name());
  }
}

//------------------------------------------------------------------------------
bool amf_n1::reroute_nas_via_an(
    const std::shared_ptr<nas_context>& nc, const std::string& target_amf_set) {
  Logger::amf_n1().debug(
      "Send a request to Reroute NAS message to the target AMF via AN");

  uint16_t amf_set_id = 0;
  if (!get_amf_set_id(target_amf_set, amf_set_id)) {
    Logger::amf_n1().warn("Could not extract AMF Set ID from Target AMF Set");
    return false;
  }

  std::shared_ptr<itti_rereoute_nas> itti_msg =
      std::make_shared<itti_rereoute_nas>(TASK_AMF_N1, TASK_AMF_N2);
  itti_msg->ran_ue_ngap_id = nc->ran_ue_ngap_id;
  itti_msg->amf_ue_ngap_id = nc->amf_ue_ngap_id;
  itti_msg->amf_set_id     = amf_set_id;

  int ret = itti_inst->send_msg(itti_msg);
  if (0 != ret) {
    Logger::amf_n1().error(
        "Could not send ITTI message %s to task TASK_AMF_N2",
        itti_msg->get_msg_name());
  }

  return true;
}

//------------------------------------------------------------------------------
bool amf_n1::get_amf_set_id(
    const std::string& target_amf_set, uint16_t& amf_set_id) {
  std::vector<std::string> words;
  boost::split(
      words, target_amf_set, boost::is_any_of("/"), boost::token_compress_on);
  if (words.size() != 4) {
    Logger::amf_n1().warn(
        "Bad value for Target AMF Set  %s", target_amf_set.c_str());
    return false;
  }
  if (words[3].size() != 3) {
    Logger::amf_n1().warn(
        "Bad value for Target AMF Set  %s", target_amf_set.c_str());
    return false;
  } else {
    try {
      amf_set_id = std::stoul(words[3].substr(0, 1), nullptr, 16)
                   << 8 + std::stoul(words[3].substr(1, 2), nullptr, 16);
    } catch (const std::exception& e) {
      Logger::amf_n1().warn(
          "Error when converting from string to int for AMF Set ID, "
          "error: %s",
          e.what());
    }
  }

  return true;
}
