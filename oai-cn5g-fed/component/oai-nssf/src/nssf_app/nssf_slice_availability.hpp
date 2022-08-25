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

/*! \file nssf_slice_avail.hpp
 \brief
 \author  Rohan Kharade
 \company Openairinterface Software Allianse
 \date Jan 2022
 \email: rohan.kharade@openairinterface.org
 */

#ifndef FILE_NSSF_SLICE_AVAILABILITY_HPP_SEEN
#define FILE_NSSF_SLICE_AVAILABILITY_HPP_SEEN

#include <boost/atomic.hpp>

#include "3gpp_29.500.h"
#include "common_root_types.h"
#include "conversions.hpp"
#include "logger.hpp"
#include "nssf.h"
#include "nssf_config.hpp"
#include <string>

#include "AuthorizedNssaiAvailabilityInfo.h"
#include "NssaiAvailabilityInfo.h"
#include "NssfEventSubscriptionCreateData.h"
#include "PatchItem.h"
#include "PlmnId.h"
#include "ProblemDetails.h"
#include "SupportedNssaiAvailabilityData.h"
#include "Tai.h"

namespace nssf {

using namespace oai::nssf_server::model;

class nssf_slice_avail {
 private:
  static bool amf_set_present(
      const std::string& target_amf_set, amf_info_t& amf_info);

 public:
  explicit nssf_slice_avail(const std::string& config_file);
  nssf_slice_avail(nssf_slice_avail const&) = delete;
  void operator=(nssf_slice_avail const&) = delete;

  virtual ~nssf_slice_avail();

  // Handle NSSF NSSAI Availability - NF Instance ID (Document)

  bool handle_create_nssai_availability(
      const std::string& nfId, const NssaiAvailabilityInfo& nssaiAvailInfo,
      AuthorizedNssaiAvailabilityInfo& auth_info, int& http_code,
      const uint8_t http_version, const ProblemDetails& problem_details);

  bool handle_update_nssai_availability(
      const std::string& nfId, const std::vector<PatchItem>& patchItem,
      int& http_code, const uint8_t http_version,
      const ProblemDetails& problem_details);

  bool handle_remove_nssai_availability(
      const std::string& nfId, int& http_code, const uint8_t http_version,
      const ProblemDetails& problem_details);

  // Handle NSSF NSSAI Availability - Subscription ID (Collection/Document)
  bool handle_create_subscription_nssai_availability(
      const NssfEventSubscriptionCreateData& subscriptionData, int& http_code,
      const uint8_t http_version, const ProblemDetails& problem_details);

  bool handle_update_subscription_nssai_availability(
      const NssfEventSubscriptionCreateData& subscriptionData, int& http_code,
      const uint8_t http_version, const ProblemDetails& problem_details);

  bool handle_remove_subscription_nssai_availability(
      const std::string& subscriptionId, int& http_code,
      const uint8_t http_version, const ProblemDetails& problem_details);
};
}  // namespace nssf
#endif /* FILE_NSSF_SLICE_AVAILABILITY_HPP_SEEN */
