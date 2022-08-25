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

/*! \file nssf_slice_select.cpp
 \brief
 \author  Rohan Kharade
 \company Openairinterface Software Allianse
 \date Jan 2022
 \email: rohan.kharade@openairinterface.org
 */

#include "nssf_slice_selection.hpp"
#include "AccessType.h"
#include "NetworkSliceInformationDocumentApiImpl.h"
#include "conversions.hpp"
#include "logger.hpp"
#include "nssf.h"
#include "nssf_config.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <stdexcept>

using namespace nssf;
using namespace std;

extern nssf_slice_select* nssf_slice_select_inst;
extern nssf_config nssf_cfg;

//------------------------------------------------------------------------------
bool nssf_slice_select::compare_snssai(const Snssai a, const Snssai b) {
  if (a.getSst() == b.getSst()) {
    if (a.sdIsSet() || b.sdIsSet()) {
      if (a.getSd().compare(b.getSd())) return false;
    }
    return true;
  } else
    return false;
}

//------------------------------------------------------------------------------
void nssf_slice_select::set_allowed_nssai(
    const std::vector<Snssai> nssai,
    AuthorizedNetworkSliceInfo& auth_slice_info) {
  // Set Subscribed-Nssai into Allowed NSSAI list
  std::vector<AllowedNssai> allowed_nssai_list;
  std::vector<AllowedSnssai> allowed_snssai_list;
  AllowedNssai allowed_nssai;
  for (auto snssai : nssai) {
    AllowedSnssai allowed_snssai;
    Snssai Snssai;
    Snssai.setSst(snssai.getSst());
    if (snssai.sdIsSet()) Snssai.setSd(snssai.getSd());
    allowed_snssai.setAllowedSnssai(Snssai);
    allowed_snssai_list.push_back(allowed_snssai);
  }
  AccessType access_type;
  access_type.setEnumValue(oai::nssf_server::model::AccessType_anyOf::
                               eAccessType_anyOf::ACCESS_3GPP);
  allowed_nssai.setAccessType(access_type);
  allowed_nssai.setAllowedSnssaiList(allowed_snssai_list);
  allowed_nssai_list.push_back(allowed_nssai);
  auth_slice_info.setAllowedNssaiList(allowed_nssai_list);
}
//------------------------------------------------------------------------------
bool nssf_slice_select::get_valid_amf(
    const std::vector<ExtSnssai> e_snssai_list,
    const std::vector<Snssai> r_nssai) {
  std::vector<Snssai> rejected_snssai;
  for (auto r_snssai : r_nssai) {
    bool amf_matched = false;
    for (auto e_snssai : e_snssai_list) {
      Snssai e_nssai_tmp;
      e_nssai_tmp.setSst(e_snssai.getSst());
      if (e_snssai.sdIsSet()) e_nssai_tmp.setSd(e_snssai.getSd());
      if (compare_snssai(r_snssai, e_nssai_tmp)) amf_matched = true;
    }
    if (!amf_matched) rejected_snssai.push_back(r_snssai);
  }

  if (rejected_snssai.size() > 0)
    return false;
  else
    return true;
}
//------------------------------------------------------------------------------
bool nssf_slice_select::get_valid_amfset(
    const std::vector<Snssai>& req_nssai,
    AuthorizedNetworkSliceInfo& auth_slice_info) {
  Logger::nssf_app().debug("Validating AMFSet");

  std::vector<std::string> candidate_amf_list;
  for (auto amfSet : nssf_cfg.nssf_amf_info.amf_info_list) {
    for (auto amfList : amfSet.amf_List) {
      for (auto ext_snssai : amfList.second) {
        if (get_valid_amf(ext_snssai.getSupportedSnssaiList(), req_nssai)) {
          Logger::nssf_app().debug("AMF matched :- %s", amfList.first.c_str());
          candidate_amf_list.push_back(amfList.first);
          break;
        }
      }
    }
    if (!candidate_amf_list.empty()) {
      auth_slice_info.setCandidateAmfList(candidate_amf_list);
      auth_slice_info.setTargetAmfSet(amfSet.target_amf_set);
      auth_slice_info.setNrfAmfSet(amfSet.nrf_amf_set);
      auth_slice_info.setNrfAmfSetNfMgtUri(amfSet.nrf_amf_set_mgt);
      return true;
    }
  }
  return false;
}
//------------------------------------------------------------------------------
bool nssf_slice_select::validate_rnssai_in_ta(
    const SliceInfoForRegistration& slice_info,
    AuthorizedNetworkSliceInfo& auth_slice_info) {
  // ToDo
  return true;
}
//------------------------------------------------------------------------------
bool nssf_slice_select::validate_rnssai_in_plmn(
    const SliceInfoForRegistration& slice_info,
    AuthorizedNetworkSliceInfo& auth_slice_info) {
  std::vector<Snssai> rejected_snssai;
  std::vector<Snssai> matched_snssai;

  for (auto r_snssai : slice_info.getRequestedNssai()) {
    bool snssai_matched = false;
    for (auto s_snssai : slice_info.getSubscribedNssai()) {
      if (compare_snssai(r_snssai, s_snssai.getSubscribedSnssai()))
        snssai_matched = true;
    }
    if (!snssai_matched)
      rejected_snssai.push_back(r_snssai);
    else
      matched_snssai.push_back(r_snssai);
  }

  if (rejected_snssai.size() == slice_info.getRequestedNssai().size()) {
    auth_slice_info.setRejectedNssaiInPlmn(slice_info.getRequestedNssai());
    Logger::nssf_app().debug("Requested Nssai is not valid in the PLMN");
    return false;
  }

  // Set Subscribed-Nssai into Allowed NSSAI list
  set_allowed_nssai(matched_snssai, auth_slice_info);
  Logger::nssf_app().debug("Requested Nssai is valid in the PLMN");
  return true;
}
//------------------------------------------------------------------------------
bool nssf_slice_select::validate_nsi(
    const SliceInfoForPDUSession& slice_info, NsiInformation& nsi_info) {
  Logger::nssf_app().debug("Validating S-NSSAI for NSI");

  Snssai requested_snssai = slice_info.getSNssai();

  for (auto nsi_info_item : nssf_cfg.nssf_nsi_info.nsi_info_list) {
    if (compare_snssai(requested_snssai, nsi_info_item.snssai)) {
      nsi_info.setNrfId(nsi_info_item.nsi_info.getNrfId());
      if (nsi_info_item.nsi_info.nsiIdIsSet())
        nsi_info.setNsiId(nsi_info_item.nsi_info.getNsiId());
      if (nsi_info_item.nsi_info.nrfNfMgtUriIsSet())
        nsi_info.setNrfNfMgtUri(nsi_info_item.nsi_info.getNrfNfMgtUri());
      return true;
    }
  }

  Logger::nssf_app().warn(
      "NS Selection: S-NSSAI from SliceInfoForPDUSession "
      "is not authorised !!!");
  Logger::nssf_app().info(
      "//---------------------------------------------------------");
  Logger::nssf_app().info("");
  return false;
}
//------------------------------------------------------------------------------
bool nssf_slice_select::validate_ta(const Tai& tai) {
  // Logger::nssf_app().debug("Validating TA");
  PlmnId requested_plmn     = tai.getPlmnId();
  std::string requested_tac = tai.getTac();

  for (auto ta_info_item : nssf_cfg.nssf_ta_info.ta_info_list) {
    if (requested_plmn.getMcc() == ta_info_item.tai.getPlmnId().getMcc() &
        requested_plmn.getMnc() == ta_info_item.tai.getPlmnId().getMnc() &
        requested_tac == ta_info_item.tai.getTac())
      return true;
  }
  Logger::nssf_app().warn("NS Selection: TAI is not authorised !!!");
  Logger::nssf_app().info(
      "//---------------------------------------------------------");
  Logger::nssf_app().info("");
  return false;
}

