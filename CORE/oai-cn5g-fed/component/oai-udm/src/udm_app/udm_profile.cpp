/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this
 *file except in compliance with the License. You may obtain a copy of the
 *License at
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

/*! \file udm_profile.cpp
 \brief
 \author  Tien-Thinh NGUYEN
 \company Eurecom
 \date 2021
 \email: Tien-Thinh.Nguyen@eurecom.fr
 */

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "logger.hpp"
#include "udm_profile.hpp"
#include "string.hpp"

// using namespace udm;
using namespace oai::udm::app;

//------------------------------------------------------------------------------
void udm_profile::set_nf_instance_id(const std::string& instance_id) {
  nf_instance_id = instance_id;
}

//------------------------------------------------------------------------------
void udm_profile::get_nf_instance_id(std::string& instance_id) const {
  instance_id = nf_instance_id;
}

//------------------------------------------------------------------------------
std::string udm_profile::get_nf_instance_id() const {
  return nf_instance_id;
}

//------------------------------------------------------------------------------
void udm_profile::set_nf_instance_name(const std::string& instance_name) {
  nf_instance_name = instance_name;
}

//------------------------------------------------------------------------------
void udm_profile::get_nf_instance_name(std::string& instance_name) const {
  instance_name = nf_instance_name;
}

//------------------------------------------------------------------------------
std::string udm_profile::get_nf_instance_name() const {
  return nf_instance_name;
}

//------------------------------------------------------------------------------
void udm_profile::set_nf_type(const std::string& type) {
  nf_type = type;
}

//------------------------------------------------------------------------------
std::string udm_profile::get_nf_type() const {
  return nf_type;
}
//------------------------------------------------------------------------------
void udm_profile::set_nf_status(const std::string& status) {
  nf_status = status;
}

//------------------------------------------------------------------------------
void udm_profile::get_nf_status(std::string& status) const {
  status = nf_status;
}

//------------------------------------------------------------------------------
std::string udm_profile::get_nf_status() const {
  return nf_status;
}

//------------------------------------------------------------------------------
void udm_profile::set_nf_heartBeat_timer(const int32_t& timer) {
  heartBeat_timer = timer;
}

//------------------------------------------------------------------------------
void udm_profile::get_nf_heartBeat_timer(int32_t& timer) const {
  timer = heartBeat_timer;
}

//------------------------------------------------------------------------------
int32_t udm_profile::get_nf_heartBeat_timer() const {
  return heartBeat_timer;
}

//------------------------------------------------------------------------------
void udm_profile::set_nf_priority(const uint16_t& p) {
  priority = p;
}

//------------------------------------------------------------------------------
void udm_profile::get_nf_priority(uint16_t& p) const {
  p = priority;
}

//------------------------------------------------------------------------------
uint16_t udm_profile::get_nf_priority() const {
  return priority;
}

//------------------------------------------------------------------------------
void udm_profile::set_nf_capacity(const uint16_t& c) {
  capacity = c;
}

//------------------------------------------------------------------------------
void udm_profile::get_nf_capacity(uint16_t& c) const {
  c = capacity;
}

//------------------------------------------------------------------------------
uint16_t udm_profile::get_nf_capacity() const {
  return capacity;
}

//------------------------------------------------------------------------------
void udm_profile::set_nf_snssais(const std::vector<snssai_t>& s) {
  snssais = s;
}

//------------------------------------------------------------------------------
void udm_profile::get_nf_snssais(std::vector<snssai_t>& s) const {
  s = snssais;
}

//------------------------------------------------------------------------------
void udm_profile::add_snssai(const snssai_t& s) {
  snssais.push_back(s);
}

//------------------------------------------------------------------------------
void udm_profile::set_fqdn(const std::string& fqdN) {
  fqdn = fqdN;
}

//------------------------------------------------------------------------------
std::string udm_profile::get_fqdn() const {
  return fqdn;
}

//------------------------------------------------------------------------------
void udm_profile::set_nf_ipv4_addresses(const std::vector<struct in_addr>& a) {
  ipv4_addresses = a;
}

