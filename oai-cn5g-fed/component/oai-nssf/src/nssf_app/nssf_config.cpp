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

#include "nssf_config.hpp"
#include "common_defs.h"
#include "conversions.hpp"
#include "if.hpp"
#include "string.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace libconfig;
using namespace nssf;

#define kJsonFileBuffer (1024)

nssf_nsi_info_t nssf_config::nssf_nsi_info;
nssf_ta_info_t nssf_config::nssf_ta_info;
nssf_amf_info_t nssf_config::nssf_amf_info;
std::string nssf_config::slice_config_file;
// nlohmann::json nssf_config::nssf_slice_config;

// C includes
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

//------------------------------------------------------------------------------
int nssf_config::execute() {
  return RETURNok;
}

//------------------------------------------------------------------------------
int nssf_config::load_interface(const Setting& if_cfg, interface_cfg_t& cfg) {
  if_cfg.lookupValue(NSSF_CONFIG_STRING_INTERFACE_NAME, cfg.if_name);
  util::trim(cfg.if_name);
  if (not boost::iequals(cfg.if_name, "none")) {
    std::string address = {};
    if_cfg.lookupValue(NSSF_CONFIG_STRING_IPV4_ADDRESS, address);
    util::trim(address);
    if (boost::iequals(address, "read")) {
      if (get_inet_addr_infos_from_iface(
              cfg.if_name, cfg.addr4, cfg.network4, cfg.mtu)) {
        Logger::nssf_app().error(
            "Could not read %s network interface configuration", cfg.if_name);
        return RETURNerror;
      }
    } else {
      std::vector<std::string> words;
      boost::split(
          words, address, boost::is_any_of("/"), boost::token_compress_on);
      if (words.size() != 2) {
        Logger::nssf_app().error(
            "Bad value " NSSF_CONFIG_STRING_IPV4_ADDRESS " = %s in config file",
            address.c_str());
        return RETURNerror;
      }
      unsigned char buf_in_addr[sizeof(struct in6_addr)];  // you never know...
      if (inet_pton(AF_INET, util::trim(words.at(0)).c_str(), buf_in_addr) ==
          1) {
        memcpy(&cfg.addr4, buf_in_addr, sizeof(struct in_addr));
      } else {
        Logger::nssf_app().error(
            "In conversion: Bad value " NSSF_CONFIG_STRING_IPV4_ADDRESS
            " = %s in config file",
            util::trim(words.at(0)).c_str());
        return RETURNerror;
      }
      cfg.network4.s_addr = htons(
          ntohs(cfg.addr4.s_addr) &
          0xFFFFFFFF << (32 - std::stoi(util::trim(words.at(1)))));
    }
    if_cfg.lookupValue(NSSF_CONFIG_STRING_SBI_PORT_HTTP1, cfg.http1_port);
    if_cfg.lookupValue(NSSF_CONFIG_STRING_SBI_PORT_HTTP2, cfg.http2_port);
  }
  return RETURNok;
}

//------------------------------------------------------------------------------
int nssf_config::load(const string& config_file) {
  Config cfg;
  unsigned char buf_in_addr[sizeof(struct in_addr) + 1];
  unsigned char buf_in6_addr[sizeof(struct in6_addr) + 1];

  // Read the file. If there is an error, report it and exit.
  try {
    cfg.readFile(config_file.c_str());
  } catch (const FileIOException& fioex) {
    Logger::nssf_app().error(
        "I/O error while reading file %s - %s", config_file.c_str(),
        fioex.what());
    throw;
  } catch (const ParseException& pex) {
    Logger::nssf_app().error(
        "Parse error at %s:%d - %s", pex.getFile(), pex.getLine(),
        pex.getError());
    throw;
  }

  const Setting& root = cfg.getRoot();

  try {
    const Setting& nssf_cfg = root[NSSF_CONFIG_STRING_NSSF_CONFIG];
  } catch (const SettingNotFoundException& nfex) {
    Logger::nssf_app().error("%s : %s", nfex.what(), nfex.getPath());
    return RETURNerror;
  }

  const Setting& nssf_cfg = root[NSSF_CONFIG_STRING_NSSF_CONFIG];

  try {
    nssf_cfg.lookupValue(
        NSSF_CONFIG_STRING_NSSF_SLICE_CONFIG, slice_config_file);
  } catch (const SettingNotFoundException& nfex) {
    Logger::nssf_app().info(
        "%s : %s, No slice_config_file configured", nfex.what(),
        nfex.getPath());
  }

  try {
    nssf_cfg.lookupValue(NSSF_CONFIG_STRING_FQDN, fqdn);
    util::trim(fqdn);
  } catch (const SettingNotFoundException& nfex) {
    Logger::nssf_app().info(
        "%s : %s, No FQDN configured", nfex.what(), nfex.getPath());
  }

  try {
    const Setting& nw_if_cfg = nssf_cfg[NSSF_CONFIG_STRING_INTERFACES];

    const Setting& sbi_cfg = nw_if_cfg[NSSF_CONFIG_STRING_SBI_INTERFACE];
    load_interface(sbi_cfg, sbi);

    sbi_cfg.lookupValue(NSSF_CONFIG_STRING_SBI_API_VERSION, sbi_api_version);
  } catch (const SettingNotFoundException& nfex) {
    Logger::nssf_app().error("%s : %s", nfex.what(), nfex.getPath());
    return RETURNerror;
  }
  return RETURNok;
}

