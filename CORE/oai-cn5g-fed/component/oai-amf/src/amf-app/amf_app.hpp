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

/*! \file amf_app.hpp
 \brief
 \author  Keliang DU, BUPT
 \date 2020
 \email: contact@openairinterface.org
 */

#ifndef _AMF_APP_H_
#define _AMF_APP_H_

#include <map>
#include <shared_mutex>
#include <string>

#include "amf_config.hpp"
#include "amf_module_from_config.hpp"
#include "amf_profile.hpp"
#include "itti.hpp"
#include "itti_msg_n11.hpp"
#include "itti_msg_amf_app.hpp"
#include "ue_context.hpp"
#include "amf_subscription.hpp"
#include "itti_msg_sbi.hpp"
#include "amf_msg.hpp"
#include "ProblemDetails.h"
#include "UeN1N2InfoSubscriptionCreateData.h"

#include "uint_generator.hpp"
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

using namespace config;

static uint32_t amf_app_ue_ngap_id_generator = 1;

namespace amf_application {

#define TASK_AMF_APP_PERIODIC_STATISTICS (0)
#define TASK_AMF_MOBILE_REACHABLE_TIMER_EXPIRE (1)
#define TASK_AMF_IMPLICIT_DEREGISTRATION_TIMER_EXPIRE (2)

class amf_app {
 private:
  amf_profile nf_instance_profile;  // AMF profile
  std::string amf_instance_id;      // AMF instance id
  timer_id_t timer_nrf_heartbeat;

  util::uint_generator<uint32_t> evsub_id_generator;
  std::map<
      std::pair<evsub_id_t, amf_event_type_t>,
      std::shared_ptr<amf_subscription>>
      amf_event_subscriptions;

  mutable std::shared_mutex m_amf_event_subscriptions;

  util::uint_generator<uint32_t> tmsi_generator;

  std::map<long, std::shared_ptr<ue_context>> amf_ue_ngap_id2ue_ctx;
  mutable std::shared_mutex m_amf_ue_ngap_id2ue_ctx;
  std::map<std::string, std::shared_ptr<ue_context>> ue_ctx_key;
  mutable std::shared_mutex m_ue_ctx_key;

  std::map<std::string, std::shared_ptr<ue_context>> supi2ue_ctx;
  mutable std::shared_mutex m_supi2ue_ctx;

  mutable std::shared_mutex m_curl_handle_responses_smf;
  std::map<uint32_t, boost::shared_ptr<boost::promise<uint32_t>>>
      curl_handle_responses_smf;

  mutable std::shared_mutex m_curl_handle_responses_n2_sm;
  std::map<uint32_t, boost::shared_ptr<boost::promise<std::string>>>
      curl_handle_responses_n2_sm;

  mutable std::shared_mutex m_curl_handle_responses_n11;
  std::map<uint32_t, boost::shared_ptr<boost::promise<nlohmann::json>>>
      curl_handle_responses_n11;

  util::uint_generator<uint32_t> n1n2sub_id_generator;
  std::map<
      std::pair<std::string, uint32_t>,
      std::shared_ptr<oai::amf::model::UeN1N2InfoSubscriptionCreateData>>
      n1n2_message_subscribe;

  mutable std::shared_mutex m_n1n2_message_subscribe;

 public:
  explicit amf_app(const amf_config& amf_cfg);
  amf_app(amf_app const&) = delete;
  void operator=(amf_app const&) = delete;
  void allRegistredModulesInit(const amf_modules& modules);

  /*
   * Generate AMF UE NGAP ID
   * @return generated ID
   */
  long generate_amf_ue_ngap_id();