//------------------------------------------------------------------------------
void udm_profile::add_nf_ipv4_addresses(const struct in_addr& a) {
  ipv4_addresses.push_back(a);
}
//------------------------------------------------------------------------------
void udm_profile::get_nf_ipv4_addresses(std::vector<struct in_addr>& a) const {
  a = ipv4_addresses;
}

//------------------------------------------------------------------------------
void udm_profile::set_udm_info(const udm_info_t& s) {
  udm_info = s;
}

//------------------------------------------------------------------------------
void udm_profile::get_udm_info(udm_info_t& s) const {
  s = udm_info;
}

//------------------------------------------------------------------------------
void udm_profile::display() const {
  Logger::udm_app().debug("- NF instance info");
  Logger::udm_app().debug("    Instance ID: %s", nf_instance_id.c_str());
  Logger::udm_app().debug("    Instance name: %s", nf_instance_name.c_str());
  Logger::udm_app().debug("    Instance type: %s", nf_type.c_str());
  Logger::udm_app().debug("    Instance fqdn: %s", fqdn.c_str());
  Logger::udm_app().debug("    Status: %s", nf_status.c_str());
  Logger::udm_app().debug("    HeartBeat timer: %d", heartBeat_timer);
  Logger::udm_app().debug("    Priority: %d", priority);
  Logger::udm_app().debug("    Capacity: %d", capacity);
  // SNSSAIs
  if (snssais.size() > 0) {
    Logger::udm_app().debug("    SNSSAI:");
  }
  for (auto s : snssais) {
    Logger::udm_app().debug("        SST, SD: %d, %s", s.sST, s.sD.c_str());
  }

  // IPv4 Addresses
  if (ipv4_addresses.size() > 0) {
    Logger::udm_app().debug("    IPv4 Addr:");
  }
  for (auto address : ipv4_addresses) {
    Logger::udm_app().debug("        %s", inet_ntoa(address));
  }

  Logger::udm_app().debug("\tUDM Info");
  Logger::udm_app().debug("\t\tGroupId: %s", udm_info.groupid.c_str());
  for (auto supi : udm_info.supi_ranges) {
    Logger::udm_app().debug(
        "\t\t SupiRanges: Start - %s, End - %s, Pattern - %s",
        supi.supi_range.start.c_str(), supi.supi_range.end.c_str(),
        supi.supi_range.pattern.c_str());
  }
  for (auto gpsi : udm_info.gpsi_ranges) {
    Logger::udm_app().debug(
        "\t\t GpsiRanges: Start - %s, End - %s, Pattern - %s",
        gpsi.identity_range.start.c_str(), gpsi.identity_range.end.c_str(),
        gpsi.identity_range.pattern.c_str());
  }
  for (auto ext_grp_id : udm_info.ext_grp_id_ranges) {
    Logger::udm_app().debug(
        "\t\t externalGroupIdentifiersRanges: Start - %s, End - %s, Pattern - "
        "%s",
        ext_grp_id.identity_range.start.c_str(),
        ext_grp_id.identity_range.end.c_str(),
        ext_grp_id.identity_range.pattern.c_str());
  }
  for (auto int_grp_id : udm_info.int_grp_id_ranges) {
    Logger::udm_app().debug(
        "\t\t internalGroupIdentifiersRanges: Start - %s, End - %s, Pattern - "
        "%s",
        int_grp_id.int_grpid_range.start.c_str(),
        int_grp_id.int_grpid_range.end.c_str(),
        int_grp_id.int_grpid_range.pattern.c_str());
  }
  for (auto route_ind : udm_info.routing_indicators) {
    Logger::udm_app().debug("\t\t Routing Indicators: %s", route_ind.c_str());
  }
}

