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

#include "ProvisionedParameterDataDocumentApiImpl.h"

#include "logger.hpp"
#include "udr_app.hpp"

namespace oai::udr::api {

using namespace oai::udr::model;

ProvisionedParameterDataDocumentApiImpl::
    ProvisionedParameterDataDocumentApiImpl(
        std::shared_ptr<Pistache::Rest::Router> rtr, udr_app* udr_app_inst,
        std::string address)
    : ProvisionedParameterDataDocumentApi(rtr),
      m_udr_app(udr_app_inst),
      m_address(address) {}

void ProvisionedParameterDataDocumentApiImpl::modify_pp_data(
    const std::string& ueId, const std::vector<PatchItem>& patchItem,
    const Pistache::Optional<std::string>& supportedFeatures,
    Pistache::Http::ResponseWriter& response) {
  response.send(
      Pistache::Http::Code::Ok, "This API has not been implemented yet!\n");
}

}  // namespace oai::udr::api