  /*
   * Handle ITTI message (NAS Signalling Establishment Request: Registration
   * Request, Service Request )
   * @param [itti_nas_signalling_establishment_request&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_nas_signalling_establishment_request& itti_msg);

  /*
   * Handle ITTI message (N1N2MessageTransferRequest)
   * @param [itti_n1n2_message_transfer_request&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_n1n2_message_transfer_request& itti_msg);

  /*
   * Handle ITTI message (SBI N1 Message Notification)
   * @param [itti_sbi_n1_message_notification&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_sbi_n1_message_notification& itti_msg);

  /*
   * Handle ITTI message (SBI N1N2 Message Subscribe)
   * @param [itti_sbi_n1n2_subscribe_message&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_sbi_n1n2_message_subscribe& itti_msg);

  /*
   * Handle ITTI message (SBI N1N2 Message UnSubscribe)
   * @param [itti_sbi_n1n2_unsubscribe_message&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_sbi_n1n2_message_unsubscribe& itti_msg);

  /*
   * Verify if a UE context associated with an AMF UE NGAP ID exist
   * @param [const long&] amf_ue_ngap_id: AMF UE NGAP ID
   * @return true if UE context exist, otherwise false
   */
  bool is_amf_ue_id_2_ue_context(const long& amf_ue_ngap_id) const;

  /*
   * Get UE context associated with an AMF UE NGAP ID
   * @param [const long&] amf_ue_ngap_id: AMF UE NGAP ID
   * @return shared pointer to the context
   */
  std::shared_ptr<ue_context> amf_ue_id_2_ue_context(
      const long& amf_ue_ngap_id) const;

  /*
   * Store an UE context associated with an AMF UE NGAP ID
   * @param [const long&] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [std::shared_ptr<ue_context>&] uc: pointer to UE context
   * @return void
   */
  void set_amf_ue_ngap_id_2_ue_context(
      const long& amf_ue_ngap_id, const std::shared_ptr<ue_context>& uc);

  /*
   * Verify if a UE context associated with an UE Context Key exist
   * @param [const std::string&] ue_context_key: UE Context Key
   * @return true if UE context exist, otherwise false
   */
  bool is_ran_amf_id_2_ue_context(const std::string& ue_context_key) const;

  /*
   * Get UE context associated with an UE Context Key
   * @param [const std::string&] ue_context_key: UE Context Key
   * @return shared pointer to the context
   */
  std::shared_ptr<ue_context> ran_amf_id_2_ue_context(
      const std::string& ue_context_key) const;

  /*
   * Get UE context associated with an UE Context Key and verify if this pointer
   * is nullptr
   * @param [const std::string&] ue_context_key: UE Context Key
   * @param [std::shared_ptr<ue_context>&] uc: pointer to UE context if exist
   * @return true if UE Context exist and not null
   */
  bool ran_amf_id_2_ue_context(
      const std::string& ue_context_key, std::shared_ptr<ue_context>& uc) const;

  /*
   * Store an UE context associated with UE Context Key
   * @param [const std::string&] ue_context_key: UE Context Key
   * @param [std::shared_ptr<ue_context>&] uc: pointer to UE context
   * @return void
   */
  void set_ran_amf_id_2_ue_context(
      const std::string& ue_context_key, const std::shared_ptr<ue_context>& uc);

  /*
   * Verify whether a UE context associated with a SUPI exist
   * @param [const std::string&] supi: UE SUPI
   * @return true if UE context exist, otherwise false
   */
  bool is_supi_2_ue_context(const string& supi) const;

  /*
   * Get UE context associated with a SUPI
   * @param [const std::string&] supi: UE SUPI
   * @return shared pointer to the context
   */
  std::shared_ptr<ue_context> supi_2_ue_context(const string& supi) const;

  /*
   * Store an UE context associated with a SUPI
   * @param [const std::string&] supi: UE SUPI
   * @param [const std::shared_ptr<ue_context>&] uc: pointer to UE context
   * @return void
   */
  void set_supi_2_ue_context(
      const string& supi, const std::shared_ptr<ue_context>& uc);

  /*
   * Find a PDU Session Context associated with a SUPI and a PDU Session ID
   * @param [const std::string&] supi: UE SUPI
   * @param [ const std::uint8_t] pdu_session_id: PDU Session ID
   * @param [const std::shared_ptr<pdu_session_context>&] psc: pointer to the
   * PDU Session Context
   * @return true if found, otherwise false
   */
  bool find_pdu_session_context(
      const string& supi, const std::uint8_t pdu_session_id,
      std::shared_ptr<pdu_session_context>& psc);

  /*
   * Get a list of PDU Session Contexts associated with a SUPI
   * @param [const std::string&] supi: UE SUPI
   * @param [std::vector<std::shared_ptr<pdu_session_context>>&] sessions_ctx:
   * vector of contexts
   * @return true if found, otherwise false
   */
  bool get_pdu_sessions_context(
      const string& supi,
      std::vector<std::shared_ptr<pdu_session_context>>& sessions_ctx);

