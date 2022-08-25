/**
 * Namf_EventExposure
 * AMF Event Exposure Service © 2019, 3GPP Organizational Partners (ARIB, ATIS,
 * CCSA, ETSI, TSDSI, TTA, TTC). All rights reserved.
 *
 * The version of the OpenAPI document: 1.1.0.alpha-1
 *
 *
 * NOTE: This class is auto generated by OpenAPI Generator
 * (https://openapi-generator.tech). https://openapi-generator.tech Do not edit
 * the class manually.
 */

#include "AmfUpdatedEventSubscription.h"
#include "Helpers.h"

#include <sstream>

namespace oai::amf::model {

AmfUpdatedEventSubscription::AmfUpdatedEventSubscription() {}

void AmfUpdatedEventSubscription::validate() const {
  std::stringstream msg;
  if (!validate(msg)) {
    throw oai::amf::helpers::ValidationException(msg.str());
  }
}

bool AmfUpdatedEventSubscription::validate(std::stringstream& msg) const {
  return validate(msg, "");
}

bool AmfUpdatedEventSubscription::validate(
    std::stringstream& msg, const std::string& pathPrefix) const {
  bool success = true;
  const std::string _pathPrefix =
      pathPrefix.empty() ? "AmfUpdatedEventSubscription" : pathPrefix;

  return success;
}

bool AmfUpdatedEventSubscription::operator==(
    const AmfUpdatedEventSubscription& rhs) const {
  return

      (getSubscription() == rhs.getSubscription())

          ;
}

bool AmfUpdatedEventSubscription::operator!=(
    const AmfUpdatedEventSubscription& rhs) const {
  return !(*this == rhs);
}

void to_json(nlohmann::json& j, const AmfUpdatedEventSubscription& o) {
  j                 = nlohmann::json();
  j["subscription"] = o.m_Subscription;
}

void from_json(const nlohmann::json& j, AmfUpdatedEventSubscription& o) {
  j.at("subscription").get_to(o.m_Subscription);
}

AmfEventSubscription AmfUpdatedEventSubscription::getSubscription() const {
  return m_Subscription;
}
void AmfUpdatedEventSubscription::setSubscription(
    AmfEventSubscription const& value) {
  m_Subscription = value;
}

}  // namespace oai::amf::model
