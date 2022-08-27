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

/*! \file amf_n1.hpp
 \brief
 \author Keliang DU (BUPT), Tien-Thinh NGUYEN (EURECOM)
 \date 2020
 \email: contact@openairinterface.org
 */

#ifndef _AMF_N1_H_
#define _AMF_N1_H_

#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <shared_mutex>

#include "3gpp_ts24501.hpp"
#include "3gpp_29.503.h"
#include "amf.hpp"
#include "amf_statistics.hpp"
#include "bstrlib.h"
#include "itti_msg_n1.hpp"
#include "mysql_db.hpp"
#include "nas_context.hpp"
#include "pdu_session_context.hpp"
#include "amf_event.hpp"
#include "RegistrationAccept.hpp"
#include "ue_context.hpp"
#include "itti.hpp"
#include "SliceInfoForRegistration.h"
#include "AuthorizedNetworkSliceInfo.h"
#include "Nssai.h"

namespace amf_application {

class amf_n1 {
 public:
  amf_n1();
  ~amf_n1();

  /*
   * Handle ITTI message
   * @param [itti_uplink_nas_data_ind&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_uplink_nas_data_ind&);

  /*
   * Handle ITTI message
   * @param [itti_downlink_nas_transfer&]: ITTI message
   * @return void
   */
  void handle_itti_message(itti_downlink_nas_transfer& itti_msg);

  /*
   * Handle NAS Establishment Request (Registration Request, Service Request)
   * @param [SecurityHeaderType_t] type: Security Header Type
   * @param [std::shared_ptr<nas_context>] nc: Shared pointer to the NAS context
   * @param [uint32_t] ran_ue_ngap_id: RAN UE NGAP Id
   * @param [long] amf_ue_ngap_id: AMF UE NGAP Id
   * @param [bstring] plain_msg: NAS message in plain text
   * @param [std::string] snn: Serving Network
   * @param [uint8_t] ulCount: UL Sequence number
   * @return void
   */
  void nas_signalling_establishment_request_handle(
      SecurityHeaderType_t type, std::shared_ptr<nas_context> nc,
      uint32_t ran_ue_ngap_id, long amf_ue_ngap_id, bstring plain_msg,
      std::string snn, uint8_t ulCount);

  /*
   * Handle UL NAS message (Authentication Response, Security Mode Complete,
   * etc)
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP Id
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP Id
   * @param [bstring] plain_msg: NAS message in plain text
   * @param [const plmn_t&] plmn: PLMN
   * @return void
   */
  void uplink_nas_msg_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
      bstring plain_msg, const plmn_t& plmn);

  /*
   * Get Security Header Type (2 bytes) from NAS message
   * @param [SecurityHeaderType_t&] type: Security Header Type
   * @param [uint8_t*] buffer: NAS message
   * @param [uint32_t] length: Length of NAS message
   * @return true if can decode with an appropriate Security Header Type,
   * otherwise return false
   */
  bool check_security_header_type(
      SecurityHeaderType_t& type, const uint8_t* buffer, const uint32_t length);

  /*
   * Verify if a UE NAS context associated with a GUTI exist
   * @param [const std::string&] guti: UE GUTI
   * @return true if UE NAS context exist, otherwise false
   */
  bool is_guti_2_nas_context(const std::string& guti) const;

  /*
   * Get UE NAS context associated with a GUTI
   * @param [const std::string&] guti: UE GUTI
   * @return shared pointer to the UE NAS context
   */
  std::shared_ptr<nas_context> guti_2_nas_context(
      const std::string& guti) const;

  /*
   * Store an UE NAS context associated with a GUTI
   * @param [const std::string&] guti: UE GUTI
   * @param [const std::shared_ptr<nas_context>&] nc: pointer to UE NAS context
   * @return void
   */
  void set_guti_2_nas_context(
      const std::string& guti, const std::shared_ptr<nas_context>& nc);

  /*
   * Remove UE NAS context associated with a GUTI
   * @param [const std::string&] guti: UE GUTI
   * @return true if successful, otherwise return false
   */
  bool remove_guti_2_nas_context(const std::string& guti);

