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

/*! \file amf_app.cpp
 \brief
 \author  Keliang DU (BUPT), Tien-Thinh NGUYEN (EURECOM)
 \date 2020
 \email: contact@openairinterface.org
 */

#include "amf_app.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "DLNASTransport.hpp"
#include "amf_config.hpp"
#include "amf_n1.hpp"
#include "amf_n11.hpp"
#include "amf_n2.hpp"
#include "amf_statistics.hpp"
#include "itti.hpp"
#include "ngap_app.hpp"
#include "comUt.hpp"
#include "RegistrationContextContainer.h"
#include "GlobalRanNodeId.h"
#include "UeN1N2InfoSubscriptionCreatedData.h"

using namespace ngap;
using namespace nas;
using namespace amf_application;
using namespace config;

extern amf_app* amf_app_inst;
extern itti_mw* itti_inst;
amf_n2* amf_n2_inst   = nullptr;
amf_n1* amf_n1_inst   = nullptr;
amf_n11* amf_n11_inst = nullptr;
extern amf_config amf_cfg;
extern statistics stacs;

void amf_app_task(void*);

//------------------------------------------------------------------------------
amf_app::amf_app(const amf_config& amf_cfg)
    : m_amf_ue_ngap_id2ue_ctx(),
      m_ue_ctx_key(),
      m_supi2ue_ctx(),
      m_curl_handle_responses_n2_sm() {
  amf_ue_ngap_id2ue_ctx       = {};
  ue_ctx_key                  = {};
  supi2ue_ctx                 = {};
  curl_handle_responses_n2_sm = {};
  Logger::amf_app().startup("Creating AMF application functionality layer");
  if (itti_inst->create_task(TASK_AMF_APP, amf_app_task, nullptr)) {
    Logger::amf_app().error("Cannot create task TASK_AMF_APP");
    throw std::runtime_error("Cannot create task TASK_AMF_APP");
  }

  try {
    amf_n1_inst = new amf_n1();
    amf_n2_inst =
        new amf_n2(std::string(inet_ntoa(amf_cfg.n2.addr4)), amf_cfg.n2.port);
    amf_n11_inst = new amf_n11();
  } catch (std::exception& e) {
    Logger::amf_app().error("Cannot create AMF APP: %s", e.what());
    throw;
  }

  // Generate an AMF profile (including NF instance)
  generate_amf_profile();

  // Register to NRF if needed
  if (amf_cfg.support_features.enable_nf_registration and
      amf_cfg.support_features.enable_external_nrf)
    register_to_nrf();

  timer_id_t tid = itti_inst->timer_setup(
      amf_cfg.statistics_interval, 0, TASK_AMF_APP,
      TASK_AMF_APP_PERIODIC_STATISTICS, 0);
  Logger::amf_app().startup("Started timer (%d)", tid);
}

//------------------------------------------------------------------------------
void amf_app::allRegistredModulesInit(const amf_modules& modules) {
  Logger::amf_app().info("Initiating all registered modules");
}

//------------------------------------------------------------------------------
void amf_app_task(void*) {
  const task_id_t task_id = TASK_AMF_APP;
  itti_inst->notify_task_ready(task_id);
  do {
    std::shared_ptr<itti_msg> shared_msg = itti_inst->receive_msg(task_id);
    auto* msg                            = shared_msg.get();
    timer_id_t tid;
    switch (msg->msg_type) {
      case NAS_SIG_ESTAB_REQ: {
        Logger::amf_app().debug("Received NAS_SIG_ESTAB_REQ");
        itti_nas_signalling_establishment_request* m =
            dynamic_cast<itti_nas_signalling_establishment_request*>(msg);
        amf_app_inst->handle_itti_message(ref(*m));
      } break;

      case N1N2_MESSAGE_TRANSFER_REQ: {
        Logger::amf_app().debug("Received N1N2_MESSAGE_TRANSFER_REQ");
        itti_n1n2_message_transfer_request* m =
            dynamic_cast<itti_n1n2_message_transfer_request*>(msg);
        amf_app_inst->handle_itti_message(ref(*m));
      } break;

      case SBI_N1_MESSAGE_NOTIFICATION: {
        Logger::amf_app().debug("Received SBI_N1_MESSAGE_NOTIFICATION");
        itti_sbi_n1_message_notification* m =
            dynamic_cast<itti_sbi_n1_message_notification*>(msg);
        amf_app_inst->handle_itti_message(ref(*m));
      } break;

      case SBI_N1N2_MESSAGE_SUBSCRIBE: {
        Logger::amf_app().debug("Received SBI_N1N2_MESSAGE_SUBSCRIBE");
        itti_sbi_n1n2_message_subscribe* m =
            dynamic_cast<itti_sbi_n1n2_message_subscribe*>(msg);
        amf_app_inst->handle_itti_message(ref(*m));
      } break;

      case SBI_N1N2_MESSAGE_UNSUBSCRIBE: {
        Logger::amf_app().debug("Received SBI_N1N2_MESSAGE_UNSUBSCRIBE");
        itti_sbi_n1n2_message_unsubscribe* m =
            dynamic_cast<itti_sbi_n1n2_message_unsubscribe*>(msg);
        amf_app_inst->handle_itti_message(ref(*m));
      } break;

      case TIME_OUT: {
        if (itti_msg_timeout* to = dynamic_cast<itti_msg_timeout*>(msg)) {
          switch (to->arg1_user) {
            case TASK_AMF_APP_PERIODIC_STATISTICS:
              tid = itti_inst->timer_setup(
                  amf_cfg.statistics_interval, 0, TASK_AMF_APP,
                  TASK_AMF_APP_PERIODIC_STATISTICS, 0);
              stacs.display();
              break;
            default:
              Logger::amf_app().info(
                  "No handler for timer(%d) with arg1_user(%d) ", to->timer_id,
                  to->arg1_user);
          }
        }
      } break;

      case TERMINATE: {
        if (itti_msg_terminate* terminate =
                dynamic_cast<itti_msg_terminate*>(msg)) {
          Logger::amf_n2().info("Received terminate message");
          return;
        }
      } break;
      default:
        Logger::amf_app().info("No handler for msg type %d", msg->msg_type);
    }
  } while (true);
}