//------------------------------------------------------------------------------
bool nssf_slice_select::handle_slice_info_for_registration(
    const SliceInfoForRegistration& slice_info, const Tai& tai,
    const PlmnId& home_plmnid, const std::string& features, int& http_code,
    const uint8_t http_version, ProblemDetails& problem_details,
    AuthorizedNetworkSliceInfo& auth_slice_info) {
  Logger::nssf_app().info(
      "NS Selection: Handle case - Registration (HTTP_VERSION %d)",
      http_version);

  //### Step 1. Validation for roaming/EPS to 5GS Mobility procedure from
  // slice_info
  if (slice_info.requestMappingIsSet()) {
    Logger::nssf_app().debug("NS Selection: Request Mapping is provided");
    // ToDo: Ts 29.531, R16.0.0, Table 6.1.6.2.10-1 SliceInfoForRegistration
    // This IE may be present when the Nnssf_NSSelection_Get procedure is
    // invoked during EPS to 5GS Mobility Registration Procedure (Idle and
    // Connected State) using N26 interface. When present this IE shall indicate
    // to the NSSF that the NSSF shall return the VPLMN specific mapped SNSSAI
    // values for the S-NSSAI values in the subscribedNssai IE.
    if (slice_info.sNssaiForMappingIsSet())
      ;  // Ignore for now
    http_code = HTTP_STATUS_CODE_503_SERVICE_UNAVAILABLE;
    Logger::nssf_app().warn(
        "NS Selection: EPS to 5GS Mobility Registration Procedure is not "
        "Supported yet. "
        "RequestMapping can not be validated !!");
    Logger::nssf_app().info(
        "//---------------------------------------------------------");
    Logger::nssf_app().info("");
    return false;
  }

  if (slice_info.mappingOfNssaiIsSet()) {
    Logger::nssf_app().debug("NS Selection: Mapping of NSSAI is provided");
    // ToDo: Ts 29.531, R16.0.0, Table 6.1.6.2.10-1 SliceInfoForRegistration
    // This IE shall contain the  mapping of S-NSSAI of the VPLMN to
    // corresponding HPLMN S-NSSAI, for the S-NSSAIs included in the
    // requestedNssai and allowedNssai IEs for the current and other
    // access types. It is also present in  EPS to 5GS handover procedure.
    http_code = HTTP_STATUS_CODE_503_SERVICE_UNAVAILABLE;
    Logger::nssf_app().warn(
        "NS Selection: Roaming/EPS to 5GS handover procedure is not "
        "Supported yet. "
        "RequestMapping can not be validated !!");
    Logger::nssf_app().info(
        "//---------------------------------------------------------");
    Logger::nssf_app().info("");
    return false;
  }

  //### Step 2. Validation for roaming from homeplmnid if provided
  // Check if UE's HPlmnId is Supported while UE is Roaminng
  if (!home_plmnid.getMcc().empty()) {
    Logger::nssf_app().debug("NS Selection: HomePlmnId is provided !!!");
    // ToDo - Validate PlmnId from nssf config (Currently we don't support
    // Roaming scenario)
    http_code = HTTP_STATUS_CODE_503_SERVICE_UNAVAILABLE;
    Logger::nssf_app().warn(
        "NS Selection: Roming is not Supported yet. "
        "HomePlmnId can not be validated !!");
    Logger::nssf_app().info(
        "//---------------------------------------------------------");
    Logger::nssf_app().info("");
    return false;
  }

  //### Step 3. Validation for TAI if provided
  // Check if UE's Tai is Supported
  if (!tai.getTac().empty()) {
    Logger::nssf_app().debug("NS Selection: TAI is provided");
    if (!validate_ta(tai)) {
      return false;
    }
  }

  bool configure_nssai = false;
  //### Step 4. Validation of slice_info for target AMFSet
  // Check for Requested S-NSSAI
  if (slice_info.requestedNssaiIsSet()) {
    Logger::nssf_app().debug("NS Selection: Requested S-NSSAI is provided");
    if (slice_info.subscribedNssaiIsSet()) {
      Logger::nssf_app().debug("NS Selection: Subscribed S-NSSAI is provided");
      http_code = HTTP_STATUS_CODE_403_FORBIDDEN;

      // Step 4.1. Validate if Requested S-NSSAI is supported in PLMN
      if (!validate_rnssai_in_plmn(slice_info, auth_slice_info))
        // ToDo: 23.501, R16.0.0., 5.15.5.2.1 - If Requested NSSAI is not valid
        // in the Serving PLMN, then NSSF based on the Subscribed SNSSAI(s) and
        // operator configuration may also determine the Configured NSSAI
        return false;
      // Step 4.2. Validate if Requested S-NSSAI is supported in TA
      if (!validate_rnssai_in_ta(slice_info, auth_slice_info)) return false;
      // Step 4.3. Get candidate AMF List for Requested S-NSSAI
      if (!get_valid_amfset(slice_info.getRequestedNssai(), auth_slice_info))
        return false;

      http_code = HTTP_STATUS_CODE_200_OK;
    }
  } else {
    // ToDo: 29.531, R16.0.0.,6.1.6.2.2 Set Configured NSSAI If NSSF did not
    // receive any Requested NSSAI
  }
  return true;
}
//------------------------------------------------------------------------------
bool nssf_slice_select::handle_slice_info_for_pdu_session(
    const SliceInfoForPDUSession& slice_info, const Tai& tai,
    const PlmnId& home_plmnid, const std::string& features, int& http_code,
    const uint8_t http_version, const ProblemDetails& problem_details,
    AuthorizedNetworkSliceInfo& auth_slice_info) {
  Logger::nssf_app().info(
      "NS Selection: Handle case - PDU Session (HTTP_VERSION %d)",
      http_version);

  // Check if UE is Roamer
  RoamingIndication roam_ind = slice_info.getRoamingIndication();
  RoamingIndication_anyOf::eRoamingIndication_anyOf roam_ind_enum =
      roam_ind.getEnumValue();
  if (int(roam_ind_enum) != ROAMING_IND_NON_ROAMING) {
    Logger::nssf_app().warn(
        "NS Selection: Roming/Local Breakout is not Supported yet !!!");
    http_code = HTTP_STATUS_CODE_503_SERVICE_UNAVAILABLE;
    Logger::nssf_app().info(
        "//---------------------------------------------------------");
    Logger::nssf_app().info("");
    return false;
  }

  // Check if UE's HPlmnId is Supported while UE is Roaminng
  if (!home_plmnid.getMcc().empty()) {
    Logger::nssf_app().debug("NS Selection: HomePlmnId is provided !!!");
    // ToDo - Validate PlmnId from nssf config (Currently we don't support
    // Roaming scenario)
    http_code = HTTP_STATUS_CODE_503_SERVICE_UNAVAILABLE;
    Logger::nssf_app().warn(
        "NS Selection: Roming is not Supported yet. "
        "HomePlmnId can not be validated !!");
    Logger::nssf_app().info(
        "//---------------------------------------------------------");
    Logger::nssf_app().info("");
    return false;
  }

  // Check if UE's Tai is Supported
  if (!tai.getTac().empty()) {
    Logger::nssf_app().debug("NS Selection: TAI is provided");
    if (!validate_ta(tai)) {
      http_code = HTTP_STATUS_CODE_400_BAD_REQUEST;
      return false;
    }
  }

  // Check if Supported feature is Supported
  if (!features.empty()) {
    Logger::nssf_app().debug("NS Selection: features are provided");
    // ToDo
    // auth_slice_info.setSupportedFeatures("123abc");
  }

  // Check NSI info for given S-NSSAI is can be provided
  NsiInformation nsi_info = {};
  if (validate_nsi(slice_info, nsi_info)) {
    auth_slice_info.setNsiInformation(nsi_info);
    http_code = HTTP_STATUS_CODE_200_OK;
    return true;
  }
  http_code = HTTP_STATUS_CODE_400_BAD_REQUEST;
  return false;
}

//------------------------------------------------------------------------------
nssf_slice_select::~nssf_slice_select() {
  Logger::nssf_app().debug("Delete NSSF_SLICE_SELECT instance...");
}