//------------------------------------------------------------------------------
void nssf_config::display() {
  Logger::nssf_app().info(
      "==== OPENAIRINTERFACE %s v%s ====", PACKAGE_NAME, PACKAGE_VERSION);
  Logger::nssf_app().info("Configuration:");
  Logger::nssf_app().info("- FQDN ..................: %s", fqdn.c_str());
  Logger::nssf_app().info("- SBI:");
  Logger::nssf_app().info("    iface ............: %s", sbi.if_name.c_str());
  Logger::nssf_app().info("    ipv4.addr ........: %s", inet_ntoa(sbi.addr4));
  Logger::nssf_app().info(
      "    ipv4.mask ........: %s", inet_ntoa(sbi.network4));
  Logger::nssf_app().info("    mtu ..............: %d", sbi.mtu);
  Logger::nssf_app().info("    http1_port .......: %u", sbi.http1_port);
  Logger::nssf_app().info("    http2_port .......: %u", sbi.http2_port);
  Logger::nssf_app().info(
      "    api_version ......: %s", sbi_api_version.c_str());
}

//------------------------------------------------------------------------------
const bool nssf_config::parse_nsi_info(
    const YAML::Node& conf, nssf_nsi_info_t& cfg) {
  static std::mutex mutex;
  try {
    for (YAML::const_iterator it = conf.begin(); it != conf.end(); ++it) {
      nsi_info_t nsi_info;
      const YAML::Node& nsiInfo = *it;

      // nsiInformationList
      nsi_info.nsi_info.setNrfId(
          nsiInfo["nsiInformationList"]["nrfId"].as<string>());
      if (nsiInfo["nsiInformationList"]["nsiId"])
        nsi_info.nsi_info.setNsiId(
            nsiInfo["nsiInformationList"]["nsiId"].as<string>());
      if (nsiInfo["nsiInformationList"]["nrfNfMgtUri"])
        nsi_info.nsi_info.setNrfNfMgtUri(
            nsiInfo["nsiInformationList"]["nrfNfMgtUri"].as<string>());

      // snssai
      nsi_info.snssai.setSst(nsiInfo["snssai"]["sst"].as<int32_t>());
      if (nsiInfo["snssai"]["sd"])
        nsi_info.snssai.setSd(nsiInfo["snssai"]["sd"].as<string>());

      std::lock_guard<std::mutex> lock(mutex);
      cfg.nsi_info_list.push_back(nsi_info);
    }
    cfg.nsi_info_list.shrink_to_fit();
  } catch (std::exception& e) {
    Logger::nssf_app().error("Eror parsing NsiInfo");
    return false;
  }
  return true;
}
//------------------------------------------------------------------------------
const bool nssf_config::parse_ta_info(
    const YAML::Node& conf, nssf_ta_info_t& cfg) {
  static std::mutex mutex;
  try {
    for (YAML::const_iterator it = conf.begin(); it != conf.end(); ++it) {
      ta_info_t ta_info;
      PlmnId plmn_id;
      const YAML::Node& taInfo = *it;

      // Set Tai
      plmn_id.setMcc(taInfo["tai"]["plmnId"]["mcc"].as<string>());
      plmn_id.setMnc(taInfo["tai"]["plmnId"]["mnc"].as<string>());
      ta_info.tai.setPlmnId(plmn_id);
      ta_info.tai.setTac(taInfo["tai"]["tac"].as<string>());

      // Set Supported Snssai List
      cfg.ta_info_list.push_back(ta_info);
    }
    cfg.ta_info_list.shrink_to_fit();
  } catch (std::exception& e) {
    Logger::nssf_app().error("Eror parsing TaInfo");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
const bool nssf_config::parse_nssai(
    const YAML::Node& conf, SupportedNssaiAvailabilityData& nssai_data) {
  try {
    Tai tai;
    PlmnId plmn_id;
    std::vector<ExtSnssai> snssai_list;
    std::vector<Tai> tai_list;

    // Parse TAI
    tai.setTac(conf["tai"]["tac"].as<string>());
    plmn_id.setMcc(conf["tai"]["plmnId"]["mcc"].as<string>());
    plmn_id.setMcc(conf["tai"]["plmnId"]["mcc"].as<string>());
    tai.setPlmnId(plmn_id);
    nssai_data.setTai(tai);

    // Parse Supported Snssai List
    const YAML::Node& slices = conf["supportedSnssaiList"];
    for (YAML::const_iterator it = slices.begin(); it != slices.end(); ++it) {
      const YAML::Node& snssai = *it;
      ExtSnssai e_snssai;
      e_snssai.setSst(snssai["sst"].as<uint32_t>());
      if (snssai["sd"]) e_snssai.setSd(snssai["sd"].as<string>());
      snssai_list.push_back(e_snssai);
    }
    nssai_data.setSupportedSnssaiList(snssai_list);

  } catch (std::exception& e) {
    Logger::nssf_app().error("Eror parsing amfList");
    return false;
  }
  return true;
}
//------------------------------------------------------------------------------
const bool nssf_config::parse_amf_list(
    const YAML::Node& conf, amf_info_t& amf_info) {
  static std::mutex mutex;
  try {
    for (YAML::const_iterator ita = conf.begin(); ita != conf.end(); ++ita) {
      const YAML::Node& amf = *ita;
      std::vector<SupportedNssaiAvailabilityData> nssai_data_list;
      SupportedNssaiAvailabilityData nssai_data;
      // ToDo:- Parse as a list
      parse_nssai(amf["supportedNssaiAvailabilityData"], nssai_data);
      nssai_data_list.push_back(nssai_data);
      amf_info.amf_List.emplace_back(amf["nfId"].as<string>(), nssai_data_list);
    }
  } catch (std::exception& e) {
    Logger::nssf_app().error("Eror parsing amfList");
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
const bool nssf_config::parse_amf_info(
    const YAML::Node& conf, nssf_amf_info_t& cfg) {
  static std::mutex mutex;
  try {
    for (YAML::const_iterator it = conf.begin(); it != conf.end(); ++it) {
      amf_info_t amf_info;
      const YAML::Node& amfInfo = *it;

      // amInfoList
      amf_info.target_amf_set  = amfInfo["targetAmfSet"].as<string>();
      amf_info.nrf_amf_set     = amfInfo["nrfAmfSet"].as<string>();
      amf_info.nrf_amf_set_mgt = amfInfo["nrfAmfSetNfMgtUri"].as<string>();
      if (!parse_amf_list(amfInfo["amfList"], amf_info)) throw std::exception();
      cfg.amf_info_list.push_back(amf_info);
    }
  } catch (std::exception& e) {
    Logger::nssf_app().error("Eror parsing amfInfo");
    return false;
  }
  return true;
}
//------------------------------------------------------------------------------
bool nssf_config::parse_config() {
  YAML::Node config = {};
  try {
    config = YAML::LoadFile(slice_config_file.c_str());
  } catch (nlohmann::detail::exception& e) {
    std::cout << "The slice config file specified does not exists" << std::endl;
    return false;
  }

  // Parse nsi_info_list
  if (config["configuration"]["nsiInfoList"]) {
    parse_nsi_info(config["configuration"]["nsiInfoList"], nssf_nsi_info);
  } else {
    Logger::nssf_app().error("Error parsing section : nsiInfoList");
    return false;
  }

  // Parse ta_info_list
  if (config["configuration"]["taInfoList"]) {
    parse_ta_info(config["configuration"]["taInfoList"], nssf_ta_info);
  } else {
    Logger::nssf_app().error("Error parsing section : taInfoList");
    return false;
  }

  // Parse amf_info_list
  if (config["configuration"]["amfInfoList"]) {
    parse_amf_info(config["configuration"]["amfInfoList"], nssf_amf_info);
  } else {
    Logger::nssf_app().error("Error parsing section : amfInfoList");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// bool nssf_config::get_slice_config(nlohmann::json& slice_config) {
//   slice_config = nssf_slice_config;
//   return true;
// }

//------------------------------------------------------------------------------
bool nssf_config::get_api_list(nlohmann::json& api_list) {
  api_list["OAI-NSSF"] = {
      {"Organisation", "Openairinterface Software Aliance"},
      {"Description", "OAI-NSSF initial Release"},
      {"Version", "1.0.0"},
      {"Supported APIs",
       {{"API", "Network Slice Information (Document)"},
        {"Method", "GET"},
        {"URI Path",
         "/nnssf-nsselection/<api_version>/network-slice-information"},
        {"Details",
         "Retrieve the Network Slice Selection Information (PDU Session)"}}}};
  return true;
}
//------------------------------------------------------------------------------
