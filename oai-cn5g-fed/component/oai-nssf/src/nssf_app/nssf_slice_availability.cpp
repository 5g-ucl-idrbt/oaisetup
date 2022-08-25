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

/*! \file nssf_slice_availability.cpp
 \brief
 \author  Rohan Kharade
 \company Openairinterface Software Allianse
 \date Jan 2022
 \email: rohan.kharade@openairinterface.org
 */

#include "nssf_slice_availability.hpp"
#include "conversions.hpp"
#include "logger.hpp"
#include "nssf.h"
#include "nssf_config.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <stdexcept>

using namespace nssf;
using namespace std;

extern nssf_slice_avail* nssf_slice_avail_inst;
extern nssf_config nssf_cfg;
//------------------------------------------------------------------------------
bool nssf_slice_avail::amf_set_present(
    const std::string& target_amf_set, amf_info_t& amf_info) {
  for (auto amf_info_it : nssf_cfg.nssf_amf_info.amf_info_list) {
    if (!amf_info_it.target_amf_set.compare(target_amf_set)) {
      amf_info = amf_info_it;
      return true;
    }
  }
  return false;
}
//------------------------------------------------------------------------------
bool nssf_slice_avail::handle_create_nssai_availability(
    const std::string& nfId, const NssaiAvailabilityInfo& nssaiAvailInfo,
    AuthorizedNssaiAvailabilityInfo& auth_info, int& http_code,
    const uint8_t http_version, const ProblemDetails& problem_details) {
  if (nssaiAvailInfo.amfSetIdIsSet()) {
    std::string amf_set = nssaiAvailInfo.getAmfSetId();
    Logger::nssf_app().info(
        "NS Availability: Create NSSAIs availability info for AMF "
        "(HTTP_VERSION %d)",
        http_version);
    // ToDo:- Check if SupportedSnssaiList & Tai in valid in plmn

    // Check if target_amf_set is present in nssf slice config
    if (!amf_set.empty()) {
      amf_info_t amf_info;
      if (amf_set_present(amf_set, amf_info)) {
        Logger::nssf_app().debug(
            "target_amf_set matched -> %s", amf_info.target_amf_set.c_str());
        bool update_done = false;
        for (auto amf : amf_info.amf_List) {
          if (!amf.first.compare(nfId)) {
            Logger::nssf_app().info(
                "Replacing nssaiAvailInfo for existing AMF");
            amf.second.clear();
            amf.second  = nssaiAvailInfo.getSupportedNssaiAvailabilityData();
            update_done = true;
            http_code   = HTTP_STATUS_CODE_204_NO_CONTENT;
            break;
          }
        }
        if (!update_done) {
          Logger::nssf_app().info("Creating nssaiAvailInfo for new AMF");
          amf_info.amf_List.emplace_back(
              nfId.c_str(), nssaiAvailInfo.getSupportedNssaiAvailabilityData());
          http_code = HTTP_STATUS_CODE_204_NO_CONTENT;
        }
      } else {
        Logger::nssf_app().warn("target_amf_set not matched");
        http_code = HTTP_STATUS_CODE_503_SERVICE_UNAVAILABLE;
        return false;
      }
    }
  } else {
    Logger::nssf_app().warn("Optional field target_amf_set not provided");
    // ToDo:- Select first target-amf_set based on tai
    // amf_info_t new_amf_info;
    http_code = HTTP_STATUS_CODE_503_SERVICE_UNAVAILABLE;
    return false;
  }

  return true;
}
//------------------------------------------------------------------------------
bool nssf_slice_avail::handle_update_nssai_availability(
    const std::string& nfId, const std::vector<PatchItem>& patchItem,
    int& http_code, const uint8_t http_version,
    const ProblemDetails& problem_details) {
  // ToDo
  return false;
}
//------------------------------------------------------------------------------
bool nssf_slice_avail::handle_remove_nssai_availability(
    const std::string& nfId, int& http_code, const uint8_t http_version,
    const ProblemDetails& problem_details) {
  // ToDo
  return false;
}
//------------------------------------------------------------------------------
// Handle NSSF NSSAI Availability - Subscription ID (Collection/Document)
bool nssf_slice_avail::handle_create_subscription_nssai_availability(
    const NssfEventSubscriptionCreateData& subscriptionData, int& http_code,
    const uint8_t http_version, const ProblemDetails& problem_details) {
  // ToDo
  return false;
}
//------------------------------------------------------------------------------
bool nssf_slice_avail::handle_update_subscription_nssai_availability(
    const NssfEventSubscriptionCreateData& subscriptionData, int& http_code,
    const uint8_t http_version, const ProblemDetails& problem_details) {
  // ToDo
  return false;
}
//------------------------------------------------------------------------------
bool nssf_slice_avail::handle_remove_subscription_nssai_availability(
    const std::string& subscriptionId, int& http_code,
    const uint8_t http_version, const ProblemDetails& problem_details) {
  // ToDo
  return false;
}
//------------------------------------------------------------------------------
nssf_slice_avail::~nssf_slice_avail() {
  Logger::nssf_app().debug("Delete NSSF_SLICE_AVAIL instance...");
}
