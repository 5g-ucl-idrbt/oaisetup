/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
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
 *      conmnc_digit_lengtht@openairinterface.org
 */

#include "nr_mac_gNB.h"

#include "mac_rrc_ul.h"

static void initial_ul_rrc_message_transfer_direct(module_id_t module_id, const f1ap_initial_ul_rrc_message_t *ul_rrc)
{
  MessageDef *msg = itti_alloc_new_message(TASK_MAC_GNB, 0, F1AP_INITIAL_UL_RRC_MESSAGE);
  /* copy all fields, but reallocate rrc_containers! */
  f1ap_initial_ul_rrc_message_t *f1ap_msg = &F1AP_INITIAL_UL_RRC_MESSAGE(msg);
  *f1ap_msg = *ul_rrc;

  f1ap_msg->rrc_container = malloc(ul_rrc->rrc_container_length);
  DevAssert(f1ap_msg->rrc_container);
  memcpy(f1ap_msg->rrc_container, ul_rrc->rrc_container, ul_rrc->rrc_container_length);
  f1ap_msg->rrc_container_length = ul_rrc->rrc_container_length;

  f1ap_msg->du2cu_rrc_container = malloc(ul_rrc->du2cu_rrc_container_length);
  DevAssert(f1ap_msg->du2cu_rrc_container);
  memcpy(f1ap_msg->du2cu_rrc_container, ul_rrc->du2cu_rrc_container, ul_rrc->du2cu_rrc_container_length);
  f1ap_msg->du2cu_rrc_container_length = ul_rrc->du2cu_rrc_container_length;

  itti_send_msg_to_task(TASK_RRC_GNB, module_id, msg);
}

void mac_rrc_ul_direct_init(struct nr_mac_rrc_ul_if_s *mac_rrc)
{
  mac_rrc->initial_ul_rrc_message_transfer = initial_ul_rrc_message_transfer_direct;
}