  /*
   * Verify if a UE NAS context associated with an AMF UE NGAP ID exist
   * @param [const long& ] amf_ue_ngap_id: AMF UE NGAP ID
   * @return true if UE NAS context exist, otherwise false
   */
  bool is_amf_ue_id_2_nas_context(const long& amf_ue_ngap_id) const;

  /*
   * Get UE NAS context associated with an AMF UE NGAP ID
   * @param [const long& ] amf_ue_ngap_id: AMF UE NGAP ID
   * @return shared pointer to the UE NAS context
   */
  std::shared_ptr<nas_context> amf_ue_id_2_nas_context(
      const long& amf_ue_ngap_id) const;

  /*
   * Store an UE NAS context associated with an AMF UE NGAP ID
   * @param [const long& ] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [const std::shared_ptr<nas_context>&] nc: pointer to UE NAS context
   * @return void
   */
  void set_amf_ue_ngap_id_2_nas_context(
      const long& amf_ue_ngap_id, std::shared_ptr<nas_context> nc);

  /*
   * Remove UE NAS context associated with an AMF UE NGAP ID
   * @param [const long& ] amf_ue_ngap_id: AMF UE NGAP ID
   * @return true if successful, otherwise return false
   */
  bool remove_amf_ue_ngap_id_2_nas_context(const long& amf_ue_ngap_id);

  /*
   * Store the mapping between SUPI and AMF UE NGAP ID
   * @param [const std::string&] SUPI: UE SUPI
   *@param [const long& ] amf_ue_ngap_id: AMF UE NGAP ID
   * @return void
   */
  void set_supi_2_amf_id(const std::string& supi, const long& amf_ue_ngap_id);

  /*
   * Get AMF UE NGAP ID
   * @param [const std::string&] SUPI: UE SUPI
   * @param [long& ] amf_ue_ngap_id: AMF UE NGAP ID
   * @return true if success
   */
  bool supi_2_amf_id(const std::string& supi, long& amf_ue_ngap_id);

  /*
   * Remove AMF UE NGAP ID from the map with SUPI
   * @param [const std::string&] SUPI: UE SUPI
   * @return true if success
   */
  bool remove_supi_2_amf_id(const std::string& supi);

  /*
   * Store the mapping between SUPI and RAN UE NGAP ID
   * @param [const std::string&] SUPI: UE SUPI
   *@param [const uint32_t&] ran_ue_ngap_id: RAN UE NGAP ID
   * @return void
   */
  void set_supi_2_ran_id(
      const std::string& supi, const uint32_t& ran_ue_ngap_id);

  /*
   * Get RAN UE NGAP ID
   * @param [const std::string&] SUPI: UE SUPI
   * @param [uint32_t& ] ran_ue_ngap_id: RAN UE NGAP ID
   * @return true if success
   */
  bool supi_2_ran_id(const std::string& supi, uint32_t& ran_ue_ngap_id);

  /*
   * Remove RAN UE NGAP ID from the map with SUPI
   * @param [const std::string&] SUPI: UE SUPI
   * @return true if success
   */
  bool remove_supi_2_ran_id(const std::string& supi);

  /*
   * Get UE NAS context associated with an IMSI
   * @param [const std::string&] imsi: UE IMSI
   * @return shared pointer to the UE NAS context
   */
  std::shared_ptr<nas_context> imsi_2_nas_context(
      const std::string& imsi) const;

  /*
   * Store an UE NAS context associated with an IMSI
   * @param [const std::string&] imsi: UE IMSI
   * @param [const std::shared_ptr<nas_context>&] nc: pointer to UE NAS context
   * @return void
   */
  void set_imsi_2_nas_context(
      const std::string& imsi, const std::shared_ptr<nas_context>& nc);

  /*
   * Remove UE NAS context associated with an IMSI
   * @param [const std::string&] imsi: UE IMSI
   * @return true if successful, otherwise return false
   */
  bool remove_imsi_2_nas_context(const std::string& imsi);

