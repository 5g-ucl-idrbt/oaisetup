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
/**
 * Nudm_SDM
 * Nudm Subscriber Data Management Service. � 2019, 3GPP Organizational Partners
 * (ARIB, ATIS, CCSA, ETSI, TSDSI, TTA, TTC). All rights reserved.
 *
 * The version of the OpenAPI document: 2.1.0.alpha-1
 *
 *
 * NOTE: This class is auto generated by OpenAPI Generator
 * (https://openapi-generator.tech). https://openapi-generator.tech Do not edit
 * the class manually.
 */
/*
 * SharedData.h
 *
 *
 */

#ifndef SharedData_H_
#define SharedData_H_

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "AccessAndMobilitySubscriptionData.h"
#include "DnnConfiguration.h"
#include "SmsManagementSubscriptionData.h"
#include "SmsSubscriptionData.h"
#include "SnssaiInfo.h"
#include "TraceData.h"

namespace oai {
namespace udm {
namespace model {

/// <summary>
///
/// </summary>
class SharedData {
 public:
  SharedData();
  virtual ~SharedData();

  void validate();

  /////////////////////////////////////////////
  /// SharedData members

  /// <summary>
  ///
  /// </summary>
  std::string getSharedDataId() const;
  void setSharedDataId(std::string const& value);
  /// <summary>
  ///
  /// </summary>
  AccessAndMobilitySubscriptionData getSharedAmData() const;
  void setSharedAmData(AccessAndMobilitySubscriptionData const& value);
  bool sharedAmDataIsSet() const;
  void unsetSharedAmData();
  /// <summary>
  ///
  /// </summary>
  SmsSubscriptionData getSharedSmsSubsData() const;
  void setSharedSmsSubsData(SmsSubscriptionData const& value);
  bool sharedSmsSubsDataIsSet() const;
  void unsetSharedSmsSubsData();
  /// <summary>
  ///
  /// </summary>
  SmsManagementSubscriptionData getSharedSmsMngSubsData() const;
  void setSharedSmsMngSubsData(SmsManagementSubscriptionData const& value);
  bool sharedSmsMngSubsDataIsSet() const;
  void unsetSharedSmsMngSubsData();
  /// <summary>
  ///
  /// </summary>
  std::map<std::string, DnnConfiguration>& getSharedDnnConfigurations();
  bool sharedDnnConfigurationsIsSet() const;
  void unsetSharedDnnConfigurations();
  /// <summary>
  ///
  /// </summary>
  TraceData getSharedTraceData() const;
  void setSharedTraceData(TraceData const& value);
  bool sharedTraceDataIsSet() const;
  void unsetSharedTraceData();
  /// <summary>
  ///
  /// </summary>
  std::map<std::string, SnssaiInfo>& getSharedSnssaiInfos();
  bool sharedSnssaiInfosIsSet() const;
  void unsetSharedSnssaiInfos();

  friend void to_json(nlohmann::json& j, const SharedData& o);
  friend void from_json(const nlohmann::json& j, SharedData& o);

 protected:
  std::string m_SharedDataId;

  AccessAndMobilitySubscriptionData m_SharedAmData;
  bool m_SharedAmDataIsSet;
  SmsSubscriptionData m_SharedSmsSubsData;
  bool m_SharedSmsSubsDataIsSet;
  SmsManagementSubscriptionData m_SharedSmsMngSubsData;
  bool m_SharedSmsMngSubsDataIsSet;
  std::map<std::string, DnnConfiguration> m_SharedDnnConfigurations;
  bool m_SharedDnnConfigurationsIsSet;
  TraceData m_SharedTraceData;
  bool m_SharedTraceDataIsSet;
  std::map<std::string, SnssaiInfo> m_SharedSnssaiInfos;
  bool m_SharedSnssaiInfosIsSet;
};

}  // namespace model
}  // namespace udm
}  // namespace oai

#endif /* SharedData_H_ */
