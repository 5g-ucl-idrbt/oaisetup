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

/*! \file nssf_http2-server.h
 \brief
 \author  Rohan Kharade
 \company Openairinterface Software Allianse
 \date 2021
 \email: rohan.kharade@openairinterface.org
 */

#include "nssf_app.hpp"
#include "conversions.hpp"
#include "logger.hpp"
#include "nssf.h"
#include "nssf_config.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <stdexcept>

using namespace nssf;
using namespace std;

extern nssf_app* nssf_app_inst;
extern nssf_config nssf_cfg;

void nssf_app_task(void*);

//------------------------------------------------------------------------------
void nssf_app::handle_slice_info_for_registration(
    const SliceInfoForRegistration& slice_info, const Tai& tai,
    const PlmnId& home_plmnid, const std::string& features, int& http_code,
    const uint8_t http_version, ProblemDetails& problem_details,
    AuthorizedNetworkSliceInfo& auth_slice_info) {
  if (nssf_nss.handle_slice_info_for_registration(
          slice_info, tai, home_plmnid, features, http_code, http_version,
          problem_details, auth_slice_info)) {
    Logger::nssf_app().info(
        "NS Selection: Authorized Network Slice Info Returned !!!");
    Logger::nssf_app().info(
        "//---------------------------------------------------------");
  } else {
    if (http_code == HTTP_STATUS_CODE_403_FORBIDDEN) {
      problem_details.setTitle("UNSUPPORTED_RESOURCE");
      problem_details.setStatus(HTTP_STATUS_CODE_403_FORBIDDEN);
      problem_details.setDetail(
          "S-NSSAI in Requested NSSAI is not supported in PLMN");
      problem_details.setCause("SNSSAI_NOT_SUPPORTED");
      Logger::nssf_app().error("NS Selection failure !!!");
    }
  }
  return;
}

//------------------------------------------------------------------------------
void nssf_app::handle_slice_info_for_pdu_session(
    const SliceInfoForPDUSession& slice_info, const Tai& tai,
    const PlmnId& home_plmnid, const std::string& features, int& http_code,
    const uint8_t http_version, ProblemDetails& problem_details,
    AuthorizedNetworkSliceInfo& auth_slice_info) {
  if (nssf_nss.handle_slice_info_for_pdu_session(
          slice_info, tai, home_plmnid, features, http_code, http_version,
          problem_details, auth_slice_info)) {
    Logger::nssf_app().info(
        "NS Selection: Authorized Network Slice Info Returned !!!");
    Logger::nssf_app().info(
        "//---------------------------------------------------------");
  } else {
    if (http_code == HTTP_STATUS_CODE_403_FORBIDDEN) {
      problem_details.setTitle("UNSUPPORTED_RESOURCE");
      problem_details.setStatus(HTTP_STATUS_CODE_403_FORBIDDEN);
      problem_details.setDetail(
          "S-NSSAI in Requested NSSAI is not supported in PLMN");
      problem_details.setCause("SNSSAI_NOT_SUPPORTED");
      Logger::nssf_app().error("NS Selection failure !!!");
    }
  }
  return;
}

//------------------------------------------------------------------------------
void nssf_app::handle_slice_info_for_ue_cu(
    const SliceInfoForUEConfigurationUpdate& slice_info, const Tai& tai,
    const PlmnId& home_plmnid, const std::string& features, int& http_code,
    const uint8_t http_version, ProblemDetails& problem_details) {
  // ToDo:
}
//------------------------------------------------------------------------------
void nssf_app::handle_create_nssai_availability(
    const std::string& nfId, const NssaiAvailabilityInfo& nssaiAvailInfo,
    AuthorizedNssaiAvailabilityInfo& auth_info, int& http_code,
    const uint8_t http_version, ProblemDetails& problem_details) {
  if (nssf_nsa.handle_create_nssai_availability(
          nfId, nssaiAvailInfo, auth_info, http_code, http_version,
          problem_details)) {
    Logger::nssf_app().info(
        "NSSAI_AVAIL: NssaiAvailabilityInfo Successfully Created/Replaced !!!");
    Logger::nssf_app().info(
        "//---------------------------------------------------------");
  } else {
    if (http_code == HTTP_STATUS_CODE_403_FORBIDDEN) {
      problem_details.setTitle("UNSUPPORTED_RESOURCE");
      problem_details.setStatus(HTTP_STATUS_CODE_403_FORBIDDEN);
      problem_details.setDetail("NSSAI Availability");
      problem_details.setCause("SNSSAI_NOT_SUPPORTED");
      Logger::nssf_app().error("NSSAI Availability failure !!!");
    }
  }
  return;
}

//------------------------------------------------------------------------------
nssf_app::~nssf_app() {
  Logger::nssf_app().debug("Delete NSSF_APP instance...");
}