  /*
   * Generate a TMSI value for UE
   * @param void
   * @return generated value in uint32_t
   */
  uint32_t generate_tmsi();

  /*
   * Create a 5G GUTI from PLMN/TMSI
   * @param [const uint32_t] ranid: RAN UE NGAP ID
   * @param [const long] amfid: AMF UE NGAP ID
   * @param [std::string&] MCC: Mobile Country Code
   * @param [std::string&] MNC: Mobile Network Code
   * @param [uint32_t&] tmsi: Generated TMSI
   * @return true if generated successfully, otherwise return false
   */
  bool generate_5g_guti(
      const uint32_t ranid, const long amfid, std::string& mcc,
      std::string& mnc, uint32_t& tmsi);

  /*
   * Generate an Event Exposure Subscription ID
   * @param [void]
   * @return the generated reference
   */
  evsub_id_t generate_ev_subscription_id();

  /*
   * Generate an N1N2MessageSubscribe ID
   * @param [void]
   * @return the generated reference
   */
  n1n2sub_id_t generate_n1n2_message_subscription_id();

  /*
   * Add an N1N2MessageSubscribe subscription to the list
   * @param [const std::string&] ue_ctx_id: UE Context ID
   * @param [const n1n2sub_id_t&] sub_id: Subscription ID
   * @param [std::shared_ptr<oai::amf::model::UeN1N2InfoSubscriptionCreateData>]
   * oai::amf::model::UeN1N2InfoSubscriptionCreateData: a shared pointer stored
   * information of the subscription
   * @return void
   */
  void add_n1n2_message_subscription(
      const std::string& ue_ctx_id, const n1n2sub_id_t& sub_id,
      std::shared_ptr<oai::amf::model::UeN1N2InfoSubscriptionCreateData>&
          subscription);

  /*
   * Remove an N1N2MessageSubscribe subscription from the list
   * @param [const std::string&] ue_ctx_id: UE Context ID
   * @param [const std::string&] sub_id: Subscription ID
   * @return true if the subscription is deleted successfully, otherwise return
   * false
   */
  bool remove_n1n2_message_subscription(
      const std::string& ue_ctx_id, const std::string& sub_id);

  /*
   * Trigger NF instance registration to NRF
   * @param [void]
   * @return void
   */
  void register_to_nrf();

  /*
   * Handle Event Exposure Msg from NF
   * @param [std::shared_ptr<itti_sbi_event_exposure_request>&] Request message
   * @return [evsub_id_t] ID of the created subscription
   */
  evsub_id_t handle_event_exposure_subscription(
      std::shared_ptr<itti_sbi_event_exposure_request> msg);

  /*
   * Handle Unsubscribe Request from an NF
   * @param [const std::string&] subscription_id: Subscription ID
   * @return true if the subscription is unsubscribed successfully, otherwise
   * return false
   */
  bool handle_event_exposure_delete(const std::string& subscription_id);

  /*
   * Handle NF status notification (e.g., when an UPF becomes available)
   * @param [std::shared_ptr<itti_sbi_notification_data>& ] msg: message
   * @param [oai::amf::model::ProblemDetails& ] problem_details
   * @param [uint32_t&] http_code
   * @return true if handle successfully, otherwise return false
   */
  bool handle_nf_status_notification(
      std::shared_ptr<itti_sbi_notification_data>& msg,
      oai::amf::model::ProblemDetails& problem_details, uint32_t& http_code);

  /*
   * Generate a random UUID for AMF instance
   * @param [void]
   * @return void
   */
  void generate_uuid();

  /*
   * Add an Event Subscription to the list
   * @param [const evsub_id_t&] sub_id: Subscription ID
   * @param [const amf_event_t&] ev: Event type
   * @param [std::shared_ptr<amf_subscription>] ss: a shared pointer stored
   * information of the subscription
   * @return void
   */
  void add_event_subscription(
      const evsub_id_t& sub_id, const amf_event_type_t& ev,
      std::shared_ptr<amf_subscription> ss);

