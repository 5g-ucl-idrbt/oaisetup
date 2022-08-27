/**
 * NSSF NSSAI Availability
 * NSSF NSSAI Availability Service. © 2021, 3GPP Organizational Partners (ARIB,
 * ATIS, CCSA, ETSI, TSDSI, TTA, TTC). All rights reserved.
 *
 * The version of the OpenAPI document: 1.1.4
 *
 *
 * NOTE: This class is auto generated by OpenAPI Generator
 * (https://openapi-generator.tech). https://openapi-generator.tech Do not edit
 * the class manually.
 */
/*
 * NssfEventSubscriptionCreatedData.h
 *
 *
 */

#ifndef NssfEventSubscriptionCreatedData_H_
#define NssfEventSubscriptionCreatedData_H_

#include "AuthorizedNssaiAvailabilityData.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace oai {
namespace nssf_server {
namespace model {

/// <summary>
///
/// </summary>
class NssfEventSubscriptionCreatedData {
 public:
  NssfEventSubscriptionCreatedData();
  virtual ~NssfEventSubscriptionCreatedData() = default;

  /// <summary>
  /// Validate the current data in the model. Throws a ValidationException on
  /// failure.
  /// </summary>
  void validate() const;

  /// <summary>
  /// Validate the current data in the model. Returns false on error and writes
  /// an error message into the given stringstream.
  /// </summary>
  bool validate(std::stringstream& msg) const;

  /// <summary>
  /// Helper overload for validate. Used when one model stores another model and
  /// calls it's validate. Not meant to be called outside that case.
  /// </summary>
  bool validate(std::stringstream& msg, const std::string& pathPrefix) const;

  bool operator==(const NssfEventSubscriptionCreatedData& rhs) const;
  bool operator!=(const NssfEventSubscriptionCreatedData& rhs) const;

  /////////////////////////////////////////////
  /// NssfEventSubscriptionCreatedData members

  /// <summary>
  ///
  /// </summary>
  std::string getSubscriptionId() const;
  void setSubscriptionId(std::string const& value);
  /// <summary>
  ///
  /// </summary>
  std::string getExpiry() const;
  void setExpiry(std::string const& value);
  bool expiryIsSet() const;
  void unsetExpiry();
  /// <summary>
  ///
  /// </summary>
  std::vector<AuthorizedNssaiAvailabilityData>
  getAuthorizedNssaiAvailabilityData() const;
  void setAuthorizedNssaiAvailabilityData(
      std::vector<AuthorizedNssaiAvailabilityData> const& value);
  bool authorizedNssaiAvailabilityDataIsSet() const;
  void unsetAuthorizedNssaiAvailabilityData();
  /// <summary>
  ///
  /// </summary>
  std::string getSupportedFeatures() const;
  void setSupportedFeatures(std::string const& value);
  bool supportedFeaturesIsSet() const;
  void unsetSupportedFeatures();

  friend void to_json(
      nlohmann::json& j, const NssfEventSubscriptionCreatedData& o);
  friend void from_json(
      const nlohmann::json& j, NssfEventSubscriptionCreatedData& o);

 protected:
  std::string m_SubscriptionId;

  std::string m_Expiry;
  bool m_ExpiryIsSet;
  std::vector<AuthorizedNssaiAvailabilityData>
      m_AuthorizedNssaiAvailabilityData;
  bool m_AuthorizedNssaiAvailabilityDataIsSet;
  std::string m_SupportedFeatures;
  bool m_SupportedFeaturesIsSet;
};

}  // namespace model
}  // namespace nssf_server
}  // namespace oai

#endif /* NssfEventSubscriptionCreatedData_H_ */