//------------------------------------------------------------------------------
long amf_app::generate_amf_ue_ngap_id() {
  long tmp = 0;
  tmp      = __sync_fetch_and_add(&amf_app_ue_ngap_id_generator, 1);
  return tmp & 0xffffffffff;
}

//------------------------------------------------------------------------------
bool amf_app::is_amf_ue_id_2_ue_context(const long& amf_ue_ngap_id) const {
  std::shared_lock lock(m_amf_ue_ngap_id2ue_ctx);
  return bool{amf_ue_ngap_id2ue_ctx.count(amf_ue_ngap_id) > 0};
}

//------------------------------------------------------------------------------
std::shared_ptr<ue_context> amf_app::amf_ue_id_2_ue_context(
    const long& amf_ue_ngap_id) const {
  std::shared_lock lock(m_amf_ue_ngap_id2ue_ctx);
  return amf_ue_ngap_id2ue_ctx.at(amf_ue_ngap_id);
}

//------------------------------------------------------------------------------
void amf_app::set_amf_ue_ngap_id_2_ue_context(
    const long& amf_ue_ngap_id, const std::shared_ptr<ue_context>& uc) {
  std::unique_lock lock(m_amf_ue_ngap_id2ue_ctx);
  amf_ue_ngap_id2ue_ctx[amf_ue_ngap_id] = uc;
}

//------------------------------------------------------------------------------
bool amf_app::is_ran_amf_id_2_ue_context(const string& ue_context_key) const {
  std::shared_lock lock(m_ue_ctx_key);
  return bool{ue_ctx_key.count(ue_context_key) > 0};
}

//------------------------------------------------------------------------------
std::shared_ptr<ue_context> amf_app::ran_amf_id_2_ue_context(
    const string& ue_context_key) const {
  std::shared_lock lock(m_ue_ctx_key);
  return ue_ctx_key.at(ue_context_key);
}

//------------------------------------------------------------------------------
bool amf_app::ran_amf_id_2_ue_context(
    const std::string& ue_context_key, std::shared_ptr<ue_context>& uc) const {
  std::shared_lock lock(m_ue_ctx_key);
  if (ue_ctx_key.count(ue_context_key) > 0) {
    uc = ue_ctx_key.at(ue_context_key);
    if (uc.get() == nullptr) return false;
    return true;
  } else
    return false;
}

//------------------------------------------------------------------------------
void amf_app::set_ran_amf_id_2_ue_context(
    const string& ue_context_key, const std::shared_ptr<ue_context>& uc) {
  std::unique_lock lock(m_ue_ctx_key);
  ue_ctx_key[ue_context_key] = uc;
}

//------------------------------------------------------------------------------
bool amf_app::is_supi_2_ue_context(const string& supi) const {
  std::shared_lock lock(m_supi2ue_ctx);
  return bool{supi2ue_ctx.count(supi) > 0};
}

//------------------------------------------------------------------------------
std::shared_ptr<ue_context> amf_app::supi_2_ue_context(
    const string& supi) const {
  std::shared_lock lock(m_supi2ue_ctx);
  return supi2ue_ctx.at(supi);
}

//------------------------------------------------------------------------------
void amf_app::set_supi_2_ue_context(
    const string& supi, const std::shared_ptr<ue_context>& uc) {
  std::unique_lock lock(m_supi2ue_ctx);
  supi2ue_ctx[supi] = uc;
}

//------------------------------------------------------------------------------
bool amf_app::find_pdu_session_context(
    const string& supi, const std::uint8_t pdu_session_id,
    std::shared_ptr<pdu_session_context>& psc) {
  if (!is_supi_2_ue_context(supi)) return false;
  std::shared_ptr<ue_context> uc = {};
  uc                             = supi_2_ue_context(supi);
  if (!uc.get()->find_pdu_session_context(pdu_session_id, psc)) return false;
  return true;
}

//------------------------------------------------------------------------------
bool amf_app::get_pdu_sessions_context(
    const string& supi,
    std::vector<std::shared_ptr<pdu_session_context>>& sessions_ctx) {
  if (!is_supi_2_ue_context(supi)) return false;
  std::shared_ptr<ue_context> uc = {};
  uc                             = supi_2_ue_context(supi);
  if (!uc.get()->get_pdu_sessions_context(sessions_ctx)) return false;
  return true;
}

//------------------------------------------------------------------------------
evsub_id_t amf_app::generate_ev_subscription_id() {
  return evsub_id_generator.get_uid();
}

//------------------------------------------------------------------------------
n1n2sub_id_t amf_app::generate_n1n2_message_subscription_id() {
  return n1n2sub_id_generator.get_uid();
}

