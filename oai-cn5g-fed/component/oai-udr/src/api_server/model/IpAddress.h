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
 * Nudr_DataRepository API OpenAPI file
 * Unified Data Repository Service. © 2020, 3GPP Organizational Partners (ARIB,
 * ATIS, CCSA, ETSI, TSDSI, TTA, TTC). All rights reserved.
 *
 * The version of the OpenAPI document: 2.1.2
 *
 *
 * NOTE: This class is auto generated by OpenAPI Generator
 * (https://openapi-generator.tech). https://openapi-generator.tech Do not edit
 * the class manually.
 */
/*
 * IpAddress.h
 *
 *
 */

#ifndef IpAddress_H_
#define IpAddress_H_

#include <nlohmann/json.hpp>
#include <string>

#include "Ipv6Addr.h"
#include "Ipv6Prefix.h"

namespace oai::udr::model {

/// <summary>
///
/// </summary>
class IpAddress {
 public:
  IpAddress();
  virtual ~IpAddress();

  void validate();

  /////////////////////////////////////////////
  /// IpAddress members

  /// <summary>
  ///
  /// </summary>
  std::string getIpv4Addr() const;
  void setIpv4Addr(std::string const& value);
  bool ipv4AddrIsSet() const;
  void unsetIpv4Addr();
  /// <summary>
  ///
  /// </summary>
  Ipv6Addr getIpv6Addr() const;
  void setIpv6Addr(Ipv6Addr const& value);
  bool ipv6AddrIsSet() const;
  void unsetIpv6Addr();
  /// <summary>
  ///
  /// </summary>
  Ipv6Prefix getIpv6Prefix() const;
  void setIpv6Prefix(Ipv6Prefix const& value);
  bool ipv6PrefixIsSet() const;
  void unsetIpv6Prefix();

  friend void to_json(nlohmann::json& j, const IpAddress& o);
  friend void from_json(const nlohmann::json& j, IpAddress& o);

 protected:
  std::string m_Ipv4Addr;
  bool m_Ipv4AddrIsSet;
  Ipv6Addr m_Ipv6Addr;
  bool m_Ipv6AddrIsSet;
  Ipv6Prefix m_Ipv6Prefix;
  bool m_Ipv6PrefixIsSet;
};

}  // namespace oai::udr::model

#endif /* IpAddress_H_ */