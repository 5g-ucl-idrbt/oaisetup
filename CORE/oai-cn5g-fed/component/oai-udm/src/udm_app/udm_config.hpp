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

/*! \file udm_config.hpp
 \brief
 \author  Hongxin WANG, BUPT
 \date 2021
 \email: contact@openairinterface.org
 */

#ifndef _UDM_CONFIG_H_
#define _UDM_CONFIG_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <libconfig.h++>
#include <mutex>
#include <string>
#include <vector>

#include "udm_config.hpp"
#include "PlmnId.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#define UDM_CONFIG_STRING_UDM_CONFIG "UDM"
#define UDM_CONFIG_STRING_PID_DIRECTORY "PID_DIRECTORY"
#define UDM_CONFIG_STRING_INSTANCE_ID "INSTANCE_ID"
#define UDM_CONFIG_STRING_UDM_NAME "UDM_NAME"

#define UDM_CONFIG_STRING_INTERFACES "INTERFACES"
#define UDM_CONFIG_STRING_INTERFACE_SBI_UDM "SBI"
#define UDM_CONFIG_STRING_SBI_HTTP2_PORT "HTTP2_PORT"
#define UDM_CONFIG_STRING_INTERFACE_NAME "INTERFACE_NAME"
#define UDM_CONFIG_STRING_IPV4_ADDRESS "IPV4_ADDRESS"
#define UDM_CONFIG_STRING_PORT "PORT"
#define UDM_CONFIG_STRING_PPID "PPID"
#define UDM_CONFIG_STRING_API_VERSION "API_VERSION"

#define UDM_CONFIG_STRING_UDR "UDR"
#define UDM_CONFIG_STRING_UDR_IPV4_ADDRESS "IPV4_ADDRESS"
#define UDM_CONFIG_STRING_UDR_PORT "PORT"

#define UDM_CONFIG_STRING_NRF "NRF"
#define UDM_CONFIG_STRING_NRF_IPV4_ADDRESS "IPV4_ADDRESS"
#define UDM_CONFIG_STRING_NRF_PORT "PORT"

#define UDM_CONFIG_STRING_SUPPORT_FEATURES "SUPPORT_FEATURES"
#define UDM_CONFIG_STRING_SUPPORT_FEATURES_USE_FQDN_DNS "USE_FQDN_DNS"
#define UDM_CONFIG_STRING_SUPPORTED_FEATURES_REGISTER_NRF "REGISTER_NRF"
#define UDM_CONFIG_STRING_SUPPORT_FEATURES_USE_HTTP2 "USE_HTTP2"
#define UDM_CONFIG_STRING_FQDN_DNS "FQDN"

using namespace libconfig;

namespace oai::udm::config {

typedef struct interface_cfg_s {
  std::string if_name;
  struct in_addr addr4;
  struct in_addr network4;
  struct in6_addr addr6;
  unsigned int mtu;
  unsigned int port;
  std::string api_version;
} interface_cfg_t;

typedef struct nf_addr_s {
  struct in_addr ipv4_addr;
  unsigned int port;
  std::string api_version;
  std::string fqdn;
} nf_addr_t;

class udm_config {
 public:
  udm_config();
  ~udm_config();
  int load(const std::string& config_file);
  int load_interface(const Setting& if_cfg, interface_cfg_t& cfg);
  void display();
  std::string get_udr_slice_selection_subscription_data_retrieval_uri(
      const std::string& supi, const oai::udm::model::PlmnId& plmn_id);
  std::string get_udr_access_and_mobility_subscription_data_uri(
      const std::string& supi, const oai::udm::model::PlmnId& plmn_id);
  std::string get_udr_session_management_subscription_data_uri(
      const std::string& supi, const oai::udm::model::PlmnId& plmn_id);
  std::string get_udr_smf_selection_subscription_data_uri(
      const std::string& supi, const oai::udm::model::PlmnId& plmn_id);
  std::string get_udr_url_base();
  std::string get_udr_sdm_subscriptions_uri(const std::string& supi);

  std::string get_udr_authentication_subscription_uri(const std::string& supi);
  std::string get_udr_authentication_status_uri(const std::string& supi);
  std::string get_udr_amf_3gpp_registration_uri(const std::string& supi);
  std::string get_udm_ueau_base();

  unsigned int instance;
  std::string pid_dir;
  std::string udm_name;

  interface_cfg_t sbi;
  unsigned int sbi_http2_port;

  nf_addr_t udr_addr;
  nf_addr_t nrf_addr;

  bool register_nrf;
  bool use_fqdn_dns;
  bool use_http2;
};

}  // namespace oai::udm::config

#endif
