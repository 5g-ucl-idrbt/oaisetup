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

/*! \file
 \brief
 \author
 \date 2021
 \email: contact@openairinterface.org
 */

#include "UEAssociationLogicalNGConnectionList.hpp"

extern "C" {
#include "dynamic_memory_check.h"
}

#include <iostream>
using namespace std;

namespace ngap {

UEAssociationLogicalNGConnectionList::UEAssociationLogicalNGConnectionList() {
  ueAssociationLogicalNGConnectionItem = nullptr;
  number_of_items                      = 0;
}

UEAssociationLogicalNGConnectionList::~UEAssociationLogicalNGConnectionList() {
  if (ueAssociationLogicalNGConnectionItem)
    delete[] ueAssociationLogicalNGConnectionItem;
}
//------------------------------------------------------------------------------
void UEAssociationLogicalNGConnectionList::
    setUEAssociationLogicalNGConnectionItem(
        UEAssociationLogicalNGConnectionItem*
            m_UEAssociationLogicalNGConnectionItem,
        int num) {
  ueAssociationLogicalNGConnectionItem = m_UEAssociationLogicalNGConnectionItem;
  number_of_items                      = num;
}

//------------------------------------------------------------------------------
void UEAssociationLogicalNGConnectionList::
    getUEAssociationLogicalNGConnectionItem(
        UEAssociationLogicalNGConnectionItem*&
            m_UEAssociationLogicalNGConnectionItem,
        int& num) {
  m_UEAssociationLogicalNGConnectionItem = ueAssociationLogicalNGConnectionItem;
  num                                    = number_of_items;
}

//------------------------------------------------------------------------------
void UEAssociationLogicalNGConnectionList::
    setUEAssociationLogicalNGConnectionItem(
        std::vector<UEAssociationLogicalNGConnectionItem>& list) {
  number_of_items = list.size();
  ueAssociationLogicalNGConnectionItem =
      new UEAssociationLogicalNGConnectionItem[number_of_items]();
  for (int i = 0; i < number_of_items; i++) {
    if (!ueAssociationLogicalNGConnectionItem[i].decode(list[i])) return;
  }
  return;
}

//------------------------------------------------------------------------------
void UEAssociationLogicalNGConnectionList::
    getUEAssociationLogicalNGConnectionItem(
        std::vector<UEAssociationLogicalNGConnectionItem>& list) {
  for (int i = 0; i < number_of_items; i++) {
    list.push_back(ueAssociationLogicalNGConnectionItem[i]);
  }
}

//------------------------------------------------------------------------------
bool UEAssociationLogicalNGConnectionList::encode(
    Ngap_UE_associatedLogicalNG_connectionList_t*
        ue_associatedLogicalNG_connectionList) {
  for (int i = 0; i < number_of_items; i++) {
    Ngap_UE_associatedLogicalNG_connectionItem_t* item =
        (Ngap_UE_associatedLogicalNG_connectionItem_t*) calloc(
            1, sizeof(Ngap_UE_associatedLogicalNG_connectionItem_t));
    if (!item) return false;
    if (!ueAssociationLogicalNGConnectionItem[i].encode(*item)) return false;
    if (ASN_SEQUENCE_ADD(&ue_associatedLogicalNG_connectionList->list, item) !=
        0)
      return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool UEAssociationLogicalNGConnectionList::decode(
    Ngap_UE_associatedLogicalNG_connectionList_t*
        ue_associatedLogicalNG_connectionList) {
  number_of_items = ue_associatedLogicalNG_connectionList->list.count;
  ueAssociationLogicalNGConnectionItem =
      new UEAssociationLogicalNGConnectionItem[number_of_items]();
  for (int i = 0; i < number_of_items; i++) {
    if (!ueAssociationLogicalNGConnectionItem[i].decode(
            ue_associatedLogicalNG_connectionList->list.array[i]))
      return false;
  }
  return true;
}

}  // namespace ngap
