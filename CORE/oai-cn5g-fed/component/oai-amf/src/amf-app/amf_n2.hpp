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

/*! \file amf_n2.hpp
 \brief
 \author Keliang DU (BUPT), Tien-Thinh NGUYEN (EURECOM)
 \date 2020
 \email: contact@openairinterface.org
 */

#ifndef _AMF_N2_H_
#define _AMF_N2_H_

#include "DownlinkRANStatusTransfer.hpp"
#include "HandoverCommandMsg.hpp"
#include "HandoverRequest.hpp"
#include "amf.hpp"
#include "itti_msg_n2.hpp"
#include "ngap_app.hpp"
#include "ue_ngap_context.hpp"

namespace amf_application {

class amf_n2 : public ngap::ngap_app {
 public:
  amf_n2(const std::string& address, const uint16_t port_num);
  ~amf_n2();

  /*
   * Handle ITTI message (New SCTP Association)
   * @param [itti_new_sctp_association&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_new_sctp_association& new_assoc);

  /*
   * Handle ITTI message (Downlink NAS Transfer)
   * @param [itti_downlink_nas_transfer&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_ng_setup_request& ngsetupreq);

  /*
   * Handle ITTI message (Downlink NAS Transfer)
   * @param [itti_downlink_nas_transfer&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_ng_reset&);

  /*
   * Handle ITTI message (SCTP Shutdown)
   * @param [itti_ng_shutdown&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_ng_shutdown&);

  /*
   * Handle ITTI message (InitialUEMessage)
   * @param [itti_initial_ue_message&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_initial_ue_message& init_ue_msg);

  /*
   * Handle ITTI message (ULNASTransport)
   * @param [itti_ul_nas_transport&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_ul_nas_transport& ul_nas_transport);

  /*
   * Handle ITTI message (DLNASTransport)
   * @param [itti_dl_nas_transport&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_dl_nas_transport& dl_nas_transport);

  /*
   * Handle ITTI message (InitialContextSetupRequest)
   * @param [itti_initial_context_setup_request&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_initial_context_setup_request& itti_msg);

  /*
   * Handle ITTI message (PDUSessionResourceSetupRequest)
   * @param [itti_pdu_session_resource_setup_request&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_pdu_session_resource_setup_request& itti_msg);

  /*
   * Handle ITTI message (PDUSessionResourceModifyRequest)
   * @param [itti_pdu_session_resource_modify_request&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_pdu_session_resource_modify_request& itti_msg);

  /*
   * Handle ITTI message (PDUSessionResourceReleaseCommand)
   * @param [itti_pdu_session_resource_release_command&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_pdu_session_resource_release_command& itti_msg);

  /*
   * Handle ITTI message (PDUSessionResourceReleaseRequest)
   * @param [itti_ue_context_release_request&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_ue_context_release_request& itti_msg);

  /*
   * Handle ITTI message (PDUSessionResourceReleaseComplete)
   * @param [itti_ue_context_release_complete&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_ue_context_release_complete& itti_msg);

  /*
   * Handle ITTI message (UEContextReleaseCommand)
   * @param [itti_ue_context_release_command&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_ue_context_release_command& itti_msg);

  /*
   * Handle ITTI message (UECapabilityIndication)
   * @param [itti_ue_radio_capability_indication&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_ue_radio_capability_indication& itti_msg);

  /*
   * Handle ITTI message (HandoverRequired)
   * @param [itti_handover_required&]: ITTI message
   * @return void
   */
  bool handle_itti_message(itti_handover_required& itti_msg);

  /*
   * Handle ITTI message (HandoverRequestAck)
   * @param [itti_handover_request_Ack&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_handover_request_Ack& itti_msg);

  /*
   * Handle ITTI message (HandoverNotify)
   * @param [itti_handover_notify&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_handover_notify& itti_msg);

  /*
   * Handle ITTI message (UplinkRANStatusTransfer)
   * @param [itti_uplink_ran_status_transfer&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_uplink_ran_status_transfer& itti_msg);

  /*
   * Handle ITTI message (RerouteNAS)
   * @param [itti_rereoute_nas&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_rereoute_nas& itti_msg);

  /*
   * Handle ITTI message (Paging)
   * @param [itti_paging&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_paging& itti_msg);

  /*
   * Send Handover Preparaton Failure message
   * @param [const unsigned long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const sctp_assoc_id_t&] gnb_assoc_id: gNB Association ID
   * @return void
   */
  void send_handover_preparation_failure(
      const unsigned long amf_ue_ngap_id, const uint32_t ran_ue_ngap_id,
      const sctp_assoc_id_t& gnb_assoc_id);

