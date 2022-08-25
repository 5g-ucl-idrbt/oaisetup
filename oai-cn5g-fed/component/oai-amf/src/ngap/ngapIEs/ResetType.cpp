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

#include "ResetType.hpp"
extern "C" {
#include "dynamic_memory_check.h"
}

#include <iostream>
using namespace std;

namespace ngap {

//------------------------------------------------------------------------------
ResetType::ResetType() {
  present                              = Ngap_ResetType_PR_NOTHING;
  nG_Interface                         = 0;
  partOfNG_Interface                   = nullptr;
  ueAssociationLogicalNGConnectionItem = nullptr;
}

//------------------------------------------------------------------------------
ResetType::~ResetType() {}

//------------------------------------------------------------------------------
void ResetType::setResetType(long nG_Interface) {
  this->present      = Ngap_ResetType_PR_nG_Interface;
  this->nG_Interface = nG_Interface;
}

//------------------------------------------------------------------------------
void ResetType::setResetType(
    std::vector<UEAssociationLogicalNGConnectionItem> list) {
  this->present = Ngap_ResetType_PR_partOfNG_Interface;

  if (!partOfNG_Interface) {
    partOfNG_Interface = (UEAssociationLogicalNGConnectionList*) calloc(
        1, sizeof(UEAssociationLogicalNGConnectionList));
  }
  partOfNG_Interface->setUEAssociationLogicalNGConnectionItem(list);
}

//------------------------------------------------------------------------------
void ResetType::getResetType(
    struct Ngap_UE_associatedLogicalNG_connectionList*&) {
  // TODO:
  return;
}

//------------------------------------------------------------------------------
bool ResetType::encode(Ngap_ResetType_t* type) {
  // TODO:
  return true;
}

//------------------------------------------------------------------------------
bool ResetType::decode(Ngap_ResetType_t* type) {
  present = type->present;
  if (type->present == Ngap_ResetType_PR_nG_Interface) {
    nG_Interface = (long) type->choice.nG_Interface;
    return true;
  } else if (type->present == Ngap_ResetType_PR_partOfNG_Interface) {
    int num_item       = type->choice.partOfNG_Interface->list.count;
    partOfNG_Interface = new UEAssociationLogicalNGConnectionList();
    partOfNG_Interface->decode(type->choice.partOfNG_Interface);
  } else {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void ResetType::getResetType(long& resetType) {
  // TODO
}

uint8_t ResetType::getResetType() {
  return present;
}

//------------------------------------------------------------------------------
void ResetType::getUE_associatedLogicalNG_connectionList(
    struct Ngap_UE_associatedLogicalNG_connectionList*& list) {
  // TODO:
}

//------------------------------------------------------------------------------
void ResetType::setUE_associatedLogicalNG_connectionList(
    std::vector<UEAssociationLogicalNGConnectionItem> list) {
  if (!partOfNG_Interface) {
    partOfNG_Interface = new UEAssociationLogicalNGConnectionList();
  }
  partOfNG_Interface->setUEAssociationLogicalNGConnectionItem(list);
}

//------------------------------------------------------------------------------
void ResetType::getUE_associatedLogicalNG_connectionList(
    std::vector<UEAssociationLogicalNGConnectionItem>& list) {
  if (partOfNG_Interface) {
    partOfNG_Interface->getUEAssociationLogicalNGConnectionItem(list);
  }
}

}  // namespace ngap
