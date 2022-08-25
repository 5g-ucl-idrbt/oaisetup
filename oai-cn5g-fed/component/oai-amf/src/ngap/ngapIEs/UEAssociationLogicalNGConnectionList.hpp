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

#ifndef _UE_ASSOCIATION_LOGICAL_NG_CONNECTION_LIST_H_
#define _UE_ASSOCIATION_LOGICAL_NG_CONNECTION_LIST_H_

#include "UEAssociationLogicalNGConnectionItem.hpp"

#include <vector>

extern "C" {
#include "Ngap_ProtocolIE-Field.h"
#include "Ngap_UE-associatedLogicalNG-connectionItem.h"
#include "Ngap_UE-associatedLogicalNG-connectionList.h"
}

namespace ngap {

class UEAssociationLogicalNGConnectionList {
 public:
  UEAssociationLogicalNGConnectionList();
  virtual ~UEAssociationLogicalNGConnectionList();

  void setUEAssociationLogicalNGConnectionItem(
      UEAssociationLogicalNGConnectionItem*
          m_UEAssociationLogicalNGConnectionItem,
      int num);
  void getUEAssociationLogicalNGConnectionItem(
      UEAssociationLogicalNGConnectionItem*&
          m_UEAssociationLogicalNGConnectionItem,
      int& num);

  void setUEAssociationLogicalNGConnectionItem(
      std::vector<UEAssociationLogicalNGConnectionItem>& list);
  void getUEAssociationLogicalNGConnectionItem(
      std::vector<UEAssociationLogicalNGConnectionItem>& list);

  bool encode(Ngap_UE_associatedLogicalNG_connectionList_t*
                  ue_associatedLogicalNG_connectionList);
  bool decode(Ngap_UE_associatedLogicalNG_connectionList_t*
                  ue_associatedLogicalNG_connectionList);

 private:
  UEAssociationLogicalNGConnectionItem* ueAssociationLogicalNGConnectionItem;
  int number_of_items;
};

}  // namespace ngap
#endif
