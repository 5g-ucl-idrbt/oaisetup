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

/*! \file udm_config.cpp
 \brief
 \author  Hongxin WANG, BUPT
 \date 2021
 \email: contact@openairinterface.org
 */

#include "udm_config.hpp"

#include <iostream>
#include <libconfig.h++>

#include "if.hpp"
#include "logger.hpp"
#include "fqdn.hpp"
#include "udm.h"
#include "string.hpp"

extern "C" {
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "common_defs.h"
}

using namespace libconfig;

namespace oai::udm::config {

//------------------------------------------------------------------------------
udm_config::udm_config() : instance(0), pid_dir(), udm_name(), sbi() {
  udr_addr.ipv4_addr.s_addr = INADDR_ANY;
  udr_addr.port             = 80;
  udr_addr.api_version      = "v1";
  udr_addr.fqdn             = {};
  nrf_addr.ipv4_addr.s_addr = INADDR_ANY;
  nrf_addr.port             = 80;
  nrf_addr.api_version      = "v1";
  nrf_addr.fqdn             = {};
  use_fqdn_dns              = false;
  use_http2                 = false;
  register_nrf              = false;
}

//------------------------------------------------------------------------------
udm_config::~udm_config() {}

//------------------------------------------------------------------------------
int udm_config::load(const std::string& config_file) {
  Logger::config().debug(
      "\nLoad UDM system configuration file(%s)", config_file.c_str());
  Config cfg;
  unsigned char buf_in6_addr[sizeof(struct in6_addr)];

  try {
    cfg.readFile(config_file.c_str());
  } catch (const FileIOException& fioex) {
    Logger::config().error(
        "I/O error while reading file %s - %s", config_file.c_str(),
        fioex.what());
    throw;
  } catch (const ParseException& pex) {
    Logger::config().error(
        "Parse error at %s:%d - %s", pex.getFile(), pex.getLine(),
        pex.getError());
    throw;
  }
  const Setting& root = cfg.getRoot();

  try {
    const Setting& udm_cfg = root[UDM_CONFIG_STRING_UDM_CONFIG];
  } catch (const SettingNotFoundException& nfex) {
    Logger::config().error("%s : %s", nfex.what(), nfex.getPath());
    return RETURNerror;
  }
  const Setting& udm_cfg = root[UDM_CONFIG_STRING_UDM_CONFIG];
  try {
    udm_cfg.lookupValue(UDM_CONFIG_STRING_INSTANCE_ID, instance);
  } catch (const SettingNotFoundException& nfex) {
    Logger::config().error(
        "%s : %s, using defaults", nfex.what(), nfex.getPath());
  }

  try {
    udm_cfg.lookupValue(UDM_CONFIG_STRING_PID_DIRECTORY, pid_dir);
  } catch (const SettingNotFoundException& nfex) {
    Logger::config().error(
        "%s : %s, using defaults", nfex.what(), nfex.getPath());
  }
  try {
    udm_cfg.lookupValue(UDM_CONFIG_STRING_UDM_NAME, udm_name);
  } catch (const SettingNotFoundException& nfex) {
    Logger::config().error(
        "%s : %s, using defaults", nfex.what(), nfex.getPath());
  }

  try {
    const Setting& new_if_cfg = udm_cfg[UDM_CONFIG_STRING_INTERFACES];

    const Setting& sbi_udm_cfg =
        new_if_cfg[UDM_CONFIG_STRING_INTERFACE_SBI_UDM];
    load_interface(sbi_udm_cfg, sbi);

    // HTTP2 port
    if (!(sbi_udm_cfg.lookupValue(
            UDM_CONFIG_STRING_SBI_HTTP2_PORT, sbi_http2_port))) {
      Logger::config().error(UDM_CONFIG_STRING_SBI_HTTP2_PORT "failed");
      throw(UDM_CONFIG_STRING_SBI_HTTP2_PORT " failed");
    }
  } catch (const SettingNotFoundException& nfex) {
    Logger::config().error(
        "%s : %s, using defaults", nfex.what(), nfex.getPath());
    return RETURNerror;
  }

  // Support features
  try {
    const Setting& support_features =
        udm_cfg[UDM_CONFIG_STRING_SUPPORT_FEATURES];
    std::string opt = {};

    support_features.lookupValue(
        UDM_CONFIG_STRING_SUPPORT_FEATURES_USE_FQDN_DNS, opt);
    if (boost::iequals(opt, "yes")) {
      use_fqdn_dns = true;
    } else {
      use_fqdn_dns = false;
    }

    support_features.lookupValue(
        UDM_CONFIG_STRING_SUPPORTED_FEATURES_REGISTER_NRF, opt);
    if (boost::iequals(opt, "yes")) {
      register_nrf = true;
    } else {
      register_nrf = false;
    }

    support_features.lookupValue(
        UDM_CONFIG_STRING_SUPPORT_FEATURES_USE_HTTP2, opt);
    if (boost::iequals(opt, "yes")) {
      use_http2 = true;
    } else {
      use_http2 = false;
    }
  } catch (const SettingNotFoundException& nfex) {
    Logger::udm_app().error(
        "%s : %s, using defaults", nfex.what(), nfex.getPath());
    return RETURNerror;
  }

  try {
    std::string astring;

    // UDR
    const Setting& udr_cfg       = udm_cfg[UDM_CONFIG_STRING_UDR];
    struct in_addr udr_ipv4_addr = {};
    unsigned int udr_port        = 0;
    std::string udr_api_version  = {};

    if (!use_fqdn_dns) {
      udr_cfg.lookupValue(UDM_CONFIG_STRING_UDR_IPV4_ADDRESS, astring);
      IPV4_STR_ADDR_TO_INADDR(
          util::trim(astring).c_str(), udr_ipv4_addr,
          "BAD IPv4 ADDRESS FORMAT FOR UDR !");
      udr_addr.ipv4_addr = udr_ipv4_addr;
      if (!(udr_cfg.lookupValue(UDM_CONFIG_STRING_UDR_PORT, udr_port))) {
        Logger::udm_app().error(UDM_CONFIG_STRING_UDR_PORT "failed");
        throw(UDM_CONFIG_STRING_UDR_PORT "failed");
      }
      udr_addr.port = udr_port;

      if (!(udr_cfg.lookupValue(
              UDM_CONFIG_STRING_API_VERSION, udr_api_version))) {
        Logger::udm_app().error(UDM_CONFIG_STRING_API_VERSION "failed");
        throw(UDM_CONFIG_STRING_API_VERSION "failed");
      }
      udr_addr.api_version = udr_api_version;
    } else {
      udr_cfg.lookupValue(UDM_CONFIG_STRING_FQDN_DNS, astring);
      uint8_t addr_type   = {0};
      std::string address = {};
      fqdn::resolve(astring, address, udr_port, addr_type);
      if (addr_type != 0) {  // IPv6
        // TODO:
        throw("DO NOT SUPPORT IPV6 ADDR FOR UDR!");
      } else {  // IPv4
        IPV4_STR_ADDR_TO_INADDR(
            util::trim(address).c_str(), udr_ipv4_addr,
            "BAD IPv4 ADDRESS FORMAT FOR UDR !");
        udr_addr.ipv4_addr = udr_ipv4_addr;
        // We hardcode udr port from config for the moment
        if (!(udr_cfg.lookupValue(UDM_CONFIG_STRING_UDR_PORT, udr_port))) {
          Logger::udm_app().error(UDM_CONFIG_STRING_UDR_PORT "failed");
          throw(UDM_CONFIG_STRING_UDR_PORT "failed");
        }
        udr_addr.port               = udr_port;
        std::string udr_api_version = {};
        if (!(udr_cfg.lookupValue(
                UDM_CONFIG_STRING_API_VERSION, udr_api_version))) {
          Logger::udm_app().error(UDM_CONFIG_STRING_API_VERSION "failed");
          throw(UDM_CONFIG_STRING_API_VERSION "failed");
        }
        udr_addr.api_version =
            udr_api_version;  // TODO: to get API version from DNS
        udr_addr.fqdn = astring;
      }
    }

  } catch (const SettingNotFoundException& nfex) {
    Logger::udm_app().error("%s : %s", nfex.what(), nfex.getPath());
    return RETURNerror;
  }

  // NRF
  if (register_nrf) {
    try {
      std::string astring = {};

      const Setting& nrf_cfg       = udm_cfg[UDM_CONFIG_STRING_NRF];
      struct in_addr nrf_ipv4_addr = {};
      unsigned int nrf_port        = 0;
      std::string nrf_api_version  = {};

      if (!use_fqdn_dns) {
        nrf_cfg.lookupValue(UDM_CONFIG_STRING_NRF_IPV4_ADDRESS, astring);
        IPV4_STR_ADDR_TO_INADDR(
            util::trim(astring).c_str(), nrf_ipv4_addr,
            "BAD IPv4 ADDRESS FORMAT FOR NRF !");
        nrf_addr.ipv4_addr = nrf_ipv4_addr;
        if (!(nrf_cfg.lookupValue(UDM_CONFIG_STRING_NRF_PORT, nrf_port))) {
          Logger::udm_app().error(UDM_CONFIG_STRING_NRF_PORT "failed");
          throw(UDM_CONFIG_STRING_NRF_PORT "failed");
        }
        nrf_addr.port = nrf_port;
        if (!(nrf_cfg.lookupValue(
                UDM_CONFIG_STRING_API_VERSION, nrf_api_version))) {
          Logger::udm_app().error(UDM_CONFIG_STRING_API_VERSION "failed");
          throw(UDM_CONFIG_STRING_API_VERSION "failed");
        }
        nrf_addr.api_version = nrf_api_version;
      } else {
        nrf_cfg.lookupValue(UDM_CONFIG_STRING_FQDN_DNS, astring);
        uint8_t addr_type   = {0};
        std::string address = {};
        fqdn::resolve(astring, address, nrf_port, addr_type);
        if (addr_type != 0) {  // IPv6
          // TODO:
          throw("DO NOT SUPPORT IPV6 ADDR FOR NRF!");
        } else {  // IPv4
          IPV4_STR_ADDR_TO_INADDR(
              util::trim(address).c_str(), nrf_ipv4_addr,
              "BAD IPv4 ADDRESS FORMAT FOR NRF !");
          nrf_addr.ipv4_addr = nrf_ipv4_addr;
          // We hardcode nrf port from config for the moment
          if (!(nrf_cfg.lookupValue(UDM_CONFIG_STRING_NRF_PORT, nrf_port))) {
            Logger::udm_app().error(UDM_CONFIG_STRING_NRF_PORT "failed");
            throw(UDM_CONFIG_STRING_NRF_PORT "failed");
          }
          nrf_addr.port        = nrf_port;
          nrf_addr.api_version = "v1";  // TODO: to get API version from DNS
          nrf_addr.fqdn        = astring;
        }
      }
    } catch (const SettingNotFoundException& nfex) {
      Logger::udm_app().error("%s : %s", nfex.what(), nfex.getPath());
      return RETURNerror;
    }
  }

  return RETURNok;
}

//------------------------------------------------------------------------------
void udm_config::display() {
  Logger::config().info(
      "==== OAI-CN5G %s v%s ====", PACKAGE_NAME, PACKAGE_VERSION);
  Logger::config().info("============== UDM =============");
  Logger::config().info("Configuration UDM:");
  Logger::config().info("- Instance................: %d", instance);
  Logger::config().info("- PID Dir.................: %s", pid_dir.c_str());
  Logger::config().info("- UDM name................: %s", udm_name.c_str());

  Logger::config().info("- SBI:");
  Logger::config().info("    Iface name............: %s", sbi.if_name.c_str());
  Logger::config().info("    IPv4 Addr.............: %s", inet_ntoa(sbi.addr4));
  Logger::config().info("    HTTP1 Port ...........: %d", sbi.port);
  Logger::config().info("    HTTP2 Port............: %d", sbi_http2_port);
  Logger::config().info(
      "    Api Version...........: %s", sbi.api_version.c_str());
  Logger::config().info("- Supported Features:");
  Logger::config().info(
      "    REGISTER NRF ..  ......: %s", register_nrf ? "Yes" : "No");
  Logger::config().info(
      "    Use FQDN ..............: %s", use_fqdn_dns ? "Yes" : "No");
  Logger::config().info(
      "    Use HTTP2..............: %s", use_http2 ? "Yes" : "No");

  Logger::config().info("- UDR:");
  Logger::config().info(
      "    IPv4 Addr.............: %s",
      inet_ntoa(*((struct in_addr*) &udr_addr.ipv4_addr)));
  Logger::config().info("    Port..................: %lu  ", udr_addr.port);
  Logger::config().info(
      "    API version...........: %s", udr_addr.api_version.c_str());
  Logger::config().info("- NRF:");
  Logger::config().info(
      "    IPv4 Addr ............: %s",
      inet_ntoa(*((struct in_addr*) &nrf_addr.ipv4_addr)));
  Logger::config().info("    Port .................: %lu  ", nrf_addr.port);
  Logger::config().info(
      "    API version ..........: %s", nrf_addr.api_version.c_str());
  if (use_fqdn_dns)
    Logger::config().info(
        "    FQDN..................: %s", nrf_addr.fqdn.c_str());
}

//------------------------------------------------------------------------------
int udm_config::load_interface(
    const libconfig::Setting& if_cfg, interface_cfg_t& cfg) {
  if_cfg.lookupValue(UDM_CONFIG_STRING_INTERFACE_NAME, cfg.if_name);
  util::trim(cfg.if_name);
  if (not boost::iequals(cfg.if_name, "none")) {
    std::string address = {};
    if_cfg.lookupValue(UDM_CONFIG_STRING_IPV4_ADDRESS, address);
    util::trim(address);
    if (boost::iequals(address, "read")) {
      if (get_inet_addr_infos_from_iface(
              cfg.if_name, cfg.addr4, cfg.network4, cfg.mtu)) {
        Logger::config().error(
            "Could not read %s network interface configuration", cfg.if_name);
        return RETURNerror;
      }
    } else {
      std::vector<std::string> words;
      boost::split(
          words, address, boost::is_any_of("/"), boost::token_compress_on);
      if (words.size() != 2) {
        Logger::config().error(
            "Bad value " UDM_CONFIG_STRING_IPV4_ADDRESS " = %s in config file",
            address.c_str());
        return RETURNerror;
      }
      unsigned char buf_in_addr[sizeof(struct in6_addr)];  // you never know...
      if (inet_pton(AF_INET, util::trim(words.at(0)).c_str(), buf_in_addr) ==
          1) {
        memcpy(&cfg.addr4, buf_in_addr, sizeof(struct in_addr));
      } else {
        Logger::config().error(
            "In conversion: Bad value " UDM_CONFIG_STRING_IPV4_ADDRESS
            " = %s in config file",
            util::trim(words.at(0)).c_str());
        return RETURNerror;
      }
      cfg.network4.s_addr = htons(
          ntohs(cfg.addr4.s_addr) &
          0xFFFFFFFF << (32 - std::stoi(util::trim(words.at(1)))));
    }
    if_cfg.lookupValue(UDM_CONFIG_STRING_PORT, cfg.port);

    // SBI API VERSION
    if (!(if_cfg.lookupValue(UDM_CONFIG_STRING_API_VERSION, cfg.api_version))) {
      Logger::config().error(UDM_CONFIG_STRING_API_VERSION "failed");
      throw(UDM_CONFIG_STRING_API_VERSION "failed");
    }
  }
  return RETURNok;
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_slice_selection_subscription_data_retrieval_uri(
    const std::string& supi, const oai::udm::model::PlmnId& plmn_id) {
  return get_udr_url_base() + "/subscription-data/" + supi + "/" +
         plmn_id.getMcc() + plmn_id.getMnc() + "/provisioned-data/am-data";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_access_and_mobility_subscription_data_uri(
    const std::string& supi, const oai::udm::model::PlmnId& plmn_id) {
  return get_udr_url_base() + "/subscription-data/" + supi + "/" +
         plmn_id.getMcc() + plmn_id.getMnc() + "/provisioned-data/am-data";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_session_management_subscription_data_uri(
    const std::string& supi, const oai::udm::model::PlmnId& plmn_id) {
  return get_udr_url_base() + "/subscription-data/" + supi + "/" +
         plmn_id.getMcc() + plmn_id.getMnc() + "/provisioned-data/sm-data";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_smf_selection_subscription_data_uri(
    const std::string& supi, const oai::udm::model::PlmnId& plmn_id) {
  return get_udr_url_base() + "/subscription-data/" + supi + "/" +
         plmn_id.getMcc() + plmn_id.getMnc() +
         "/provisioned-data/smf-selection-subscription-data";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_sdm_subscriptions_uri(const std::string& supi) {
  return get_udr_url_base() + "/subscription-data/" + supi +
         "/context-data/sdm-subscriptions";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_authentication_subscription_uri(
    const std::string& supi) {
  return get_udr_url_base() + "/subscription-data/" + supi +
         NUDR_AUTHENTICATION_SUBSCRIPTION_ENDPOINT;
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_authentication_status_uri(
    const std::string& supi) {
  return get_udr_url_base() + "/subscription-data/" + supi +
         "/authentication-data/authentication-status";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_amf_3gpp_registration_uri(
    const std::string& supi) {
  return get_udr_url_base() + "/subscription-data/" + supi +
         "/context-data/amf-3gpp-access";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udm_ueau_base() {
  return std::string(inet_ntoa(*((struct in_addr*) &udr_addr.ipv4_addr))) +
         ":" + std::to_string(udr_addr.port) + NUDM_UE_AU_BASE +
         udr_addr.api_version;
}
//------------------------------------------------------------------------------
std::string udm_config::get_udr_url_base() {
  return std::string(inet_ntoa(*((struct in_addr*) &udr_addr.ipv4_addr))) +
         ":" + std::to_string(udr_addr.port) + NUDR_DATA_REPOSITORY +
         udr_addr.api_version;
}

}  // namespace oai::udm::config
