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

/*! \file udr_profile.cpp
 \brief
 \author  Tien-Thinh NGUYEN
 \company Eurecom
 \date 2021
 \email: Tien-Thinh.Nguyen@eurecom.fr
 */

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "fqdn.hpp"
#include "logger.hpp"
#include "string.hpp"
#include "udr_profile.hpp"

// using namespace udr;
using namespace oai::udr::app;

//------------------------------------------------------------------------------
void udr_profile::set_nf_instance_id(const std::string& instance_id) {
  nf_instance_id = instance_id;
}

//------------------------------------------------------------------------------
void udr_profile::get_nf_instance_id(std::string& instance_id) const {
  instance_id = nf_instance_id;
}

//------------------------------------------------------------------------------
std::string udr_profile::get_nf_instance_id() const {
  return nf_instance_id;
}

//------------------------------------------------------------------------------
void udr_profile::set_nf_instance_name(const std::string& instance_name) {
  nf_instance_name = instance_name;
}

//------------------------------------------------------------------------------
void udr_profile::get_nf_instance_name(std::string& instance_name) const {
  instance_name = nf_instance_name;
}

//------------------------------------------------------------------------------
std::string udr_profile::get_nf_instance_name() const {
  return nf_instance_name;
}

//------------------------------------------------------------------------------
void udr_profile::set_nf_type(const std::string& type) {
  nf_type = type;
}

//------------------------------------------------------------------------------
std::string udr_profile::get_nf_type() const {
  return nf_type;
}
//------------------------------------------------------------------------------
void udr_profile::set_nf_status(const std::string& status) {
  nf_status = status;
}

//------------------------------------------------------------------------------
void udr_profile::get_nf_status(std::string& status) const {
  status = nf_status;
}

//------------------------------------------------------------------------------
std::string udr_profile::get_nf_status() const {
  return nf_status;
}

//------------------------------------------------------------------------------
void udr_profile::set_nf_heartBeat_timer(const int32_t& timer) {
  heartBeat_timer = timer;
}

//------------------------------------------------------------------------------
void udr_profile::get_nf_heartBeat_timer(int32_t& timer) const {
  timer = heartBeat_timer;
}

//------------------------------------------------------------------------------
int32_t udr_profile::get_nf_heartBeat_timer() const {
  return heartBeat_timer;
}

//------------------------------------------------------------------------------
void udr_profile::set_nf_priority(const uint16_t& p) {
  priority = p;
}

//------------------------------------------------------------------------------
void udr_profile::get_nf_priority(uint16_t& p) const {
  p = priority;
}

//------------------------------------------------------------------------------
uint16_t udr_profile::get_nf_priority() const {
  return priority;
}

//------------------------------------------------------------------------------
void udr_profile::set_nf_capacity(const uint16_t& c) {
  capacity = c;
}

//------------------------------------------------------------------------------
void udr_profile::get_nf_capacity(uint16_t& c) const {
  c = capacity;
}

//------------------------------------------------------------------------------
uint16_t udr_profile::get_nf_capacity() const {
  return capacity;
}

//------------------------------------------------------------------------------
void udr_profile::set_nf_snssais(const std::vector<snssai_t>& s) {
  snssais = s;
}

//------------------------------------------------------------------------------
void udr_profile::get_nf_snssais(std::vector<snssai_t>& s) const {
  s = snssais;
}

//------------------------------------------------------------------------------
void udr_profile::add_snssai(const snssai_t& s) {
  snssais.push_back(s);
}

//------------------------------------------------------------------------------
void udr_profile::set_fqdn(const std::string& fqdN) {
  fqdn = fqdN;
}

//------------------------------------------------------------------------------
std::string udr_profile::get_fqdn() const {
  return fqdn;
}

//------------------------------------------------------------------------------
void udr_profile::set_nf_ipv4_addresses(const std::vector<struct in_addr>& a) {
  ipv4_addresses = a;
}

//------------------------------------------------------------------------------
void udr_profile::add_nf_ipv4_addresses(const struct in_addr& a) {
  ipv4_addresses.push_back(a);
}
//------------------------------------------------------------------------------
void udr_profile::get_nf_ipv4_addresses(std::vector<struct in_addr>& a) const {
  a = ipv4_addresses;
}

//------------------------------------------------------------------------------
void udr_profile::set_udr_info(const udr_info_t& s) {
  udr_info = s;
}

//------------------------------------------------------------------------------
void udr_profile::get_udr_info(udr_info_t& s) const {
  s = udr_info;
}

