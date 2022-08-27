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

#ifndef FILE_UDR_SEEN
#define FILE_UDR_SEEN

#define HEART_BEAT_TIMER 10

#define _unused(x) ((void) (x))

#define NNRF_NFM_BASE "/nnrf-nfm/"
#define UDR_NF_REGISTER_URL "/nf-instances/"

#define NUDR_DR_BASE "/nudr-dr/"
#define NUDR_DR_AUTH_STATUS "authentication-status"
#define NUDR_DR_AUTH_SUBS "authentication-subscription"
#define NUDR_DR_AMF_XGPP_ACCESS "amf-3gpp-access"
#define NUDR_DR_AM_DATA "am-data"
#define NUDR_DR_SDM_SUBS "sdm-subscriptions"
#define NUDR_DR_SM_DATA "sm-data"
#define NUDR_DR_SMF_REG "smf-registrations"
#define NUDR_DR_SMF_SELECT "smf-selection-subscription-data"

#define NF_CURL_TIMEOUT_MS 1000L

typedef struct identity_range_s {
  std::string start;
  std::string end;
  std::string pattern;
} identity_range_t;

typedef struct identity_range_udr_info_item_s {
  identity_range_t identity_range;
} identity_range_udr_info_item_t;

typedef struct supi_range_s {
  std::string start;
  std::string end;
  std::string pattern;
} supi_range_t;

typedef struct supi_range_udr_info_item_s {
  supi_range_t supi_range;
} supi_range_udr_info_item_t;

typedef struct udr_info_s {
  std::string groupid;
  std::vector<supi_range_udr_info_item_t> supi_ranges;
  std::vector<identity_range_udr_info_item_t> gpsi_ranges;
  std::vector<identity_range_udr_info_item_t> ext_grp_id_ranges;
  std::vector<std::string> data_set_id;
} udr_info_t;

#endif
