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

/*! \file udr_nrf.cpp
 \brief
 \author  Jian Yang, Fengjiao He, Hongxin Wang, Tien-Thinh NGUYEN
 \company Eurecom
 \date 2020
 \email:
 */

#include "udr_nrf.hpp"
#include "udr_app.hpp"
#include "udr_client.hpp"
#include "udr_profile.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <pistache/http.h>
#include <pistache/mime.h>
#include <stdexcept>

#include "logger.hpp"
#include "udr.h"

using namespace oai::udr::config;
// using namespace udr;
using namespace oai::udr::app;

using json = nlohmann::json;

extern udr_config udr_cfg;
extern udr_nrf* udr_nrf_inst;
udr_client* udr_client_instance = nullptr;

//------------------------------------------------------------------------------
udr_nrf::udr_nrf(udr_event& ev) : m_event_sub(ev) {}
//---------------------------------------------------------------------------------------------
void udr_nrf::get_udr_api_root(std::string& api_root) {
  api_root =
      std::string(inet_ntoa(*((struct in_addr*) &udr_cfg.nrf_addr.ipv4_addr))) +
      ":" + std::to_string(udr_cfg.nrf_addr.port) + NNRF_NFM_BASE +
      udr_cfg.nrf_addr.api_version;
}

//---------------------------------------------------------------------------------------------
void udr_nrf::generate_udr_profile(
    udr_profile& udr_nf_profile, std::string& udr_instance_id) {
  // TODO: remove hardcoded values
  udr_nf_profile.set_nf_instance_id(udr_instance_id);
  udr_nf_profile.set_nf_instance_name("OAI-UDR");
  udr_nf_profile.set_nf_type("UDR");
  udr_nf_profile.set_nf_status("REGISTERED");
  udr_nf_profile.set_nf_heartBeat_timer(50);
  udr_nf_profile.set_nf_priority(1);
  udr_nf_profile.set_nf_capacity(100);
  // udr_nf_profile.set_fqdn(udr_cfg.fqdn);
  udr_nf_profile.add_nf_ipv4_addresses(udr_cfg.nudr.addr4);

  // UDR info (Hardcoded for now)
  // ToDo:-If none of these parameters are provided, the UDR can serve any
  // external group and any SUPI or GPSI managed by the PLMN of the UDR
  // instance. If "supiRanges", "gpsiRanges" and
  // "externalGroupIdentifiersRanges" attributes are absent, and "groupId" is
  // present, the SUPIs / GPSIs / ExternalGroups served by this UDR instance is
  // determined by the NRF (see 3GPP TS 23.501 [2], clause 6.2.6.2).
  udr_info_t udr_info_item;
  supi_range_udr_info_item_t supi_ranges;
  udr_info_item.groupid = "oai-udr-testgroupid";
  udr_info_item.data_set_id.push_back("0210");
  udr_info_item.data_set_id.push_back("9876");
  supi_ranges.supi_range.start   = "001010000000031";
  supi_ranges.supi_range.pattern = "^imsi-00101[31-131]{6}$";
  supi_ranges.supi_range.start   = "001010000000031";
  udr_info_item.supi_ranges.push_back(supi_ranges);
  identity_range_udr_info_item_t gpsi_ranges;
  gpsi_ranges.identity_range.start   = "752740000";
  gpsi_ranges.identity_range.pattern = "^gpsi-75274[0-9]{4}$";
  gpsi_ranges.identity_range.end     = "752749999";
  udr_info_item.gpsi_ranges.push_back(gpsi_ranges);
  udr_nf_profile.set_udr_info(udr_info_item);
  // ToDo:- Add remaining fields
  // UDR info item end

  udr_nf_profile.display();
}
//---------------------------------------------------------------------------------------------
void udr_nrf::register_to_nrf() {
  // generate UUID
  udr_instance_id              = to_string(boost::uuids::random_generator()());
  nlohmann::json response_data = {};

  // Generate NF Profile
  udr_profile udr_nf_profile;
  generate_udr_profile(udr_nf_profile, udr_instance_id);

  // Send NF registeration request
  std::string udr_api_root = {};
  std::string response     = {};
  std::string method       = {"PUT"};
  get_udr_api_root(udr_api_root);
  std::string remoteUri = udr_api_root + UDR_NF_REGISTER_URL + udr_instance_id;
  nlohmann::json json_data = {};
  udr_nf_profile.to_json(json_data);

  Logger::udr_nrf().info("Sending NF registeration request");
  udr_client_instance->curl_http_client(
      remoteUri, method, json_data.dump().c_str(), response);

  try {
    response_data = nlohmann::json::parse(response);
    if (response.find("REGISTERED") != 0) {
      start_event_nf_heartbeat(remoteUri);
    }
  } catch (nlohmann::json::exception& e) {
    Logger::udr_nrf().info("NF registeration procedure failed");
  }
}
//---------------------------------------------------------------------------------------------
void udr_nrf::start_event_nf_heartbeat(std::string& remoteURI) {
  // get current time
  uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  struct itimerspec its;
  its.it_value.tv_sec  = HEART_BEAT_TIMER;  // seconds
  its.it_value.tv_nsec = 0;                 // 100 * 1000 * 1000; //100ms
  const uint64_t interval =
      its.it_value.tv_sec * 1000 +
      its.it_value.tv_nsec / 1000000;  // convert sec, nsec to msec

  task_connection = m_event_sub.subscribe_task_nf_heartbeat(
      boost::bind(&udr_nrf::trigger_nf_heartbeat_procedure, this, _1), interval,
      ms + interval);
}
//---------------------------------------------------------------------------------------------
void udr_nrf::trigger_nf_heartbeat_procedure(uint64_t ms) {
  _unused(ms);
  oai::udr::model::PatchItem patch_item = {};
  std::vector<oai::udr::model::PatchItem> patch_items;
  //{"op":"replace","path":"/nfStatus", "value": "REGISTERED"}
  patch_item.setOp("replace");
  patch_item.setPath("/nfStatus");
  patch_item.setValue("REGISTERED");
  patch_items.push_back(patch_item);
  Logger::udr_nrf().info("Sending NF heartbeat request");

  std::string response     = {};
  std::string method       = {"PATCH"};
  nlohmann::json json_data = nlohmann::json::array();
  for (auto i : patch_items) {
    nlohmann::json item = {};
    to_json(item, i);
    json_data.push_back(item);
  }

  std::string udr_api_root = {};
  get_udr_api_root(udr_api_root);
  std::string remoteUri = udr_api_root + UDR_NF_REGISTER_URL + udr_instance_id;
  udr_client_instance->curl_http_client(
      remoteUri, method, json_data.dump().c_str(), response);
  if (!response.empty()) task_connection.disconnect();
}
