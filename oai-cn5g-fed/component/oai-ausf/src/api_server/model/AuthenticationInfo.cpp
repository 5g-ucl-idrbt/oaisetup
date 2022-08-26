/**
 * AUSF API
 * AUSF UE Authentication Service. © 2020, 3GPP Organizational Partners (ARIB,
 * ATIS, CCSA, ETSI, TSDSI, TTA, TTC). All rights reserved.
 *
 * The version of the OpenAPI document: 1.1.1
 *
 *
 * NOTE: This class is auto generated by OpenAPI Generator
 * (https://openapi-generator.tech). https://openapi-generator.tech Do not edit
 * the class manually.
 */

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

#include "AuthenticationInfo.h"

namespace oai {
namespace ausf_server {
namespace model {

AuthenticationInfo::AuthenticationInfo() {
  m_SupiOrSuci                 = "";
  m_ServingNetworkName         = "";
  m_ResynchronizationInfoIsSet = false;
  m_Pei                        = "";
  m_PeiIsSet                   = false;
  m_TraceDataIsSet             = false;
  m_UdmGroupId                 = "";
  m_UdmGroupIdIsSet            = false;
  m_RoutingIndicator           = "";
  m_RoutingIndicatorIsSet      = false;
  m_CellCagInfoIsSet           = false;
  m_N5gcInd                    = false;
  m_N5gcIndIsSet               = false;
}

AuthenticationInfo::~AuthenticationInfo() {}

void AuthenticationInfo::validate() {
  // TODO: implement validation
}

void to_json(nlohmann::json& j, const AuthenticationInfo& o) {
  j                       = nlohmann::json();
  j["supiOrSuci"]         = o.m_SupiOrSuci;
  j["servingNetworkName"] = o.m_ServingNetworkName;
  if (o.resynchronizationInfoIsSet())
    j["resynchronizationInfo"] = o.m_ResynchronizationInfo;
  if (o.peiIsSet()) j["pei"] = o.m_Pei;
  if (o.traceDataIsSet()) j["traceData"] = o.m_TraceData;
  if (o.udmGroupIdIsSet()) j["udmGroupId"] = o.m_UdmGroupId;
  if (o.routingIndicatorIsSet()) j["routingIndicator"] = o.m_RoutingIndicator;
  if (o.cellCagInfoIsSet() || !o.m_CellCagInfo.empty())
    j["cellCagInfo"] = o.m_CellCagInfo;
  if (o.n5gcIndIsSet()) j["n5gcInd"] = o.m_N5gcInd;
}

void from_json(const nlohmann::json& j, AuthenticationInfo& o) {
  j.at("supiOrSuci").get_to(o.m_SupiOrSuci);
  j.at("servingNetworkName").get_to(o.m_ServingNetworkName);
  if (j.find("resynchronizationInfo") != j.end()) {
    j.at("resynchronizationInfo").get_to(o.m_ResynchronizationInfo);
    o.m_ResynchronizationInfoIsSet = true;
  }
  if (j.find("pei") != j.end()) {
    j.at("pei").get_to(o.m_Pei);
    o.m_PeiIsSet = true;
  }
  if (j.find("traceData") != j.end()) {
    j.at("traceData").get_to(o.m_TraceData);
    o.m_TraceDataIsSet = true;
  }
  if (j.find("udmGroupId") != j.end()) {
    j.at("udmGroupId").get_to(o.m_UdmGroupId);
    o.m_UdmGroupIdIsSet = true;
  }
  if (j.find("routingIndicator") != j.end()) {
    j.at("routingIndicator").get_to(o.m_RoutingIndicator);
    o.m_RoutingIndicatorIsSet = true;
  }
  if (j.find("cellCagInfo") != j.end()) {
    j.at("cellCagInfo").get_to(o.m_CellCagInfo);
    o.m_CellCagInfoIsSet = true;
  }
  if (j.find("n5gcInd") != j.end()) {
    j.at("n5gcInd").get_to(o.m_N5gcInd);
    o.m_N5gcIndIsSet = true;
  }
}

std::string AuthenticationInfo::getSupiOrSuci() const {
  return m_SupiOrSuci;
}
void AuthenticationInfo::setSupiOrSuci(std::string const& value) {
  m_SupiOrSuci = value;
}
std::string AuthenticationInfo::getServingNetworkName() const {
  return m_ServingNetworkName;
}
void AuthenticationInfo::setServingNetworkName(std::string const& value) {
  m_ServingNetworkName = value;
}
ResynchronizationInfo AuthenticationInfo::getResynchronizationInfo() const {
  return m_ResynchronizationInfo;
}
void AuthenticationInfo::setResynchronizationInfo(
    ResynchronizationInfo const& value) {
  m_ResynchronizationInfo      = value;
  m_ResynchronizationInfoIsSet = true;
}
bool AuthenticationInfo::resynchronizationInfoIsSet() const {
  return m_ResynchronizationInfoIsSet;
}
void AuthenticationInfo::unsetResynchronizationInfo() {
  m_ResynchronizationInfoIsSet = false;
}
std::string AuthenticationInfo::getPei() const {
  return m_Pei;
}
void AuthenticationInfo::setPei(std::string const& value) {
  m_Pei      = value;
  m_PeiIsSet = true;
}
bool AuthenticationInfo::peiIsSet() const {
  return m_PeiIsSet;
}
void AuthenticationInfo::unsetPei() {
  m_PeiIsSet = false;
}
TraceData AuthenticationInfo::getTraceData() const {
  return m_TraceData;
}
void AuthenticationInfo::setTraceData(TraceData const& value) {
  m_TraceData      = value;
  m_TraceDataIsSet = true;
}
bool AuthenticationInfo::traceDataIsSet() const {
  return m_TraceDataIsSet;
}
void AuthenticationInfo::unsetTraceData() {
  m_TraceDataIsSet = false;
}
std::string AuthenticationInfo::getUdmGroupId() const {
  return m_UdmGroupId;
}
void AuthenticationInfo::setUdmGroupId(std::string const& value) {
  m_UdmGroupId      = value;
  m_UdmGroupIdIsSet = true;
}
bool AuthenticationInfo::udmGroupIdIsSet() const {
  return m_UdmGroupIdIsSet;
}
void AuthenticationInfo::unsetUdmGroupId() {
  m_UdmGroupIdIsSet = false;
}
std::string AuthenticationInfo::getRoutingIndicator() const {
  return m_RoutingIndicator;
}
void AuthenticationInfo::setRoutingIndicator(std::string const& value) {
  m_RoutingIndicator      = value;
  m_RoutingIndicatorIsSet = true;
}
bool AuthenticationInfo::routingIndicatorIsSet() const {
  return m_RoutingIndicatorIsSet;
}
void AuthenticationInfo::unsetRoutingIndicator() {
  m_RoutingIndicatorIsSet = false;
}
std::vector<std::string>& AuthenticationInfo::getCellCagInfo() {
  return m_CellCagInfo;
}
void AuthenticationInfo::setCellCagInfo(std::vector<std::string> const& value) {
  m_CellCagInfo      = value;
  m_CellCagInfoIsSet = true;
}
bool AuthenticationInfo::cellCagInfoIsSet() const {
  return m_CellCagInfoIsSet;
}
void AuthenticationInfo::unsetCellCagInfo() {
  m_CellCagInfoIsSet = false;
}
bool AuthenticationInfo::isN5gcInd() const {
  return m_N5gcInd;
}
void AuthenticationInfo::setN5gcInd(bool const value) {
  m_N5gcInd      = value;
  m_N5gcIndIsSet = true;
}
bool AuthenticationInfo::n5gcIndIsSet() const {
  return m_N5gcIndIsSet;
}
void AuthenticationInfo::unsetN5gcInd() {
  m_N5gcIndIsSet = false;
}

}  // namespace model
}  // namespace ausf_server
}  // namespace oai