//------------------------------------------------------------------------------
void udm_profile::to_json(nlohmann::json& data) const {
  data["nfInstanceId"]   = nf_instance_id;
  data["nfInstanceName"] = nf_instance_name;
  data["nfType"]         = nf_type;
  data["nfStatus"]       = nf_status;
  data["heartBeatTimer"] = heartBeat_timer;
  // SNSSAIs
  data["sNssais"] = nlohmann::json::array();
  for (auto s : snssais) {
    nlohmann::json tmp = {};
    tmp["sst"]         = s.sST;
    tmp["sd"]          = s.sD;
    data["sNssais"].push_back(tmp);
  }
  data["fqdn"] = fqdn;
  // ipv4_addresses
  data["ipv4Addresses"] = nlohmann::json::array();
  for (auto address : ipv4_addresses) {
    nlohmann::json tmp = inet_ntoa(address);
    data["ipv4Addresses"].push_back(tmp);
  }

  data["priority"] = priority;
  data["capacity"] = capacity;

  // UDM Info
  data["udmInfo"]["groupId"]                        = udm_info.groupid;
  data["udmInfo"]["supiRanges"]                     = nlohmann::json::array();
  data["udmInfo"]["gpsiRanges"]                     = nlohmann::json::array();
  data["udmInfo"]["externalGroupIdentifiersRanges"] = nlohmann::json::array();
  data["udmInfo"]["routingIndicators"]              = nlohmann::json::array();
  data["udmInfo"]["internalGroupIdentifiersRanges"] = nlohmann::json::array();

  for (auto supi : udm_info.supi_ranges) {
    nlohmann::json tmp = {};
    tmp["start"]       = supi.supi_range.start;
    tmp["end"]         = supi.supi_range.end;
    tmp["pattern"]     = supi.supi_range.pattern;
    data["udmInfo"]["supiRanges"].push_back(tmp);
  }
  for (auto gpsi : udm_info.gpsi_ranges) {
    nlohmann::json tmp = {};
    tmp["start"]       = gpsi.identity_range.start;
    tmp["end"]         = gpsi.identity_range.end;
    tmp["pattern"]     = gpsi.identity_range.pattern;
    data["udmInfo"]["gpsiRanges"].push_back(tmp);
  }
  for (auto ext_grp_id : udm_info.ext_grp_id_ranges) {
    nlohmann::json tmp = {};
    tmp["start"]       = ext_grp_id.identity_range.start;
    tmp["end"]         = ext_grp_id.identity_range.end;
    tmp["pattern"]     = ext_grp_id.identity_range.pattern;
    data["udmInfo"]["externalGroupIdentifiersRanges"].push_back(tmp);
  }
  for (auto route_ind : udm_info.routing_indicators) {
    std::string tmp = route_ind;
    data["udmInfo"]["routingIndicators"].push_back(route_ind);
  }
  for (auto int_grp_id : udm_info.int_grp_id_ranges) {
    nlohmann::json tmp = {};
    tmp["start"]       = int_grp_id.int_grpid_range.start;
    tmp["end"]         = int_grp_id.int_grpid_range.end;
    tmp["pattern"]     = int_grp_id.int_grpid_range.pattern;
    data["udmInfo"]["internalGroupIdentifiersRanges"].push_back(tmp);
  }
  Logger::udm_app().debug("udm profile to JSON:\n %s", data.dump().c_str());
}