  /*
   * Perform Registration procedure
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return true if successful, otherwise return false
   */
  void run_registration_procedure(std::shared_ptr<nas_context>& nc);

  /*
   * Perform Mobility Registration procedure
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [uint16_t] uplink_data_status: UL Data Status
   * @param [uint16_t] pdu_session_status: PDU Session Status
   * @return void
   */
  void run_mobility_registration_update_procedure(
      std::shared_ptr<nas_context>& nc, uint16_t uplink_data_status,
      uint16_t pdu_session_status);

  /*
   * Perform Periodic Registration Update procedure
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [uint16_t] pdu_session_status: PDU Session Status
   * @return void
   */
  void run_periodic_registration_update_procedure(
      std::shared_ptr<nas_context>& nc, uint16_t pdu_session_status);

  /*
   * Perform Periodic Registration Update procedure
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [bstring&] nas_msg: NAS message
   * @return void
   */
  void run_periodic_registration_update_procedure(
      std::shared_ptr<nas_context>& nc, bstring& nas_msg);

  /*
   * Generate the Authentication Vectors
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return true if generated successfully, otherwise return false
   */
  bool auth_vectors_generator(std::shared_ptr<nas_context>& nc);

  /*
   * Get the Authentication Vectors from an external AUSF
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return true if generated successfully, otherwise return false
   */
  bool get_authentication_vectors_from_ausf(std::shared_ptr<nas_context>& nc);

  /*
   * Get the 5G AKA Confirmation from an external AUSF
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [bstring] resStar: resStar
   * @return true if generated successfully, otherwise return false
   */
  bool _5g_aka_confirmation_from_ausf(
      std::shared_ptr<nas_context>& nc, bstring resStar);

  /*
   * Generate the Authentication Vectors (locally at AMF)
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return true if generated successfully, otherwise return false
   */
  bool authentication_vectors_generator_in_ausf(
      std::shared_ptr<nas_context>& nc);

  /*
   * Generate the Authentication Vectors (RANDOM, locally at AMF)
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return true if generated successfully, otherwise return false
   */
  bool authentication_vectors_generator_in_udm(
      std::shared_ptr<nas_context>& nc);

  /*
   * Handle the Authentication Vector to setup security context with the UE
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return void
   */
  void handle_auth_vector_successful_result(std::shared_ptr<nas_context>& nc);

  /*
   * Start the Authenticatio procedure
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [int] vindex: vindex
   * @param [uint8_t] ngksi: ngksi
   * @return true if successful, otherwise, false
   */
  bool start_authentication_procedure(
      std::shared_ptr<nas_context>& nc, int vindex, uint8_t ngksi);

  /*
   * Perform the Security Mode Control procedure
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return true if successful, otherwise return false
   */
  bool start_security_mode_control_procedure(std::shared_ptr<nas_context>& nc);

  /*
   * Verify whether a common nas procedure is running
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return true if yes, otherwise return false
   */
  bool check_nas_common_procedure_on_going(std::shared_ptr<nas_context>& nc);

  /*
   * Select the appropriate Security Algorithms for the UE
   * @param [uint8_t] nea: supported NEA (UE)
   * @param [uint8_t] nia: supported NIA (UE)
   * @param [uint8_t&] amf_nea: selected NEA
   * @param [uint8_t&] amf_nia: selected NIA
   * @return true if can find appropriate NEA, NIA
   */
  bool security_select_algorithms(
      uint8_t nea, uint8_t nia, uint8_t& amf_nea, uint8_t& amf_nia);

  /*
   * Encode the NAS message with corresponding integrity and ciphered algorithms
   * @param [nas_secu_ctx*] nsc: NAS Security context
   * @param [bool] is_secu_ctx_new: indicate the status of the security context
   * (new/old)
   * @param [uint8_t] security_header_type: Security Header Type
   * @param [uint8_t] direction: Direction
   * @param [uint8_t*] input_nas_buf: Buffer of the input NAS
   * @param [int] input_nas_les: Length of the buffer
   * @param [bstring&] encrypted_nas: Encrypted NAS (output)
   * @return void
   */
  void encode_nas_message_protected(
      nas_secu_ctx* nsc, bool is_secu_ctx_new, uint8_t security_header_type,
      uint8_t direction, uint8_t* input_nas_buf, int input_nas_len,
      bstring& encrypted_nas);

