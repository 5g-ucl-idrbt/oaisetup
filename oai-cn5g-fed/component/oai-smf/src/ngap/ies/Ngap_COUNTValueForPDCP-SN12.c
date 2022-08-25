/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NGAP-IEs"
 * 	found in "asn.1/Information Element Definitions.asn1"
 * 	`asn1c -pdu=all -fcompound-names -fno-include-deps -findirect-choice
 * -gen-PER -D src`
 */

#include "Ngap_COUNTValueForPDCP-SN12.h"

#include "Ngap_ProtocolExtensionContainer.h"
static int memb_Ngap_pDCP_SN12_constraint_1(
    const asn_TYPE_descriptor_t* td, const void* sptr,
    asn_app_constraint_failed_f* ctfailcb, void* app_key) {
  long value;

  if (!sptr) {
    ASN__CTFAIL(
        app_key, td, sptr, "%s: value not given (%s:%d)", td->name, __FILE__,
        __LINE__);
    return -1;
  }

  value = *(const long*) sptr;

  if ((value >= 0 && value <= 4095)) {
    /* Constraint check succeeded */
    return 0;
  } else {
    ASN__CTFAIL(
        app_key, td, sptr, "%s: constraint failed (%s:%d)", td->name, __FILE__,
        __LINE__);
    return -1;
  }
}

static int memb_Ngap_hFN_PDCP_SN12_constraint_1(
    const asn_TYPE_descriptor_t* td, const void* sptr,
    asn_app_constraint_failed_f* ctfailcb, void* app_key) {
  long value;

  if (!sptr) {
    ASN__CTFAIL(
        app_key, td, sptr, "%s: value not given (%s:%d)", td->name, __FILE__,
        __LINE__);
    return -1;
  }

  value = *(const long*) sptr;

  if ((value >= 0 && value <= 1048575)) {
    /* Constraint check succeeded */
    return 0;
  } else {
    ASN__CTFAIL(
        app_key, td, sptr, "%s: constraint failed (%s:%d)", td->name, __FILE__,
        __LINE__);
    return -1;
  }
}

static asn_oer_constraints_t asn_OER_memb_Ngap_pDCP_SN12_constr_2 CC_NOTUSED = {
    {2, 1} /* (0..4095) */,
    -1};
static asn_per_constraints_t asn_PER_memb_Ngap_pDCP_SN12_constr_2 CC_NOTUSED = {
    {APC_CONSTRAINED, 12, 12, 0, 4095} /* (0..4095) */,
    {APC_UNCONSTRAINED, -1, -1, 0, 0},
    0,
    0 /* No PER value map */
};
static asn_oer_constraints_t asn_OER_memb_Ngap_hFN_PDCP_SN12_constr_3
    CC_NOTUSED = {{4, 1} /* (0..1048575) */, -1};
static asn_per_constraints_t asn_PER_memb_Ngap_hFN_PDCP_SN12_constr_3
    CC_NOTUSED = {
        {APC_CONSTRAINED, 20, -1, 0, 1048575} /* (0..1048575) */,
        {APC_UNCONSTRAINED, -1, -1, 0, 0},
        0,
        0 /* No PER value map */
};
asn_TYPE_member_t asn_MBR_Ngap_COUNTValueForPDCP_SN12_1[] = {
    {ATF_NOFLAGS,
     0,
     offsetof(struct Ngap_COUNTValueForPDCP_SN12, pDCP_SN12),
     (ASN_TAG_CLASS_CONTEXT | (0 << 2)),
     -1, /* IMPLICIT tag at current level */
     &asn_DEF_NativeInteger,
     0,
     {&asn_OER_memb_Ngap_pDCP_SN12_constr_2,
      &asn_PER_memb_Ngap_pDCP_SN12_constr_2, memb_Ngap_pDCP_SN12_constraint_1},
     0,
     0, /* No default value */
     "pDCP-SN12"},
    {ATF_NOFLAGS,
     0,
     offsetof(struct Ngap_COUNTValueForPDCP_SN12, hFN_PDCP_SN12),
     (ASN_TAG_CLASS_CONTEXT | (1 << 2)),
     -1, /* IMPLICIT tag at current level */
     &asn_DEF_NativeInteger,
     0,
     {&asn_OER_memb_Ngap_hFN_PDCP_SN12_constr_3,
      &asn_PER_memb_Ngap_hFN_PDCP_SN12_constr_3,
      memb_Ngap_hFN_PDCP_SN12_constraint_1},
     0,
     0, /* No default value */
     "hFN-PDCP-SN12"},
    {ATF_POINTER,
     1,
     offsetof(struct Ngap_COUNTValueForPDCP_SN12, iE_Extensions),
     (ASN_TAG_CLASS_CONTEXT | (2 << 2)),
     -1, /* IMPLICIT tag at current level */
     &asn_DEF_Ngap_ProtocolExtensionContainer_175P32,
     0,
     {0, 0, 0},
     0,
     0, /* No default value */
     "iE-Extensions"},
};
static const int asn_MAP_Ngap_COUNTValueForPDCP_SN12_oms_1[]            = {2};
static const ber_tlv_tag_t asn_DEF_Ngap_COUNTValueForPDCP_SN12_tags_1[] = {
    (ASN_TAG_CLASS_UNIVERSAL | (16 << 2))};
static const asn_TYPE_tag2member_t
    asn_MAP_Ngap_COUNTValueForPDCP_SN12_tag2el_1[] = {
        {(ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0}, /* pDCP-SN12 */
        {(ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0}, /* hFN-PDCP-SN12 */
        {(ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0}  /* iE-Extensions */
};
asn_SEQUENCE_specifics_t asn_SPC_Ngap_COUNTValueForPDCP_SN12_specs_1 = {
    sizeof(struct Ngap_COUNTValueForPDCP_SN12),
    offsetof(struct Ngap_COUNTValueForPDCP_SN12, _asn_ctx),
    asn_MAP_Ngap_COUNTValueForPDCP_SN12_tag2el_1,
    3,                                         /* Count of tags in the map */
    asn_MAP_Ngap_COUNTValueForPDCP_SN12_oms_1, /* Optional members */
    1,
    0, /* Root/Additions */
    3, /* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_Ngap_COUNTValueForPDCP_SN12 = {
    "COUNTValueForPDCP-SN12",
    "COUNTValueForPDCP-SN12",
    &asn_OP_SEQUENCE,
    asn_DEF_Ngap_COUNTValueForPDCP_SN12_tags_1,
    sizeof(asn_DEF_Ngap_COUNTValueForPDCP_SN12_tags_1) /
        sizeof(asn_DEF_Ngap_COUNTValueForPDCP_SN12_tags_1[0]), /* 1 */
    asn_DEF_Ngap_COUNTValueForPDCP_SN12_tags_1, /* Same as above */
    sizeof(asn_DEF_Ngap_COUNTValueForPDCP_SN12_tags_1) /
        sizeof(asn_DEF_Ngap_COUNTValueForPDCP_SN12_tags_1[0]), /* 1 */
    {0, 0, SEQUENCE_constraint},
    asn_MBR_Ngap_COUNTValueForPDCP_SN12_1,
    3,                                           /* Elements count */
    &asn_SPC_Ngap_COUNTValueForPDCP_SN12_specs_1 /* Additional specs */
};
