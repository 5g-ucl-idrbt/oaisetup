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

/*! \file udm_client.hpp
 \author  Tien-Thinh NGUYEN
 \company Eurecom
 \date 2020
 \email:
 */

#ifndef FILE_UDM_NRF_SEEN
#define FILE_UDM_NRF_SEEN

#include <map>
#include <thread>

#include <curl/curl.h>

#include "logger.hpp"
#include "udm_config.hpp"
#include "udm_event.hpp"
#include "udm_profile.hpp"

namespace oai {
namespace udm {
namespace app {

class udm_nrf {
 private:
 public:
  udm_profile udm_nf_profile;   // UDM profile
  std::string udm_instance_id;  // UDM instance id
  // timer_id_t timer_udm_heartbeat;

  udm_nrf(udm_event& ev);
  udm_nrf(udm_nrf const&) = delete;
  void operator=(udm_nrf const&) = delete;

  void generate_uuid();
  /*
   * Start event nf heartbeat procedure
   * @param [void]
   * @return void
   */
  void start_event_nf_heartbeat(std::string& remoteURI);
  /*
   * Trigger NF heartbeat procedure
   * @param [void]
   * @return void
   */
  void trigger_nf_heartbeat_procedure(uint64_t ms);
  /*
   * Generate a UDM profile for this instance
   * @param [void]
   * @return void
   */
  void generate_udm_profile(
      udm_profile& udm_nf_profile, std::string& udm_instance_id);

  /*
   * Trigger NF instance registration to NRF
   * @param [void]
   * @return void
   */
  void register_to_nrf();
  /*
   * Get udm API Root
   * @param [std::string& ] api_root: udm's API Root
   * @return void
   */
  void get_udm_api_root(std::string& api_root);

 private:
  udm_event& m_event_sub;
  bs2::connection task_connection;
};
}  // namespace app
}  // namespace udm
}  // namespace oai
#endif /* FILE_UDM_NRF_SEEN */