  /*
   * Encrypt with integrity algorithm
   * @param [nas_secu_ctx*] nsc: NAS Security context
   * @param [uint8_t] direction: Direction
   * @param [uint8_t*] input_nas_buf: Buffer of the input NAS
   * @param [int] input_nas_les: Length of the buffer
   * @param [uint32_t&] mac: calculated MAC (result for NIA)
   * @return true if MAC can be calculated successfully, otherwise return false
   */
  bool nas_message_integrity_protected(
      nas_secu_ctx* nsc, uint8_t direction, uint8_t* input_nas,
      int input_nas_len, uint32_t& mac);

  /*
   * Cipher NAS message with the corresponding ciphered algorithm
   * @param [nas_secu_ctx*] nsc: NAS Security context
   * @param [uint8_t] direction: Direction
   * @param [bstring] input_nas: Input NAS message
   * @param [bstring&] output_nas: Output NAS message
   * @return true if message is successfully ciphered, otherwise return false
   */
  bool nas_message_cipher_protected(
      nas_secu_ctx* nsc, uint8_t direction, bstring input_nas,
      bstring& output_nas);

  // NOTE: All the MySQL-related functions are currently implemented in
  // mysql_db.cpp

  /*
   * Get the Authentication info from the DB (MySQL)
   * @param [const std::string&] imsi: UE IMSI
   * @param [mysql_auth_info_t&] resp: Response from MySQL
   * @return true if retrieve successfully, otherwise return false
   */
  bool get_mysql_auth_info(const std::string& imsi, mysql_auth_info_t& resp);

  /*
   * Update the RAND and SQN to the DB (MySQL)
   * @param [const std::string&] imsi: UE IMSI
   * @param [uint8_t*] rand_p: RAND
   * @param [uint8_t*] sqn: SQN
   * @return void
   */
  void mysql_push_rand_sqn(
      const std::string& imsi, uint8_t* rand_p, uint8_t* sqn);

  /*
   * Increment SQN in the DB for next round (MySQL)
   * @param [const std::string&] imsi: UE IMSI
   * @return void
   */
  void mysql_increment_sqn(const std::string& imsi);

  /*
   * Establish the connection to the DB (MySQL)
   * @return true if successfully, otherwise return false
   */
  bool connect_to_mysql();

  /*
   * Generate a RAND with corresponding length
   * @param [uint8_t*] random_p: RAND
   * @param [ssize_t] length: length of RAND
   * @return void
   */
  void generate_random(uint8_t* random_p, ssize_t length);

  /*
   * Generate 5G HE AV Vector (locally at AMF)
   * @param [uint8_t[16]] opc: OPC
   * @param [const std::string&] imsi: UE IMSI
   * @param [uint8_t[16]] key: Operator Key
   * @param [uint8_t[16]] sqn: SQN
   * @param [std::string&] serving_network: Serving Network
   * @param [_5G_HE_AV_t&] vector: Generated vector
   * @return void
   */
  void generate_5g_he_av_in_udm(
      const uint8_t opc[16], const std::string& imsi, uint8_t key[16],
      uint8_t sqn[6], std::string& serving_network, _5G_HE_AV_t& vector);

  /*
   * Perform annex_a_4_33501 algorithm
   * @param [uint8_t[16]] ck: ck
   * @param [uint8_t[16]] ik: ik
   * @param [uint8_t*] input: input
   * @param [uint8_t[16]] rand: rand
   * @param [std::string&] serving_network: Serving Network
   * @param [uint8_t*] output: output
   * @return void
   */
  void annex_a_4_33501(
      uint8_t ck[16], uint8_t ik[16], uint8_t* input, uint8_t rand[16],
      std::string& serving_network, uint8_t* output);