//------------------------------------------------------------------------------
void udm_profile::from_json(const nlohmann::json& data) {
  if (data.find("nfInstanceId") != data.end()) {
    nf_instance_id = data["nfInstanceId"].get<std::string>();
  }

  if (data.find("nfInstanceName") != data.end()) {
    nf_instance_name = data["nfInstanceName"].get<std::string>();
  }

  if (data.find("nfType") != data.end()) {
    nf_type = data["nfType"].get<std::string>();
  }

  if (data.find("nfStatus") != data.end()) {
    nf_status = data["nfStatus"].get<std::string>();
  }

  if (data.find("heartBeatTimer") != data.end()) {
    heartBeat_timer = data["heartBeatTimer"].get<int>();
  }
  // sNssais
  if (data.find("sNssais") != data.end()) {
    for (auto it : data["sNssais"]) {
      snssai_t s = {};
      s.sST      = it["sst"].get<int>();
      s.sD       = it["sd"].get<std::string>();
      snssais.push_back(s);
    }
  }

  if (data.find("ipv4Addresses") != data.end()) {
    nlohmann::json addresses = data["ipv4Addresses"];

    for (auto it : addresses) {
      struct in_addr addr4 = {};
      std::string address  = it.get<std::string>();
      unsigned char buf_in_addr[sizeof(struct in_addr)];
      if (inet_pton(AF_INET, util::trim(address).c_str(), buf_in_addr) == 1) {
        memcpy(&addr4, buf_in_addr, sizeof(struct in_addr));
      } else {
        Logger::udm_app().warn(
            "Address conversion: Bad value %s", util::trim(address).c_str());
      }
      add_nf_ipv4_addresses(addr4);
    }
  }

  if (data.find("priority") != data.end()) {
    priority = data["priority"].get<int>();
  }

  if (data.find("capacity") != data.end()) {
    capacity = data["capacity"].get<int>();
  }

  // UDM info
  if (data.find("udmInfo") != data.end()) {
    nlohmann::json info = data["udmInfo"];
    if (info.find("groupId") != info.end()) {
      udm_info.groupid = info["groupId"].get<std::string>();
    }
    if (info.find("supiRanges") != info.end()) {
      nlohmann::json supi_ranges = data["udmInfo"]["supiRanges"];
      for (auto d : supi_ranges) {
        supi_range_udm_info_item_t supi;
        supi.supi_range.start   = d["start"];
        supi.supi_range.end     = d["end"];
        supi.supi_range.pattern = d["pattern"];
        udm_info.supi_ranges.push_back(supi);
      }
    }
    if (info.find("gpsiRanges") != info.end()) {
      nlohmann::json gpsi_ranges = data["udmInfo"]["gpsiRanges"];
      for (auto d : gpsi_ranges) {
        identity_range_udm_info_item_t gpsi;
        gpsi.identity_range.start   = d["start"];
        gpsi.identity_range.end     = d["end"];
        gpsi.identity_range.pattern = d["pattern"];
        udm_info.gpsi_ranges.push_back(gpsi);
      }
    }
    if (info.find("externalGroupIdentifiersRanges") != info.end()) {
      nlohmann::json ext_grp_id_ranges =
          data["udmInfo"]["externalGroupIdentifiersRanges"];
      for (auto d : ext_grp_id_ranges) {
        identity_range_udm_info_item_t ext_grp_id;
        ext_grp_id.identity_range.start   = d["start"];
        ext_grp_id.identity_range.end     = d["end"];
        ext_grp_id.identity_range.pattern = d["pattern"];
        udm_info.ext_grp_id_ranges.push_back(ext_grp_id);
      }
    }
    if (info.find("routingIndicators") != info.end()) {
      nlohmann::json routing_indicators_list =
          data["udmInfo"]["routingIndicators"];
      for (auto d : routing_indicators_list) {
        udm_info.routing_indicators.push_back(d);
      }
    }
    if (info.find("internalGroupIdentifiersRanges") != info.end()) {
      nlohmann::json int_grp_id_ranges =
          data["udmInfo"]["internalGroupIdentifiersRanges"];
      for (auto d : int_grp_id_ranges) {
        internal_grpid_range_udm_info_item_t int_grp_id;
        int_grp_id.int_grpid_range.start   = d["start"];
        int_grp_id.int_grpid_range.end     = d["end"];
        int_grp_id.int_grpid_range.pattern = d["pattern"];
        udm_info.int_grp_id_ranges.push_back(int_grp_id);
      }
    }
  }
  display();
}

//------------------------------------------------------------------------------
void udm_profile::handle_heartbeart_timeout(uint64_t ms) {
  Logger::udm_app().info(
      "Handle heartbeart timeout profile %s, time %d", nf_instance_id.c_str(),
      ms);
  set_nf_status("SUSPENDED");
}
