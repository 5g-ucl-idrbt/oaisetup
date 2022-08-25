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

/*! \file nssf_slice_select.hpp
 \brief
 \author  Rohan Kharade
 \company Openairinterface Software Allianse
 \date Jan 2022
 \email: rohan.kharade@openairinterface.org
 */

#ifndef FILE_NSSF_SLICE_SELECT_HPP_SEEN
#define FILE_NSSF_SLICE_SELECT_HPP_SEEN

#include <boost/atomic.hpp>
#include <string>

#include "3gpp_29.500.h"
#include "AuthorizedNetworkSliceInfo.h"
#include "ExtSnssai.h"
#include "PatchItem.h"
#include "PlmnId.h"
#include "ProblemDetails.h"
#include "SliceInfoForPDUSession.h"
#include "SliceInfoForRegistration.h"
#include "SliceInfoForUEConfigurationUpdate.h"
#include "Tai.h"
#include "common_root_types.h"

namespace nssf {

using namespace oai::nssf_server::model;

class nssf_slice_select {
 private:
  static bool validate_rnssai_in_plmn(
      const SliceInfoForRegistration& slice_info,
      AuthorizedNetworkSliceInfo& auth_slice_info);
  static bool validate_rnssai_in_ta(
      const SliceInfoForRegistration& slice_info,
      AuthorizedNetworkSliceInfo& auth_slice_info);

  static bool get_valid_amfset(
      const std::vector<Snssai>& req_nssai,
      AuthorizedNetworkSliceInfo& auth_slice_info);

  static bool get_valid_amf(
      const std::vector<ExtSnssai> e_snssai_list,
      const std::vector<Snssai> r_nssai);
  static void set_allowed_nssai(
      const std::vector<Snssai> nssai,
      AuthorizedNetworkSliceInfo& auth_slice_info);
  static bool compare_snssai(const Snssai a, const Snssai b);
  static bool validate_ta(const Tai tai, std::vector<Snssai> rejected_snssai);
  static bool validate_ta(const Tai& tai);
  static bool validate_nsi(
      const SliceInfoForPDUSession& slice_info, NsiInformation& nsi_info);

 public:
  explicit nssf_slice_select(const std::string& config_file);
  nssf_slice_select(nssf_slice_select const&) = delete;
  void operator=(nssf_slice_select const&) = delete;

  virtual ~nssf_slice_select();
  //  Handle Network Slice Information (Document)

  /*
   * @param [const SliceInfoForRegistration&] slice_info:
   * SliceInfoForRegistration
   * @param [const Tai&] tai: Tracking Area Identity
   * @param [const PlmnId&] home_plmnid: Home plmnid
   * @param [const std::string&] features: Supported features
   * @param [int &] http_code: HTTP code used to return to the consumer
   * @param [const uint8_t] http_version: HTTP version
   * @param [ProblemDetails &] problem_details: Store details of the error
   * @return void
   */
  bool handle_slice_info_for_registration(
      const SliceInfoForRegistration& slice_info, const Tai& tai,
      const PlmnId& home_plmnid, const std::string& features, int& http_code,
      const uint8_t http_version, ProblemDetails& problem_details,
      AuthorizedNetworkSliceInfo& auth_slice_info);

  /*
   * @param [const SliceInfoForPDUSessionn&] slice_info: SliceInfoForPDUSession
   * @param [const Tai&] tai: Tracking Area Identity
   * @param [const PlmnId&] home_plmnid: Home plmnid
   * @param [const std::string&] features: Supported features
   * @param [int &] http_code: HTTP code used to return to the consumer
   * @param [const uint8_t] http_version: HTTP version
   * @param [ProblemDetails &] problem_details: Store details of the error
   * @return void
   */
  bool handle_slice_info_for_pdu_session(
      const SliceInfoForPDUSession& slice_info, const Tai& tai,
      const PlmnId& home_plmnid, const std::string& features, int& http_code,
      const uint8_t http_version, const ProblemDetails& problem_details,
      AuthorizedNetworkSliceInfo& auth_slice_info);

  /*
   * @param [const SliceInfoForUEConfigurationUpdate&] slice_info:
   * SliceInfoForUEConfigurationUpdate
   * @param [const Tai&] tai: Tracking Area Identity
   * @param [const PlmnId&] home_plmnid: Home plmnid
   * @param [const std::string&] features: Supported features
   * @param [int &] http_code: HTTP code used to return to the consumer
   * @param [const uint8_t] http_version: HTTP version
   * @param [ProblemDetails &] problem_details: Store details of the error
   * @return void
   */
  bool handle_slice_info_for_ue_cu(
      const SliceInfoForUEConfigurationUpdate& slice_info, const Tai& tai,
      const PlmnId& home_plmnid, const std::string& features, int& http_code,
      const uint8_t http_version, const ProblemDetails& problem_details);
};
}  // namespace nssf
#endif /* FILE_NSSF_SLICE_SELECT_HPP_SEEN */