  /*
   * Get the list of PDU session to be activated from PDU session status
   * @param [uint16_t] pdu_session_status: PDU Session Status
   * @param [std::vector<uint8_t>&] pdu_session_to_be_activated: list of PDU
   * session to be activated
   * @return void
   */
  void get_pdu_session_to_be_activated(
      const uint16_t pdu_session_status,
      std::vector<uint8_t>& pdu_session_to_be_activated);

  /*
   * Initialize Registration Accept with default parameters
   * @param [std::unique_ptr<nas::RegistrationAccept>&] registration_accept:
   * Pointer to the Registration Accept message
   * @return void
   */
  void initialize_registration_accept(
      std::unique_ptr<nas::RegistrationAccept>& registration_accept);

  /*
   * Initialize Registration Accept with the parameters from NAS context
   * @param [std::unique_ptr<nas::RegistrationAccept>&] registration_accept:
   * Pointer to the Registration Accept message
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @return void
   */
  void initialize_registration_accept(
      std::unique_ptr<nas::RegistrationAccept>& registration_accept,
      const std::shared_ptr<nas_context>& nc);

  /*
   * Find the UE Context associated with a NAS Context
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [std::shared_ptr<ue_context>&] uc: Pointer to the UE Context
   * @return true if found, otherwise return false
   */
  bool find_ue_context(
      const std::shared_ptr<nas_context>& nc, std::shared_ptr<ue_context>& uc);

  /*
   * Find the UE Context associated with RAN UE NGAP ID and AMF UE NGAP ID
   * @param [uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [std::shared_ptr<ue_context>&] uc: Pointer to the UE Context
   * @return true if found, otherwise return false
   */
  bool find_ue_context(
      uint32_t ran_ue_ngap_id, long amf_ue_ngap_id,
      std::shared_ptr<ue_context>& uc);

  // Timers handling related functions
  /*
   * Handle the UE Reachable Timer timeout
   * @param [timer_id_t] timer_id: Timer ID
   * @param [uint64_t] amf_ue_ngap_id: AMF UE NGAP ID
   * @return void
   */
  void mobile_reachable_timer_timeout(
      timer_id_t& timer_id, const uint64_t amf_ue_ngap_id);

  /*
   * Set the status of timer timeout in the UE NAS context
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [const bool&] b: whether timer expires or not
   * @return void
   */
  void set_mobile_reachable_timer_timeout(
      std::shared_ptr<nas_context>& nc, const bool& b);

  /*
   * Get the status of timer timeout in the UE NAS context
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [bool&] b: timer timeout status
   * @return void
   */
  void get_mobile_reachable_timer_timeout(
      const std::shared_ptr<nas_context>& nc, bool& b) const;

  /*
   * Get the status of timer timeout in the UE NAS context
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @return bool, true if timer expires, otherwise, return false
   */
  bool get_mobile_reachable_timer_timeout(
      const std::shared_ptr<nas_context>& nc) const;

  /*
   * Store the UE Reachable Timer on the NAS context
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [const timer_id_t&] timer_id: Timer ID
   * @return void
   */
  void set_mobile_reachable_timer(
      std::shared_ptr<nas_context>& nc, const timer_id_t& t);

  /*
   * Store the Implicit Deregistration Timer on the NAS context
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [const timer_id_t&] timer_id: Timer ID
   * @return void
   */
  void set_implicit_deregistration_timer(
      std::shared_ptr<nas_context>& nc, const timer_id_t& t);

  /*
   * Handle the Implicit Deregistration Timer timeout
   * @param [timer_id_t] timer_id: Timer ID
   * @param [uint64_t] amf_ue_ngap_id: AMF UE NGAP ID
   * @return void
   */
  void implicit_deregistration_timer_timeout(
      timer_id_t timer_id, uint64_t amf_ue_ngap_id);

  // NETWORK SLICING RELATED FUNCTIONS

