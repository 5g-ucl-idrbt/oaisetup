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

/*! \file 3gpp_29.503.h
 \brief
 \author Tien Thinh NGUYEN
 \company Eurecom
 \email: tien-thinh.nguyen@eurecom.fr
 */
#ifndef FILE_3GPP_29_503_SEEN
#define FILE_3GPP_29_503_SEEN

#include <stdint.h>

#include <string>
#include <vector>
#include "3gpp_23.003.h"

typedef struct nssai_s {
  std::vector<snssai_t> default_single_nssais;
  std::vector<snssai_t> single_nssais;
} nssai_t;

#endif