  /*
   * Get list of common PLMN between AMF and gNB
   * @param [const std::vector<SupportedItem_t>&] list: Supported TA list from
   * gNB
   * @param [std::vector<SupportedItem_t>&] result: list of common TA
   * @return true if there's at least 1 common TA, otherwise return false
   */
  bool get_common_plmn(
      const std::vector<SupportedItem_t>& list,
      std::vector<SupportedItem_t>& result);

  /*
   * Get UE NGAP context associated with a RAN UE NGAP ID
   * @param [const uint32_t&] ran_ue_ngap_id: RAN UE NGAP ID
   * @return shared pointer to the UE NGAP context
   */
  std::shared_ptr<ue_ngap_context> ran_ue_id_2_ue_ngap_context(
      const uint32_t& ran_ue_ngap_id) const;

  /*
   * Verify whether a UE NGAP context associated with a RAN UE NGAP ID exist
   * @param [const uint32_t&] ran_ue_ngap_id: RAN UE NGAP ID
   * @return true if exist, otherwise return false
   */
  bool is_ran_ue_id_2_ue_ngap_context(const uint32_t& ran_ue_ngap_id) const;

  /*
   * Store UE NGAP context associated with a RAN UE NGAP ID
   * @param [const uint32_t&] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const std::shared_ptr<ue_ngap_context>&] unc: pointer to UE NGAP
   * context
   * @return void
   */
  void set_ran_ue_ngap_id_2_ue_ngap_context(
      const uint32_t& ran_ue_ngap_id,
      const std::shared_ptr<ue_ngap_context>& unc);

  /*
   * Remove UE NGAP context associated with a RAN UE NGAP ID
   * @param [const uint32_t&] ran_ue_ngap_id: RAN UE NGAP ID
   * @return void
   */
  void remove_ran_ue_ngap_id_2_ngap_context(const uint32_t& ran_ue_ngap_id);

  /*
   * Remove UE Context associated with a RAN UE NGAP ID
   * @param [const uint32_t&] ran_ue_ngap_id: RAN UE NGAP ID
   * @return void
   */
  void remove_ue_context_with_ran_ue_ngap_id(const uint32_t& ran_ue_ngap_id);

  /*
   * Get UE NGAP context associated with a AMF UE NGAP ID
   * @param [const unsigned long&] amf_ue_ngap_id: AMF UE NGAP ID
   * @return shared pointer to the UE NGAP context
   */
  std::shared_ptr<ue_ngap_context> amf_ue_id_2_ue_ngap_context(
      const unsigned long& amf_ue_ngap_id) const;

  /*
   * Verify whether a UE NGAP context associated with a AMF UE NGAP ID exist
   * @param [const unsigned long&] amf_ue_ngap_id: AMF UE NGAP ID
   * @return true if exist, otherwise return false
   */
  bool is_amf_ue_id_2_ue_ngap_context(
      const unsigned long& amf_ue_ngap_id) const;

  /*
   * Store UE NGAP context associated with a AMF UE NGAP ID
   * @param [const unsigned long&] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [const std::shared_ptr<ue_ngap_context>&] unc: pointer to UE NGAP
   * context
   * @return void
   */
  void set_amf_ue_ngap_id_2_ue_ngap_context(
      const unsigned long& amf_ue_ngap_id,
      std::shared_ptr<ue_ngap_context> unc);

  /*
   * Remove UE NGAP context associated with a AMF UE NGAP ID
   * @param [const unsigned long&] amf_ue_ngap_id: AMF UE NGAP ID
   * @return void
   */
  void remove_amf_ue_ngap_id_2_ue_ngap_context(
      const unsigned long& amf_ue_ngap_id);

  /*
   * Remove UE Context associated with a AMF UE NGAP ID
   * @param [const unsigned long&] amf_ue_ngap_id: AMF UE NGAP ID
   * @return void
   */
  void remove_ue_context_with_amf_ue_ngap_id(
      const unsigned long& amf_ue_ngap_id);

  /*
   * Get list of UE Context associated with a gNB
   * @param [const sctp_assoc_id_t&] gnb_assoc_id: gNB Association ID
   * @param [std::vector<std::shared_ptr<ue_ngap_context>>&] ue_contexts: vector
   * of UE Context
   * @return void
   */
  void get_ue_ngap_contexts(
      const sctp_assoc_id_t& gnb_assoc_id,
      std::vector<std::shared_ptr<ue_ngap_context>>& ue_contexts);

 private:
  std::map<uint32_t, std::shared_ptr<ue_ngap_context>>
      ranid2uecontext;  // ran ue ngap id
  mutable std::shared_mutex m_ranid2uecontext;

  std::map<unsigned long, std::shared_ptr<ue_ngap_context>>
      amfueid2uecontext;  // amf ue id
  mutable std::shared_mutex m_amfueid2uecontext;
};

}  // namespace amf_application

#endif