  /*
   * Reroute the Registration Request message to the target AMF if neccessary
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [bool&] reroute_result: true if reroute successfully
   * @return true if current AMF can't handle this request
   */
  bool reroute_registration_request(
      std::shared_ptr<nas_context>& nc, bool& reroute_result);

  /*
   * Get the Slice Selection Subscription Data (from UDM/configuration file)
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [oai::amf::model::Nssai&] nssai: NSSAI
   * @return true if can get NSSAI from UDM/configuration file
   */
  bool get_slice_selection_subscription_data(
      const std::shared_ptr<nas_context>& nc, oai::amf::model::Nssai& nssai);

  /*
   * Get the Slice Selection Subscription Data from the configuration file
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [oai::amf::model::Nssai&] nssai: NSSAI
   * @return true if can get NSSAI from the configuration file
   */
  bool get_slice_selection_subscription_data_from_conf_file(
      const std::shared_ptr<nas_context>& nc, oai::amf::model::Nssai& nssai);

  /*
   * Verify whether the current AMF can process the Subscribed NSSAIs or not
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [oai::amf::model::Nssai&] nssai: NSSAI
   * @return true if AMF can process, otherwise return false
   */
  bool check_subscribed_nssai(
      const std::shared_ptr<nas_context>& nc, oai::amf::model::Nssai& nssai);

  /*
   * Verify whether the current AMF can process the Requested NSSAIs or not
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [oai::amf::model::Nssai&] nssai: NSSAI
   * @return true if AMF can process, otherwise return false
   */
  bool check_requested_nssai(const std::shared_ptr<nas_context>& nc);

  /*
   * Get the Network Selection Data (from NSSF/configuration file)
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [const std::string&] nf_instance_id: NF instance ID
   * @param [const oai::amf::model::SliceInfoForRegistration&] slice_info: Slice
   * information
   * @param [oai::amf::model::AuthorizedNetworkSliceInfo&]
   * authorized_network_slice_info: Authorized NSSAI
   * @return true if can get Network Selection Data from NSSF/configuration file
   */
  bool get_network_slice_selection(
      const std::shared_ptr<nas_context>& nc, const std::string& nf_instance_id,
      const oai::amf::model::SliceInfoForRegistration& slice_info,
      oai::amf::model::AuthorizedNetworkSliceInfo&
          authorized_network_slice_info);

  /*
   * Get the Network Selection Data from the configuration file
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [const std::string&] nf_instance_id: NF instance ID
   * @param [const oai::amf::model::SliceInfoForRegistration&] slice_info: Slice
   * information
   * @param [oai::amf::model::AuthorizedNetworkSliceInfo&]
   * authorized_network_slice_info: Authorized Network Slice Info
   * @return true if can get Network Selection Data from the configuration file
   */
  bool get_network_slice_selection_from_conf_file(
      const std::string& nf_instance_id,
      const oai::amf::model::SliceInfoForRegistration& slice_info,
      oai::amf::model::AuthorizedNetworkSliceInfo&
          authorized_network_slice_info) const;

  /*
   * Find the appropriate target AMF from the Authorized Network Slice
   * information (from NSSF/Conf file)
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [oai::amf::model::AuthorizedNetworkSliceInfo&]
   * authorized_network_slice_info: Authorized NSSAI
   * @return true if can get the Target AMF info, otherwise return false
   */
  bool get_target_amf(
      const std::shared_ptr<nas_context>& nc, std::string& target_amf,
      const oai::amf::model::AuthorizedNetworkSliceInfo&
          authorized_network_slice_info);

  /*
   * Select the target AMF from the list of candidates
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [std::string&] target_amf: Target AMF's URI
   * @param [const nlohmann::json&] amf_candidate_list: List of candidate AMFs
   * @return true if can select the appropriate target AMF, otherwise return
   * false
   */
  bool select_target_amf(
      const std::shared_ptr<nas_context>& nc, std::string& target_amf,
      const nlohmann::json& amf_candidate_list);