//------------------------------------------------------------------------------
void udr_profile::display() const {
  Logger::udr_app().debug("- NF instance info");
  Logger::udr_app().debug("    Instance ID: %s", nf_instance_id.c_str());
  Logger::udr_app().debug("    Instance name: %s", nf_instance_name.c_str());
  Logger::udr_app().debug("    Instance type: %s", nf_type.c_str());
  Logger::udr_app().debug("    Instance fqdn: %s", fqdn.c_str());
  Logger::udr_app().debug("    Status: %s", nf_status.c_str());
  Logger::udr_app().debug("    HeartBeat timer: %d", heartBeat_timer);
  Logger::udr_app().debug("    Priority: %d", priority);
  Logger::udr_app().debug("    Capacity: %d", capacity);
  // SNSSAIs
  if (snssais.size() > 0) {
    Logger::udr_app().debug("    SNSSAI:");
  }
  for (auto s : snssais) {
    Logger::udr_app().debug("        SST, SD: %d, %s", s.sST, s.sD.c_str());
  }

  // IPv4 Addresses
  if (ipv4_addresses.size() > 0) {
    Logger::udr_app().debug("    IPv4 Addr:");
  }
  for (auto address : ipv4_addresses) {
    Logger::udr_app().debug("        %s", inet_ntoa(address));
  }

  Logger::udr_app().debug("\tUDR Info");
  Logger::udr_app().debug("\t\tGroupId: %s", udr_info.groupid.c_str());
  for (auto supi : udr_info.supi_ranges) {
    Logger::udr_app().debug(
        "\t\t SupiRanges: Start - %s, End - %s, Pattern - %s",
        supi.supi_range.start.c_str(), supi.supi_range.end.c_str(),
        supi.supi_range.pattern.c_str());
  }
  for (auto gpsi : udr_info.gpsi_ranges) {
    Logger::udr_app().debug(
        "\t\t GpsiRanges: Start - %s, End - %s, Pattern - %s",
        gpsi.identity_range.start.c_str(), gpsi.identity_range.end.c_str(),
        gpsi.identity_range.pattern.c_str());
  }
  for (auto ext_grp_id : udr_info.ext_grp_id_ranges) {
    Logger::udr_app().debug(
        "\t\t externalGroupIdentifiersRanges: Start - %s, "
        "End - %s, Pattern - %s",
        ext_grp_id.identity_range.start.c_str(),
        ext_grp_id.identity_range.end.c_str(),
        ext_grp_id.identity_range.pattern.c_str());
  }
  for (auto data_set_Id : udr_info.data_set_id) {
    Logger::udr_app().debug("\t\t Data Set Id: %s", data_set_Id.c_str());
  }
}

//------------------------------------------------------------------------------
void udr_profile::to_json(nlohmann::json& data) const {
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

  // UDR Info
  data["udrInfo"]["groupId"]                        = udr_info.groupid;
  data["udrInfo"]["supiRanges"]                     = nlohmann::json::array();
  data["udrInfo"]["gpsiRanges"]                     = nlohmann::json::array();
  data["udrInfo"]["externalGroupIdentifiersRanges"] = nlohmann::json::array();
  data["udrInfo"]["DataSetId"]                      = nlohmann::json::array();
  for (auto supi : udr_info.supi_ranges) {
    nlohmann::json tmp = {};
    tmp["start"]       = supi.supi_range.start;
    tmp["end"]         = supi.supi_range.end;
    tmp["pattern"]     = supi.supi_range.pattern;
    data["udrInfo"]["supiRanges"].push_back(tmp);
  }
  for (auto gpsi : udr_info.gpsi_ranges) {
    nlohmann::json tmp = {};
    tmp["start"]       = gpsi.identity_range.start;
    tmp["end"]         = gpsi.identity_range.end;
    tmp["pattern"]     = gpsi.identity_range.pattern;
    data["udrInfo"]["gpsiRanges"].push_back(tmp);
  }
  for (auto ext_grp_id : udr_info.ext_grp_id_ranges) {
    nlohmann::json tmp = {};
    tmp["start"]       = ext_grp_id.identity_range.start;
    tmp["end"]         = ext_grp_id.identity_range.end;
    tmp["pattern"]     = ext_grp_id.identity_range.pattern;
    data["udrInfo"]["externalGroupIdentifiersRanges"].push_back(tmp);
  }
  for (auto data_set_Id : udr_info.data_set_id) {
    std::string tmp = data_set_Id;
    data["udrInfo"]["DataSetId"].push_back(data_set_Id);
  }

  Logger::udr_app().debug("udr profile to JSON:\n %s", data.dump().c_str());
}

//------------------------------------------------------------------------------
void udr_profile::from_json(const nlohmann::json& data) {
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
        Logger::udr_app().warn(
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

  // UDR info
  if (data.find("udrInfo") != data.end()) {
    nlohmann::json info = data["udrInfo"];
    if (info.find("groupId") != info.end()) {
      udr_info.groupid = info["groupId"].get<std::string>();
    }
    if (info.find("DataSetId") != info.end()) {
      nlohmann::json data_set_id_list = data["udrInfo"]["DataSetId"];
      for (auto d : data_set_id_list) {
        udr_info.data_set_id.push_back(d);
      }
    }
    if (info.find("supiRanges") != info.end()) {
      nlohmann::json supi_ranges = data["udrInfo"]["supiRanges"];
      for (auto d : supi_ranges) {
        supi_range_udr_info_item_t supi;
        supi.supi_range.start   = d["start"];
        supi.supi_range.end     = d["end"];
        supi.supi_range.pattern = d["pattern"];
        udr_info.supi_ranges.push_back(supi);
      }
    }
    if (info.find("gpsiRanges") != info.end()) {
      nlohmann::json gpsi_ranges = data["udrInfo"]["gpsiRanges"];
      for (auto d : gpsi_ranges) {
        identity_range_udr_info_item_t gpsi;
        gpsi.identity_range.start   = d["start"];
        gpsi.identity_range.end     = d["end"];
        gpsi.identity_range.pattern = d["pattern"];
        udr_info.gpsi_ranges.push_back(gpsi);
      }
    }
    if (info.find("externalGroupIdentifiersRanges") != info.end()) {
      nlohmann::json ext_grp_id_ranges =
          data["udrInfo"]["externalGroupIdentifiersRanges"];
      for (auto d : ext_grp_id_ranges) {
        identity_range_udr_info_item_t ext_grp_id;
        ext_grp_id.identity_range.start   = d["start"];
        ext_grp_id.identity_range.end     = d["end"];
        ext_grp_id.identity_range.pattern = d["pattern"];
        udr_info.ext_grp_id_ranges.push_back(ext_grp_id);
      }
    }
  }
  display();
}

//------------------------------------------------------------------------------
void udr_profile::handle_heartbeart_timeout(uint64_t ms) {
  Logger::udr_app().info(
      "Handle heartbeart timeout profile %s, time %d", nf_instance_id.c_str(),
      ms);
  set_nf_status("SUSPENDED");
}