  /*
   * Remove an Event Subscription from the list
   * @param [const evsub_id_t&] sub_id: Subscription ID
   * @return bool
   */
  bool remove_event_subscription(const evsub_id_t& sub_id);

  /*
   * Get a list of subscription associated with a particular event
   * @param [const amf_event_t] ev: Event type
   * @param [std::vector<std::shared_ptr<amf_subscription>>&] subscriptions:
   * store the list of the subscription associated with this event type
   * @return void
   */
  void get_ee_subscriptions(
      const amf_event_type_t& ev,
      std::vector<std::shared_ptr<amf_subscription>>& subscriptions);

  /*
   * Get a list of subscription associated with a particular event
   * @param [const evsub_id_t&] sub_id: Subscription ID
   * @param [std::vector<std::shared_ptr<amf_subscription>>&] subscriptions:
   * store the list of the subscription associated with this event type
   * @return void
   */
  void get_ee_subscriptions(
      const evsub_id_t& sub_id,
      std::vector<std::shared_ptr<amf_subscription>>& subscriptions);

  /*
   * Get a list of subscription associated with a particular event
   * @param [const amf_event_t] ev: Event type
   * @param [std::string&] supi: SUPI
   * @param [std::vector<std::shared_ptr<amf_subscription>>&] subscriptions:
   * store the list of the subscription associated with this event type
   * @return void
   */
  void get_ee_subscriptions(
      const amf_event_type_t& ev, std::string& supi,
      std::vector<std::shared_ptr<amf_subscription>>& subscriptions);

  /*
   * Generate a AMF profile for this instance
   * @param [void]
   * @return void
   */
  void generate_amf_profile();

  /*
   * Send request to N11 task to trigger NF instance registration to NRF
   * @param [void]
   * @return void
   */
  void trigger_nf_registration_request();

  /*
   * Send request to N11 task to trigger NF instance deregistration to NRF
   * @param [void]
   * @return void
   */
  void trigger_nf_deregistration() const;

  /*
   * Store the promise
   * @param [const uint32_t] pid: promise id
   * @param [const boost::shared_ptr<boost::promise<uint32_t>>&] p: promise
   * @return void
   */
  void add_promise(
      const uint32_t pid, const boost::shared_ptr<boost::promise<uint32_t>>& p);

  /*
   * Store the promise
   * @param [const uint32_t] pid: promise id
   * @param [const boost::shared_ptr<boost::promise<std::string>>&] p: promise
   * @return void
   */
  void add_promise(
      const uint32_t pid,
      const boost::shared_ptr<boost::promise<std::string>>& p);

  /*
   * Store the promise
   * @param [const uint32_t] pid: promise id
   * @param [const boost::shared_ptr<boost::promise<nlohmann::json>>&] p:
   * promise
   * @return void
   */
  void add_promise(
      const uint32_t pid,
      const boost::shared_ptr<boost::promise<nlohmann::json>>& p);

  /*
   * Remove the promise
   * @param [uint32_t] pid: promise id
   * @return void
   */
  void remove_promise(const uint32_t id);

  /*
   * Generate an unique value for promise id
   * @param void
   * @return generated promise id
   */
  static uint64_t generate_promise_id() {
    return util::uint_uid_generator<uint64_t>::get_instance().get_uid();
  }

  /*
   * Trigger the response from API server
   * @param [const uint32_t] pid: promise id
   * @param [const uint32_t] http_code: result for the corresponding promise
   * @return void
   */
  void trigger_process_response(const uint32_t pid, const uint32_t http_code);

  /*
   * Trigger the response from API server
   * @param [const uint32_t] pid: promise id
   * @param [const std::string] n2_sm: result for the corresponding promise
   * @return void
   */
  void trigger_process_response(const uint32_t pid, const std::string& n2_sm);

  /*
   * Trigger the response from API server
   * @param [const uint32_t] pid: promise id
   * @param [const nlohmann::json&] json_data: result for the corresponding
   * promise
   * @return void
   */
  void trigger_process_response(
      const uint32_t pid, const nlohmann::json& json_data);

  /*
   * Get the AMF's NF instance
   * @return NF instance in string format
   */
  std::string get_nf_instance() const;
};

}  // namespace amf_application

#endif