  /*
   * Send N1 message to the target AMF using N1 Message Notify API
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS
   * Context
   * @param [const std::string&] target_amf: Target AMF's URI
   * @return void
   */
  void send_n1_message_notity(
      const std::shared_ptr<nas_context>& nc,
      const std::string& target_amf) const;

  /*
   * Reroute the Registration Request message to the target AMF via AN
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the UE NAS Context
   * @param [const std::string&] target_amf_set: Target AMF Set
   * @return true if the current AMF can forward Reroute NAS to AN successfully
   */
  bool reroute_nas_via_an(
      const std::shared_ptr<nas_context>& nc,
      const std::string& target_amf_set);

  /*
   * Get the target AMF Set ID info from the Target AMF Set info
   * @param [const std::string&] target_amf_set: Target AMF Set
   * @param [uint16_t&] amf_set_id: Target AMF Set ID
   * @return true if found, otherwise false
   */
  bool get_amf_set_id(const std::string& target_amf_set, uint16_t& amf_set_id);

  /*
   * Update the 5GMM state
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the NAS context
   * @param [const _5gmm_state_t&] state: 5GMM state
   * @return void
   */
  void set_5gmm_state(
      std::shared_ptr<nas_context>& nc, const _5gmm_state_t& state);

  /*
   * Get the 5GMM state
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the NAS context
   * @param [const _5gmm_state_t&] state: 5GMM state
   * @return void
   */
  void get_5gmm_state(
      const std::shared_ptr<nas_context>& nc, _5gmm_state_t& state) const;

  /*
   * Update the 5G CM state
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the NAS context
   * @param [const cm_state_t&] state: 5G CM state
   * @return void
   */
  void set_5gcm_state(
      std::shared_ptr<nas_context>& nc, const cm_state_t& state);

  /*
   * Get the 5G CM state
   * @param [const std::shared_ptr<nas_context>&] nc: Pointer to the NAS context
   * @param [cm_state_t&] state: 5G CM state
   * @return void
   */
  void get_5gcm_state(
      const std::shared_ptr<nas_context>& nc, cm_state_t& state) const;

  // Event Exposure-related functions
  /*
   * Handle the UE Location Change event to trigger the notification to the
   * subscribed NFs
   * @param [std::string] supi: SUPI
   * @param [oai::amf::model::UserLocation] user_location: User location
   * information
   * @param [uint8_t] http_version: HTTP version (for the notification)
   * @return void
   */
  void handle_ue_location_change(
      std::string supi, oai::amf::model::UserLocation, uint8_t http_version);

  /*
   * Handle the UE Reachability Status Change event to trigger the notification
   * to the subscribed NFs
   * @param [std::string] supi: SUPI
   * @param [uint8_t] status: Reachability status
   * @param [uint8_t] http_version: HTTP version (for the notification)
   * @return void
   */
  void handle_ue_reachability_status_change(
      std::string supi, uint8_t status, uint8_t http_version);

  /*
   * Handle the UE Registration Status Change event to trigger the notification
   * to the subscribed NFs
   * @param [std::string] supi: SUPI
   * @param [uint8_t] status: Registration status
   * @param [uint8_t] http_version: HTTP version (for the notification)
   * @return void
   */
  void handle_ue_registration_state_change(
      std::string supi, uint8_t status, uint8_t http_version);

  /*
   * Handle the UE Connectivity State Change event to trigger the notification
   * to the subscribed NFs
   * @param [std::string] supi: SUPI
   * @param [uint8_t] status: UE Connectivity status
   * @param [uint8_t] http_version: HTTP version (for the notification)
   * @return void
   */
  void handle_ue_connectivity_state_change(
      std::string supi, uint8_t status, uint8_t http_version);