//------------------------------------------------------------------------------
void amf_app::handle_itti_message(
    itti_n1n2_message_transfer_request& itti_msg) {
  if (itti_msg.is_ppi_set) {  // Paging procedure
    Logger::amf_app().info(
        "Handle ITTI N1N2 Message Transfer Request for Paging");
    std::shared_ptr<itti_paging> i =
        std::make_shared<itti_paging>(TASK_AMF_APP, TASK_AMF_N2);
    amf_n1_inst->supi_2_amf_id(itti_msg.supi, i.get()->amf_ue_ngap_id);
    amf_n1_inst->supi_2_ran_id(itti_msg.supi, i.get()->ran_ue_ngap_id);

    int ret = itti_inst->send_msg(i);
    if (0 != ret) {
      Logger::amf_app().error(
          "Could not send ITTI message %s to task TASK_AMF_N2",
          i->get_msg_name());
    }
  } else {
    Logger::amf_app().info("Handle ITTI N1N2 Message Transfer Request");
    // Encode DL NAS TRANSPORT message(NAS message)
    DLNASTransport* dl = new DLNASTransport();
    dl->setHeader(PLAIN_5GS_MSG);
    dl->setPayload_Container_Type(N1_SM_INFORMATION);
    dl->setPayload_Container(
        (uint8_t*) bdata(itti_msg.n1sm), blength(itti_msg.n1sm));
    dl->setPDUSessionId(itti_msg.pdu_session_id);

    uint8_t nas[BUFFER_SIZE_1024];
    int encoded_size = dl->encode2buffer(nas, BUFFER_SIZE_1024);
    comUt::print_buffer("amf_app", "n1n2 transfer", nas, encoded_size);
    bstring dl_nas = blk2bstr(nas, encoded_size);

    itti_downlink_nas_transfer* dl_msg =
        new itti_downlink_nas_transfer(TASK_AMF_APP, TASK_AMF_N1);
    dl_msg->dl_nas = dl_nas;
    if (!itti_msg.is_n2sm_set) {
      dl_msg->is_n2sm_set = false;
    } else {
      dl_msg->n2sm           = itti_msg.n2sm;
      dl_msg->pdu_session_id = itti_msg.pdu_session_id;
      dl_msg->is_n2sm_set    = true;
      dl_msg->n2sm_info_type = itti_msg.n2sm_info_type;
    }
    amf_n1_inst->supi_2_amf_id(itti_msg.supi, dl_msg->amf_ue_ngap_id);
    amf_n1_inst->supi_2_ran_id(itti_msg.supi, dl_msg->ran_ue_ngap_id);
    std::shared_ptr<itti_downlink_nas_transfer> i =
        std::shared_ptr<itti_downlink_nas_transfer>(dl_msg);
    int ret = itti_inst->send_msg(i);
    if (0 != ret) {
      Logger::amf_app().error(
          "Could not send ITTI message %s to task TASK_AMF_N1",
          i->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_app::handle_itti_message(
    itti_nas_signalling_establishment_request& itti_msg) {
  // 1. Generate amf_ue_ngap_id
  // 2. Create UE Context and store related information information
  // 3. Send nas-pdu to task_amf_n1

  long amf_ue_ngap_id = 0;
  std::shared_ptr<ue_context> uc;

  // Check UE Context with 5g-s-tmsi
  if ((amf_ue_ngap_id = itti_msg.amf_ue_ngap_id) == -1) {
    amf_ue_ngap_id = generate_amf_ue_ngap_id();
  }

  string ue_context_key = "app_ue_ranid_" + to_string(itti_msg.ran_ue_ngap_id) +
                          ":amfid_" + to_string(amf_ue_ngap_id);
  if (!is_ran_amf_id_2_ue_context(ue_context_key)) {
    Logger::amf_app().debug(
        "No existing UE Context, Create a new one with ran_amf_id %s",
        ue_context_key.c_str());
    uc = std::shared_ptr<ue_context>(new ue_context());
    set_ran_amf_id_2_ue_context(ue_context_key, uc);
  }

  // Update AMF UE NGAP ID
  std::shared_ptr<ue_ngap_context> unc = {};
  if (!amf_n2_inst->is_ran_ue_id_2_ue_ngap_context(itti_msg.ran_ue_ngap_id)) {
    Logger::amf_n1().error(
        "Could not find UE NGAP Context with ran_ue_ngap_id "
        "(" GNB_UE_NGAP_ID_FMT ")",
        itti_msg.ran_ue_ngap_id);
  } else {
    unc = amf_n2_inst->ran_ue_id_2_ue_ngap_context(itti_msg.ran_ue_ngap_id);
    unc.get()->amf_ue_ngap_id = amf_ue_ngap_id;
    amf_n2_inst->set_amf_ue_ngap_id_2_ue_ngap_context(amf_ue_ngap_id, unc);
  }

  if (uc.get() == nullptr) {
    Logger::amf_app().error(
        "Failed to create ue_context with ran_amf_id %s",
        ue_context_key.c_str());
  } else {
    uc.get()->cgi = itti_msg.cgi;
    uc.get()->tai = itti_msg.tai;
    if (itti_msg.rrc_cause != -1)
      uc.get()->rrc_estb_cause =
          (e_Ngap_RRCEstablishmentCause) itti_msg.rrc_cause;
    if (itti_msg.ueCtxReq == -1)
      uc.get()->isUeContextRequest = false;
    else
      uc.get()->isUeContextRequest = true;
    uc.get()->ran_ue_ngap_id = itti_msg.ran_ue_ngap_id;
    uc.get()->amf_ue_ngap_id = amf_ue_ngap_id;

    std::string guti;
    bool is_guti_valid = false;
    if (itti_msg.is_5g_s_tmsi_present) {
      guti = itti_msg.tai.mcc + itti_msg.tai.mnc + amf_cfg.guami.regionID +
             itti_msg._5g_s_tmsi;
      is_guti_valid = true;
      Logger::amf_app().debug("Receiving GUTI %s", guti.c_str());
    }

    itti_uplink_nas_data_ind* itti_n1_msg =
        new itti_uplink_nas_data_ind(TASK_AMF_APP, TASK_AMF_N1);
    itti_n1_msg->amf_ue_ngap_id              = amf_ue_ngap_id;
    itti_n1_msg->ran_ue_ngap_id              = itti_msg.ran_ue_ngap_id;
    itti_n1_msg->is_nas_signalling_estab_req = true;
    itti_n1_msg->nas_msg                     = itti_msg.nas_buf;
    itti_n1_msg->mcc                         = itti_msg.tai.mcc;
    itti_n1_msg->mnc                         = itti_msg.tai.mnc;
    itti_n1_msg->is_guti_valid               = is_guti_valid;
    if (is_guti_valid) {
      itti_n1_msg->guti = guti;
    }
    std::shared_ptr<itti_uplink_nas_data_ind> i =
        std::shared_ptr<itti_uplink_nas_data_ind>(itti_n1_msg);
    int ret = itti_inst->send_msg(i);
    if (0 != ret) {
      Logger::amf_app().error(
          "Could not send ITTI message %s to task TASK_AMF_N1",
          i->get_msg_name());
    }
  }
}

//------------------------------------------------------------------------------
void amf_app::handle_itti_message(itti_sbi_n1_message_notification& itti_msg) {
  Logger::amf_app().info(
      "Target AMF, handling a N1 Message Notification from the initial AMF");

  // Step 1. Get UE, gNB related information

  // Get NAS message (RegistrationRequest, this message included
  // in N1 Message Notify is actually is RegistrationRequest from UE to the
  // initial AMF)
  bstring n1sm;
  conv::msg_str_2_msg_hex(itti_msg.n1sm, n1sm);

  // get RegistrationContextContainer including gNB info
  // UE context information, N1 message from UE, AN address
  oai::amf::model::RegistrationContextContainer registration_context =
      itti_msg.notification_msg.getRegistrationCtxtContainer();

  // Step 2. Create gNB context if necessary

  // TODO: How to get SCTP-related information and establish a new SCTP
  // connection between the Target AMF and gNB?
  std::shared_ptr<gnb_context> gc = {};

  // GlobalRAN Node ID (~in NGSetupRequest)
  oai::amf::model::GlobalRanNodeId ran_node_id =
      registration_context.getRanNodeId();
  // RAN UE NGAP ID
  uint32_t ran_ue_ngap_id = registration_context.getAnN2ApId();

  if (ran_node_id.gNbIdIsSet()) {
    oai::amf::model::GNbId gnb_id_model = ran_node_id.getGNbId();
    uint32_t gnb_id                     = {};

    try {
      gnb_id = std::stoul(gnb_id_model.getGNBValue(), nullptr, 10);
    } catch (const std::exception& e) {
      Logger::amf_app().warn(
          "Error when converting from string to uint32_t for gNB Value: %s",
          e.what());
      return;
    }

    gc->globalRanNodeId = gnb_id;
    // TODO:   gc->gnb_name
    // TODO: DefaultPagingDRX
    // TODO: Supported TA List
    amf_n2_inst->set_gnb_id_2_gnb_context(gnb_id, gc);
  }

  // UserLocation getUserLocation()
  // std::string getAnN2IPv4Addr()
  // AllowedNssai getAllowedNssai()
  // std::vector<ConfiguredSnssai>& getConfiguredNssai();
  // rejectedNssaiInPlmn
  // rejectedNssaiInTa
  // std::string getInitialAmfName()

  // Step 3. Create UE Context
  oai::amf::model::UeContext ue_ctx = registration_context.getUeContext();
  std::string supi                  = {};
  std::shared_ptr<ue_context> uc    = {};

  if (ue_ctx.supiIsSet()) {
    supi = ue_ctx.getSupi();
    if (!is_supi_2_ue_context(supi)) {
      // Create a new UE Context
      Logger::amf_app().debug(
          "No existing UE Context, Create a new one with SUPI %s",
          supi.c_str());
      uc                 = std::shared_ptr<ue_context>(new ue_context());
      uc->amf_ue_ngap_id = -1;
      uc->supi           = supi;
      set_supi_2_ue_context(supi, uc);
    } else {  // Update UE Context
      uc = supi_2_ue_context(supi);
    }
  }

  // TODO: 5gMmCapability
  // TODO: MmContext
  // TODO: PduSessionContext

  /*
  if (!is_supi_2_ue_context(itti_msg.ue_id)) {
    // TODO: Create a new UE Context
  } else {  // Update UE Context
    uc = supi_2_ue_context(itti_msg.ue_id);
  }
*/

  long amf_ue_ngap_id = -1;
  // Generate AMF UE NGAP ID if necessary
  if (!uc.get()) {  // No UE context existed
    amf_ue_ngap_id = generate_amf_ue_ngap_id();
  } else {
    if ((amf_ue_ngap_id = uc->amf_ue_ngap_id) == -1) {
      amf_ue_ngap_id = generate_amf_ue_ngap_id();
    }
  }

  string ue_context_key = "app_ue_ranid_" + to_string(ran_ue_ngap_id) +
                          ":amfid_" + to_string(amf_ue_ngap_id);
  if (!is_ran_amf_id_2_ue_context(ue_context_key)) {
    Logger::amf_app().debug(
        "No existing UE Context associated with UE Context Key %s",
        ue_context_key.c_str());
    if (!uc.get()) {
      // Create a new UE Context
      Logger::amf_app().debug(
          "Create a new UE Context with UE Context Key",
          ue_context_key.c_str());
      uc = std::shared_ptr<ue_context>(new ue_context());
    }
    set_ran_amf_id_2_ue_context(ue_context_key, uc);
  } else {
    uc = ran_amf_id_2_ue_context(ue_context_key);
  }
  // Return if UE Context is still invalid
  if (!uc.get()) {
    Logger::amf_app().error("Failed to get UE Context");
    return;
  }

  // Update info for UE context
  uc.get()->amf_ue_ngap_id = amf_ue_ngap_id;
  uc.get()->ran_ue_ngap_id = ran_ue_ngap_id;
  // RrcEstCause
  if (registration_context.rrcEstCauseIsSet()) {
    uint8_t rrc_cause = {};
    try {
      rrc_cause =
          std::stoul(registration_context.getRrcEstCause(), nullptr, 10);
    } catch (const std::exception& e) {
      Logger::amf_app().warn(
          "Error when converting from string to int for RrcEstCause: %s",
          e.what());
      rrc_cause = 0;
    }

    uc.get()->rrc_estb_cause = (e_Ngap_RRCEstablishmentCause) rrc_cause;
  }
  // ueContextRequest
  uc.get()->isUeContextRequest = registration_context.isUeContextRequest();

  // Step 4. Create UE NGAP Context if necessary
  // Create/Update UE NGAP Context
  std::shared_ptr<ue_ngap_context> unc = {};
  if (!amf_n2_inst->is_ran_ue_id_2_ue_ngap_context(ran_ue_ngap_id)) {
    Logger::amf_app().debug(
        "Create a new UE NGAP context with ran_ue_ngap_id " GNB_UE_NGAP_ID_FMT,
        ran_ue_ngap_id);
    unc = std::shared_ptr<ue_ngap_context>(new ue_ngap_context());
    amf_n2_inst->set_ran_ue_ngap_id_2_ue_ngap_context(ran_ue_ngap_id, unc);
  } else {
    unc = amf_n2_inst->ran_ue_id_2_ue_ngap_context(ran_ue_ngap_id);
    if (!unc.get()) {
      Logger::amf_app().error(
          "Failed to get UE NGAP context for "
          "ran_ue_ngap_id " GNB_UE_NGAP_ID_FMT,
          ran_ue_ngap_id);
      return;
    }
  }

  // Store related information into UE NGAP context
  unc.get()->ran_ue_ngap_id = ran_ue_ngap_id;
  // TODO:  unc.get()->sctp_stream_recv
  // TODO: unc.get()->sctp_stream_send
  // TODO: gc.get()->next_sctp_stream
  // TODO: unc.get()->gnb_assoc_id
  // TODO: unc.get()->tai

  // Step 5. Trigger the procedure following RegistrationRequest

  itti_uplink_nas_data_ind* itti_n1_msg =
      new itti_uplink_nas_data_ind(TASK_AMF_APP, TASK_AMF_N1);
  itti_n1_msg->amf_ue_ngap_id              = amf_ue_ngap_id;
  itti_n1_msg->ran_ue_ngap_id              = ran_ue_ngap_id;
  itti_n1_msg->is_nas_signalling_estab_req = true;
  itti_n1_msg->nas_msg                     = n1sm;
  itti_n1_msg->mcc                         = ran_node_id.getPlmnId().getMcc();
  itti_n1_msg->mnc                         = ran_node_id.getPlmnId().getMnc();
  itti_n1_msg->is_guti_valid               = false;

  std::shared_ptr<itti_uplink_nas_data_ind> i =
      std::shared_ptr<itti_uplink_nas_data_ind>(itti_n1_msg);
  int ret = itti_inst->send_msg(i);
  if (0 != ret) {
    Logger::amf_app().error(
        "Could not send ITTI message %s to task TASK_AMF_N1",
        i->get_msg_name());
  }

  return;
}

//------------------------------------------------------------------------------
void amf_app::handle_itti_message(itti_sbi_n1n2_message_subscribe& itti_msg) {
  Logger::amf_app().info(
      "Handle an N1N2MessageSubscribe from a NF (HTTP version "
      "%d)",
      itti_msg.http_version);

  // Generate a subscription ID Id and store the corresponding information in a
  // map (subscription id, info)
  n1n2sub_id_t n1n2sub_id = generate_n1n2_message_subscription_id();
  std::shared_ptr<oai::amf::model::UeN1N2InfoSubscriptionCreateData>
      subscription_data =
          std::make_shared<oai::amf::model::UeN1N2InfoSubscriptionCreateData>(
              itti_msg.subscription_data);
  add_n1n2_message_subscription(
      itti_msg.ue_cxt_id, n1n2sub_id, subscription_data);

  std::string location =
      amf_cfg.get_amf_n1n2_message_subscribe_uri(itti_msg.ue_cxt_id) + "/" +
      std::to_string((uint32_t) n1n2sub_id);

  // Trigger the response from AMF API Server
  oai::amf::model::UeN1N2InfoSubscriptionCreatedData created_data = {};
  created_data.setN1n2NotifySubscriptionId(
      std::to_string((uint32_t) n1n2sub_id));
  nlohmann::json created_data_json = {};
  to_json(created_data_json, created_data);

  nlohmann::json response_data      = {};
  response_data["createdData"]      = created_data;
  response_data["httpResponseCode"] = 201;  // TODO:
  response_data["location"]         = location;

  // Notify to the result
  if (itti_msg.promise_id > 0) {
    trigger_process_response(itti_msg.promise_id, response_data);
    return;
  }
}

//------------------------------------------------------------------------------
void amf_app::handle_itti_message(itti_sbi_n1n2_message_unsubscribe& itti_msg) {
  Logger::amf_app().info(
      "Handle an N1N2MessageUnSubscribe from a NF (HTTP version "
      "%d)",
      itti_msg.http_version);

  // Process the request and trigger the response from AMF API Server
  nlohmann::json response_data = {};
  if (remove_n1n2_message_subscription(
          itti_msg.ue_cxt_id, itti_msg.subscription_id)) {
    response_data["httpResponseCode"] = 204;  // TODO:
  } else {
    response_data["httpResponseCode"]               = 400;  // TODO:
    oai::amf::model::ProblemDetails problem_details = {};
    // TODO set problem_details
    to_json(response_data["ProblemDetails"], problem_details);
  }

  // Notify to the result
  if (itti_msg.promise_id > 0) {
    trigger_process_response(itti_msg.promise_id, response_data);
    return;
  }
}

//---------------------------------------------------------------------------------------------
void amf_app::add_n1n2_message_subscription(
    const std::string& ue_ctx_id, const n1n2sub_id_t& sub_id,
    std::shared_ptr<oai::amf::model::UeN1N2InfoSubscriptionCreateData>&
        subscription_data) {
  Logger::amf_app().debug("Add an N1N2 Message Subscribe (Sub ID %d)", sub_id);
  std::unique_lock lock(m_n1n2_message_subscribe);
  n1n2_message_subscribe.emplace(
      std::make_pair(ue_ctx_id, sub_id), subscription_data);
}

//---------------------------------------------------------------------------------------------
bool amf_app::remove_n1n2_message_subscription(
    const std::string& ue_ctx_id, const std::string& sub_id) {
  Logger::amf_app().debug(
      "Remove an N1N2 Message Subscribe (UE Context ID %s, Sub ID %s)",
      ue_ctx_id.c_str(), sub_id.c_str());

  // Verify Subscription ID
  n1n2sub_id_t n1n2sub_id = {};
  try {
    n1n2sub_id = std::stoi(sub_id);
  } catch (const std::exception& err) {
    Logger::amf_app().warn(
        "Received a Unsubscribe Request, couldn't find the corresponding "
        "subscription");
    return false;
  }

  // Remove from the list
  std::unique_lock lock(m_n1n2_message_subscribe);
  if (n1n2_message_subscribe.erase(std::make_pair(ue_ctx_id, n1n2sub_id)) == 1)
    return true;
  else
    return false;
  return true;
}

//------------------------------------------------------------------------------
uint32_t amf_app::generate_tmsi() {
  return tmsi_generator.get_uid();
}

//------------------------------------------------------------------------------
bool amf_app::generate_5g_guti(
    const uint32_t ranid, const long amfid, string& mcc, string& mnc,
    uint32_t& tmsi) {
  string ue_context_key =
      "app_ue_ranid_" + to_string(ranid) + ":amfid_" + to_string(amfid);
  if (!is_ran_amf_id_2_ue_context(ue_context_key)) {
    Logger::amf_app().error(
        "No UE context for ran_amf_id %s, exit", ue_context_key.c_str());
    return false;
  }
  std::shared_ptr<ue_context> uc = {};
  uc                             = ran_amf_id_2_ue_context(ue_context_key);
  mcc                            = uc.get()->tai.mcc;
  mnc                            = uc.get()->tai.mnc;
  tmsi                           = generate_tmsi();
  uc.get()->tmsi                 = tmsi;
  return true;
}

//------------------------------------------------------------------------------
evsub_id_t amf_app::handle_event_exposure_subscription(
    std::shared_ptr<itti_sbi_event_exposure_request> msg) {
  Logger::amf_app().info(
      "Handle an Event Exposure Subscription Request from a NF (HTTP version "
      "%d)",
      msg->http_version);

  // Generate a subscription ID Id and store the corresponding information in a
  // map (subscription id, info)
  evsub_id_t evsub_id = generate_ev_subscription_id();

  std::vector<amf_event_t> event_subscriptions =
      msg->event_exposure.get_event_subs();

  // store subscription
  for (auto i : event_subscriptions) {
    std::shared_ptr<amf_subscription> ss = std::make_shared<amf_subscription>();
    ss.get()->sub_id                     = evsub_id;
    // TODO:
    if (msg->event_exposure.is_supi_is_set()) {
      ss.get()->supi        = msg->event_exposure.get_supi();
      ss.get()->supi_is_set = true;
    }
    ss.get()->notify_correlation_id =
        msg->event_exposure.get_notify_correlation_id();
    ss.get()->notify_uri = msg->event_exposure.get_notify_uri();
    ss.get()->nf_id      = msg->event_exposure.get_nf_id();
    ss.get()->ev_type    = i.type;
    add_event_subscription(evsub_id, i.type, ss);
    ss.get()->display();
  }
  return evsub_id;
}

//---------------------------------------------------------------------------------------------
bool amf_app::handle_event_exposure_delete(const std::string& subscription_id) {
  // verify Subscription ID
  evsub_id_t sub_id = {};
  try {
    sub_id = std::stoi(subscription_id);
  } catch (const std::exception& err) {
    Logger::amf_app().warn(
        "Received a Unsubscribe Request, couldn't find the corresponding "
        "subscription");
    return false;
  }

  return remove_event_subscription(sub_id);
}

//------------------------------------------------------------------------------
bool amf_app::handle_nf_status_notification(
    std::shared_ptr<itti_sbi_notification_data>& msg,
    oai::amf::model::ProblemDetails& problem_details, uint32_t& http_code) {
  Logger::amf_app().info(
      "Handle a NF status notification from NRF (HTTP version "
      "%d)",
      msg->http_version);
  // TODO
  http_code = 204;  // HTTP_STATUS_CODE_204_NO_CONTENT;
  return true;
}

//------------------------------------------------------------------------------
void amf_app::generate_uuid() {
  amf_instance_id = to_string(boost::uuids::random_generator()());
}

//---------------------------------------------------------------------------------------------
void amf_app::add_event_subscription(
    const evsub_id_t& sub_id, const amf_event_type_t& ev,
    std::shared_ptr<amf_subscription> ss) {
  Logger::amf_app().debug(
      "Add an Event subscription (Sub ID %d, Event %d)", sub_id, (uint8_t) ev);
  std::unique_lock lock(m_amf_event_subscriptions);
  amf_event_subscriptions.emplace(std::make_pair(sub_id, ev), ss);
}

//---------------------------------------------------------------------------------------------
bool amf_app::remove_event_subscription(const evsub_id_t& sub_id) {
  Logger::amf_app().debug("Remove an Event subscription (Sub ID %d)", sub_id);
  std::unique_lock lock(m_amf_event_subscriptions);
  for (auto it = amf_event_subscriptions.cbegin();
       it != amf_event_subscriptions.cend();) {
    if ((uint8_t) std::get<0>(it->first) == (uint32_t) sub_id) {
      Logger::amf_app().debug(
          "Found an event subscription (Event ID %d)",
          (uint8_t) std::get<0>(it->first));
      amf_event_subscriptions.erase(it++);
      // it = amf_event_subscriptions.erase(it)
      return true;
    } else {
      ++it;
    }
  }
  return false;
}

//---------------------------------------------------------------------------------------------
void amf_app::get_ee_subscriptions(
    const amf_event_type_t& ev,
    std::vector<std::shared_ptr<amf_subscription>>& subscriptions) {
  for (auto const& i : amf_event_subscriptions) {
    if ((uint8_t) std::get<1>(i.first) == (uint8_t) ev) {
      Logger::amf_app().debug(
          "Found an event subscription (Event ID %d, Event %d)",
          (uint8_t) std::get<0>(i.first), (uint8_t) ev);
      subscriptions.push_back(i.second);
    }
  }
}

//---------------------------------------------------------------------------------------------
void amf_app::get_ee_subscriptions(
    const evsub_id_t& sub_id,
    std::vector<std::shared_ptr<amf_subscription>>& subscriptions) {
  for (auto const& i : amf_event_subscriptions) {
    if (i.first.first == sub_id) {
      subscriptions.push_back(i.second);
    }
  }
}

//---------------------------------------------------------------------------------------------
void amf_app::get_ee_subscriptions(
    const amf_event_type_t& ev, std::string& supi,
    std::vector<std::shared_ptr<amf_subscription>>& subscriptions) {
  for (auto const& i : amf_event_subscriptions) {
    if ((i.first.second == ev) && (i.second->supi == supi)) {
      subscriptions.push_back(i.second);
    }
  }
}

//---------------------------------------------------------------------------------------------
void amf_app::generate_amf_profile() {
  // generate UUID
  generate_uuid();
  nf_instance_profile.set_nf_instance_id(amf_instance_id);
  nf_instance_profile.set_nf_instance_name("OAI-AMF");
  nf_instance_profile.set_nf_type("AMF");
  nf_instance_profile.set_nf_status("REGISTERED");
  nf_instance_profile.set_nf_heartBeat_timer(50);
  nf_instance_profile.set_nf_priority(1);
  nf_instance_profile.set_nf_capacity(100);
  nf_instance_profile.add_nf_ipv4_addresses(amf_cfg.n11.addr4);

  // NF services
  nf_service_t nf_service        = {};
  nf_service.service_instance_id = "namf_communication";
  nf_service.service_name        = "namf_communication";
  nf_service_version_t version   = {};
  version.api_version_in_uri     = "v1";
  version.api_full_version       = "1.0.0";  // TODO: to be updated
  nf_service.versions.push_back(version);
  nf_service.scheme            = "http";
  nf_service.nf_service_status = "REGISTERED";
  // IP Endpoint
  ip_endpoint_t endpoint = {};
  endpoint.ipv4_address  = amf_cfg.n11.addr4;
  endpoint.transport     = "TCP";
  endpoint.port          = amf_cfg.n11.port;
  nf_service.ip_endpoints.push_back(endpoint);

  nf_instance_profile.add_nf_service(nf_service);

  // TODO: custom info
  // AMF info
  amf_info_t info    = {};
  info.amf_region_id = amf_cfg.guami.regionID;
  info.amf_set_id    = amf_cfg.guami.AmfSetID;
  for (auto g : amf_cfg.guami_list) {
    guami_5g_t guami = {};
    guami.amf_id =
        g.regionID + ":" + g.AmfSetID + ":" + g.AmfPointer;  // TODO verify??
    guami.plmn.mcc = g.mcc;
    guami.plmn.mnc = g.mnc;
    info.guami_list.push_back(guami);
  }

  nf_instance_profile.set_amf_info(info);

  // Display the profile
  nf_instance_profile.display();
}

//---------------------------------------------------------------------------------------------
std::string amf_app::get_nf_instance() const {
  return amf_instance_id;
}

//---------------------------------------------------------------------------------------------
void amf_app::register_to_nrf() {
  // send request to N11 to send NF registration to NRF
  trigger_nf_registration_request();
}

//------------------------------------------------------------------------------
void amf_app::trigger_nf_registration_request() {
  Logger::amf_app().debug(
      "Send ITTI msg to N11 task to trigger the registration request to NRF");

  std::shared_ptr<itti_n11_register_nf_instance_request> itti_msg =
      std::make_shared<itti_n11_register_nf_instance_request>(
          TASK_AMF_APP, TASK_AMF_N11);
  itti_msg->profile = nf_instance_profile;

  amf_n11_inst->register_nf_instance(itti_msg);
  /*

  int ret           = itti_inst->send_msg(itti_msg);
  if (RETURNok != ret) {
    Logger::amf_app().error(
        "Could not send ITTI message %s to task TASK_AMF_N11",
        itti_msg->get_msg_name());
  }
  */
}

//------------------------------------------------------------------------------
void amf_app::trigger_nf_deregistration() const {
  Logger::amf_app().debug(
      "Send ITTI msg to N11 task to trigger the deregistration request to NRF");

  std::shared_ptr<itti_n11_deregister_nf_instance> itti_msg =
      std::make_shared<itti_n11_deregister_nf_instance>(
          TASK_AMF_APP, TASK_AMF_N11);
  itti_msg->amf_instance_id = amf_instance_id;
  int ret                   = itti_inst->send_msg(itti_msg);
  if (RETURNok != ret) {
    Logger::amf_app().error(
        "Could not send ITTI message %s to task TASK_AMF_N11",
        itti_msg->get_msg_name());
  }
}

//---------------------------------------------------------------------------------------------
void amf_app::add_promise(
    const uint32_t pid, const boost::shared_ptr<boost::promise<uint32_t>>& p) {
  std::unique_lock lock(m_curl_handle_responses_smf);
  curl_handle_responses_smf.emplace(pid, p);
}

//---------------------------------------------------------------------------------------------
void amf_app::add_promise(
    const uint32_t id,
    const boost::shared_ptr<boost::promise<std::string>>& p) {
  std::unique_lock lock(m_curl_handle_responses_n2_sm);
  curl_handle_responses_n2_sm.emplace(id, p);
}

//---------------------------------------------------------------------------------------------
void amf_app::add_promise(
    const uint32_t pid,
    const boost::shared_ptr<boost::promise<nlohmann::json>>& p) {
  std::unique_lock lock(m_curl_handle_responses_n11);
  curl_handle_responses_n11.emplace(pid, p);
}

//---------------------------------------------------------------------------------------------
void amf_app::trigger_process_response(
    const uint32_t pid, const uint32_t http_code) {
  Logger::amf_app().debug(
      "Trigger process response: Set promise with ID %u "
      "to ready",
      pid);
  std::unique_lock lock(m_curl_handle_responses_smf);
  if (curl_handle_responses_smf.count(pid) > 0) {
    curl_handle_responses_smf[pid]->set_value(http_code);
    // Remove this promise from list
    curl_handle_responses_smf.erase(pid);
  }
}

//------------------------------------------------------------------------------
void amf_app::trigger_process_response(
    const uint32_t pid, const std::string& n2_sm) {
  Logger::amf_app().debug(
      "Trigger process response: Set promise with ID %u "
      "to ready",
      pid);
  std::unique_lock lock(m_curl_handle_responses_n2_sm);
  if (curl_handle_responses_n2_sm.count(pid) > 0) {
    curl_handle_responses_n2_sm[pid]->set_value(n2_sm);
    // Remove this promise from list
    curl_handle_responses_n2_sm.erase(pid);
  }
}

//------------------------------------------------------------------------------
void amf_app::trigger_process_response(
    const uint32_t pid, const nlohmann::json& json_data) {
  Logger::amf_app().debug(
      "Trigger process response: Set promise with ID %u "
      "to ready",
      pid);
  std::unique_lock lock(m_curl_handle_responses_n11);
  if (curl_handle_responses_n11.count(pid) > 0) {
    curl_handle_responses_n11[pid]->set_value(json_data);
    // Remove this promise from list
    curl_handle_responses_n11.erase(pid);
  }
}