 private:
  /*
   * Handle UE-initiated Deregistration Request message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] nas: NAS message
   * @return void
   */
  void ue_initiate_de_registration_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id, bstring nas);

  /*
   * Handle Registration Request message
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the NAS context
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [const std::string&] snn: Serving Network
   * @param [bstring] reg: NAS Registration Request message
   * @return void
   */
  void registration_request_handle(
      std::shared_ptr<nas_context>& nc, const uint32_t ran_ue_ngap_id,
      const long amf_ue_ngap_id, const std::string& snn, bstring reg);

  /*
   * Handle Authentication Response message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] plain_msg: NAS Authentication Response message
   * @return void
   */
  void authentication_response_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
      bstring plain_msg);

  /*
   * Handle Authentication Failure message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] plain_msg: NAS Authentication Failure message
   * @return void
   */
  void authentication_failure_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
      bstring plain_msg);

  /*
   * Handle Security Mode Complete message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] nas_msg: NAS Security Mode Complete message
   * @return void
   */
  void security_mode_complete_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
      bstring nas_msg);

  /*
   * Handle Security Mode Reject message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] nas_msg: NAS Security Mode Reject message
   * @return void
   */
  void security_mode_reject_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
      bstring nas_msg);

  /*
   * Handle Registration Complete message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] nas: NAS message
   * @return void
   */
  void registration_complete_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
      bstring nas_msg);

  /*
   * Handle Service Request message
   * @param [std::shared_ptr<nas_context>&] nc: Pointer to the NAS context
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] nas: NAS Service Request message
   * @return void
   */
  void service_request_handle(
      std::shared_ptr<nas_context> nc, const uint32_t ran_ue_ngap_id,
      const long amf_ue_ngap_id, bstring nas);

  /*
   * Handle Identity Response message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] nas: NAS Service Request message
   * @return void
   */
  void identity_response_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id,
      bstring plain_msg);

  /*
   * Handle UL NAS Transport message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @param [bstring] nas: NAS message
   * @param [const plmn_t&] plmnn: PLMN
   * @return void
   */
  void ul_nas_transport_handle(
      const uint32_t ran_ue_ngap_id, const long amf_ue_ngap_id, bstring nas,
      const plmn_t& plmn);

  /*
   * Sha256 Algorithm
   * @param [unsigned char*] message: Input message
   * @param [int] msg_len: Length of the input message
   * @param [unsigned char*] output: Output message
   * @return void
   */
  void sha256(unsigned char* message, int msg_len, unsigned char* output);

  /*
   * Send ITTI message DL NAS Buffer to task N2
   * @param [bstring&] nas_msg: NAS message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @return void
   */
  void itti_send_dl_nas_buffer_to_task_n2(
      bstring& nas_msg, const uint32_t ran_ue_ngap_id,
      const long amf_ue_ngap_id);

  /*
   * Send ITTI message DL NAS Buffer to task N2
   * @param [uint8_t] cause_value: Value for Cause IE in NAS message
   * @param [const uint32_t] ran_ue_ngap_id: RAN UE NGAP ID
   * @param [const long] amf_ue_ngap_id: AMF UE NGAP ID
   * @return void
   */
  void send_registration_reject_msg(
      const uint8_t cause_value, const uint32_t ran_ue_ngap_id,
      const long amf_ue_ngap_id);

  std::map<long, std::shared_ptr<nas_context>>
      amfueid2nas_context;  // amf ue ngap id
  mutable std::shared_mutex m_amfueid2nas_context;

  std::map<std::string, std::shared_ptr<nas_context>> imsi2nas_context;
  std::map<std::string, long> supi2amfId;
  std::map<std::string, uint32_t> supi2ranId;
  mutable std::shared_mutex m_nas_context;

  std::map<std::string, std::shared_ptr<nas_context>> guti2nas_context;
  mutable std::shared_mutex m_guti2nas_context;

  static std::map<std::string, std::string> rand_record;
  static uint8_t no_random_delta;
  random_state_t random_state;
  database_t* db_desc;

  // for Event Handling
  amf_event event_sub;
  bs2::connection ee_ue_location_report_connection;
  bs2::connection ee_ue_reachability_status_connection;
  bs2::connection ee_ue_registration_state_connection;
  bs2::connection ee_ue_connectivity_state_connection;
};
}  // namespace amf_application

#endif
