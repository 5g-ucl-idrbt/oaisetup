/*
  Frame work to read and write UICC cards
  Copyright (C) Laurent THOMAS, Open Cells Project

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <uicc.h>
#include <milenage.h>
#include <sys/time.h>

struct uicc_vals {
  bool setIt=false;
  string adm="";
  string iccid="";
  string imsi="";
  string opc="";
  string op="";
  string isdn="";
  string acc="";
  string key="";
  string spn="open cells";
  string ust="866F1F1C231E0000400050";
  int mncLen=2;
  bool authenticate=false;
  string sqn="";
  string rand="";
};

#define sc(in, out)           \
  USIMcard.send_check( string( (char*)in +37 ,sizeof(in) -37),  \
                       string( (char*)out+37 ,sizeof(out)-37) )

bool readSIMvalues(char *port) {
  SIM SIMcard;
  string ATR;
  Assert((ATR=SIMcard.open(port))!="", "Failed to open %s", port);
  //dump_hex("ATR", ATR);
  vector<string> res;
  cout << "GSM IMSI: " << SIMcard.decodeIMSI(SIMcard.readFile("IMSI")[0]) << endl;
  // Show only the first isdn (might be several)
  cout << "GSM MSISDN: " << SIMcard.decodeISDN(SIMcard.readFile("MSISDN")[0]) <<endl;
  SIMcard.close();
  return true;
}

int readUSIMvalues(char *port) {
  vector<string> res;
  USIM USIMcard;
  string ATR;
  Assert((ATR=USIMcard.open(port))!="", "Failed to open %s", port);
  //dump_hex("ATR", ATR);
  res=USIMcard.readFile("ICCID");
  string iccid=to_hex(res[0], true);
  cout << "ICCID: " << iccid <<endl;

  if (!luhn( iccid))
    printf("WARNING: iccid luhn encoding of last digit not done \n");

  USIMcard.openUSIM();
  string imsi=USIMcard.readFile("IMSI")[0];
  cout << "USIM IMSI: " << USIMcard.decodeIMSI(imsi) << endl;
  res=USIMcard.readFile("PLMN selector with Access Technology");
  //cout << "USIM PLMN selector: " << bcdToAscii(res[0]) <<endl;
  // Show only the first isdn (might be several)
  string msisdn=USIMcard.readFile("MSISDN")[0];
  cout << "USIM MSISDN: " << USIMcard.decodeISDN(msisdn) <<endl;
  string spn=USIMcard.readFile("Service Provider Name")[0];
  cout << "USIM Service Provider Name: " << printable(spn.substr(1)) <<endl;

  if (USIMcard.debug) {
    string stt=USIMcard.readFile("USIM service table")[0];
    decodeServiceTable(stt);
  }

  return USIMcard.GRver;
}

bool testATR(char *port) {
  SIM card;
  string ATR;
  Assert((ATR=card.open(port))!="", "Failed to open %s", port);
  // Proprietary handcheck to open card flashing
  return ATR[ATR.size()-1] == '\xac'  ||  ATR[ATR.size()-1] == '\xf3';
}

bool writeSIMv2values(char *port, struct uicc_vals &values) {
  SIM card;
  string ATR;
  bool ATRpersonalized=testATR(port);
  Assert((ATR=card.open(port))!="", "Failed to open %s", port);

  // Proprietary handcheck to open card flashing
  if (ATRpersonalized) // if already flashed
    card.send_check(hexa("A0580000083132333431323334"),hexa("9000"), 10);

  card.send_check(hexa("A0580000083132333431323334"),hexa("9000"), 10);

  if (card.debug) {
    // Check the card available AIDs
    vector<string> EFdir=card.readFile("EFDIR");
    card.decodeEFdir(EFdir);
  }

  // Proprietary PIN1, PUK1
  card.send_check(hexa("A0A40000020100"),hexa("9F10"));
  card.send_check(hexa("A0D6000017000000 31323334FFFFFFFF 8383 3838383838383838 8A8A"),hexa("9000"));
  // Proprietary PIN2, PUK2
  card.send_check(hexa("A0A40000020200"),hexa("9F10"));
  card.send_check(hexa("A0D6000017010000 31323334FFFFFFFF 8383 3838383838383838 8A8A"),hexa("9000"));
  // Proprietary ADM
  card.send_check(hexa("A0A40000020B00"),hexa("9F10"));
  card.send_check(hexa("A0D600000D010000") + values.adm + hexa("8A8A"),hexa("9000"));
  //Write ALG Type  1910:Millenage   1920:XOR
  card.send_check(hexa("A0A40000022FD0"),hexa("9F10"));
  card.send_check(hexa("A0D6000002 1910"),hexa("9000"));

  //Write Ki
  if ( values.key.size() == 32 ) {
    card.send_check(hexa("A0A40000020001"),hexa("9F10"));
    card.send_check(hexa("A0D6000010") + hexa(values.key),hexa("9000"));
  } else
    printf("No Key or not 32 char length key\n");

  // Write OPc
  if ( values.opc.size() == 32 ) {
    card.send_check(hexa("A0A40000026002"),hexa("9F10"));
    card.send_check(hexa("A0D600001101") + hexa(values.opc),hexa("9000"),10);
  } else
    printf("No OPc or not 32 char length key\n");

  // Set milenage R and C
  card.send_check(hexa("A0A40000022FE6"),hexa("9F10"));
  card.send_check(hexa("A0DC0104114000000000000000000000000000000000"),hexa("9000"));
  card.send_check(hexa("A0DC0204110000000000000000000000000000000001"),hexa("9000"));
  card.send_check(hexa("A0DC0304112000000000000000000000000000000002"),hexa("9000"));
  card.send_check(hexa("A0DC0404114000000000000000000000000000000004"),hexa("9000"));
  card.send_check(hexa("A0DC0504116000000000000000000000000000000008"),hexa("9000"));
  card.send_check(hexa("A0A40000022FE5"), hexa("9F10"));
  card.send_check(hexa("A0D6000005081C2A0001"),hexa("9000"));

  //
  // We enter regular files, defined in the 3GPP documents
  //
  if (values.iccid.size() > 0)
    Assert(card.writeFile("ICCID", card.encodeICCID(values.iccid)),
           "can't set iccid %s",values.iccid.c_str());

  vector<string> li;
  li.push_back("en");
  Assert(card.writeFile("Extended language preference", li), "can't set language");
  Assert(card.writeFile("language preference", makeBcdVect("01",false)), "can't set language");
  vector<string> ad;
  ad.push_back(makeBcd("810000",false));
  ad[0]+=(char) values.mncLen;
  Assert(card.writeFile("Administrative data", ad),
         "can't set Administrative data");

  if ( values.imsi.size() > 0) {
    Assert(card.writeFile("IMSI", card.encodeIMSI(values.imsi)),
           "can't set imsi %s",values.imsi.c_str());
    string MccMnc=card.encodeMccMnc(values.imsi.substr(0,3),
                                    values.imsi.substr(3,values.mncLen));
    vector<string> VectMccMnc;
    VectMccMnc.push_back(MccMnc);
    Assert(card.writeFile("PLMN selector", VectMccMnc, true), "Can't write PLMN Selector");
    vector<string> loci;
    loci.push_back(makeBcd("",true,4));
    loci[0]+=MccMnc;

    if (values.mncLen == 3 )
      loci[0]+=makeBcd("00ff01", false);
    else
      loci[0]+=makeBcd("0000ff01", false);

    Assert(card.writeFile("Location information",
                          loci), "location information");
  }

  if ( values.acc.size() > 0)
    Assert(card.writeFile("Access control class", card.encodeACC(values.acc)),
           "can't set acc %s",values.acc.c_str());

  vector<string> spn;
  spn.push_back(string(u8"\x01",1));
  spn[0]+=values.spn;
  Assert(card.writeFile("Service Provider Name", spn, true), "can't set spn");
  Assert(card.writeFile("Higher Priority PLMN search period",
                        makeBcdVect("02", false)), "can't set plmn search period");
  Assert(card.writeFile("Forbidden PLMN",
                        makeBcdVect(""),true), "can't set forbidden plmn");
  Assert(card.writeFile("Group Identifier Level 1",
                        makeBcdVect(""),true), "can't set GID1");
  Assert(card.writeFile("Group Identifier Level 2",
                        makeBcdVect(""),true), "can't set GID2");
  // Typical service list, a bit complex to define (see 3GPP TS 51.011)
  Assert(card.writeFile("SIM service table", makeBcdVect("ff33ffff00003f033000",false)),
         "can't set GSM service table");

  if (values.isdn.size() > 0)
    Assert(card.writeFile("MSISDN",
                          card.encodeISDN("9" + values.isdn, card.fileRecordSize("MSISDN"))),
           "can't set msisdn %s",values.isdn.c_str());
  //
  // Set USIM values, from GSM APDU CLA, proprietary method but regular file names
  //
  Assert(card.writeFile("USIM Extended language preference", li), "can't set language");
  Assert(card.writeFile("USIM Administrative data", ad),
         "can't set Administrative data");
  Assert(card.writeFile("USIM Short Message Service Parameters", makeBcdVect("FFFFFFFFFFFFFFFFFFFFFFFFFFF1FFFFFFFFFFFFFFFFFFFFFFFF 0191",true,40)),
         "can't set SMSC");

  if (values.isdn.size() > 0)
    Assert(card.writeFile("USIM MSISDN", card.encodeISDN(values.isdn, card.fileRecordSize("MSISDN"))),
           "can't set msisdn %s",values.isdn.c_str());

  if ( values.acc.size() > 0)
    Assert(card.writeFile("USIM Access control class", card.encodeACC(values.acc)),
           "can't set acc %s",values.acc.c_str());

  if ( values.imsi.size() > 0) {
    Assert(card.writeFile("USIM IMSI", card.encodeIMSI(values.imsi)),
           "can't set imsi %s",values.imsi.c_str());
    string MccMnc=card.encodeMccMnc(values.imsi.substr(0,3),
                                    values.imsi.substr(3,values.mncLen));
    vector<string> VectMccMnc;
    VectMccMnc.push_back(MccMnc);
    vector<string> MccMncWithAct=VectMccMnc;
    // Add EUTRAN access techno only
    // add NG-RAN and satellite NG-RAN
    MccMncWithAct[0]+=string(u8"\x7c\x00",2);
    Assert(card.writeFile("USIM PLMN selector with Access Technology",
                          MccMncWithAct, true), "Can't write PLMN Selector");
    Assert(card.writeFile("USIM Operator controlled PLMN selector with Access Technology",
                          MccMncWithAct, true), "Can't write Operator PLMN Selector");
    Assert(card.writeFile("USIM Home PLMN selector with Access Technology",
                          MccMncWithAct, true), "Can't write home  PLMN Selector");
    vector<string> psloci;
    psloci.push_back(makeBcd("",true,7));
    psloci[0]+=MccMnc;

    if (values.mncLen == 3 )
      psloci[0]+=makeBcd("00ff01", false);
    else
      psloci[0]+=makeBcd("0000ff01", false);

    Assert(card.writeFile("USIM PS Location information",
                          psloci,false),
           "PS location information");
    vector<string> csloci;
    csloci.push_back(makeBcd("",true,4));
    csloci[0]+=MccMnc;

    if (values.mncLen == 3 )
      csloci[0]+=makeBcd("00ff01", false);
    else
      csloci[0]+=makeBcd("0000ff01", false);

    Assert(card.writeFile("USIM CS Location information",
                          csloci, false),
           "CS location information");
  }

  Assert(card.writeFile("USIM Service Provider Name", spn, true), "can't set spn");
  Assert(card.writeFile("USIM Higher Priority PLMN search period", makeBcdVect("02", false)), "can't set plmn search period");
  Assert(card.writeFile("USIM Forbidden PLMNs", makeBcdVect("",true,12)), "can't set forbidden plmn");
  Assert(card.writeFile("USIM Group Identifier Level 1", makeBcdVect("",true,4)), "can't set GID1");
  Assert(card.writeFile("USIM Group Identifier Level 2", makeBcdVect("",true,4)), "can't set GID2");
  vector<string> ecc;
  ecc.push_back(makeBcd("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",false));
  ecc.push_back(makeBcd("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",false));
  ecc.push_back(makeBcd("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",false));
  ecc.push_back(makeBcd("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",false));
  Assert(card.writeFile("USIM emergency call codes", ecc), "can't set emergency call codes");
  // Typical service list, a bit complex to define (see 3GPP TS 51.011)
  Assert(card.writeFile("USIM service table", makeBcdVect(values.ust, false)),
         "can't set USIM service table");
  return true;
}

bool writeSIMvalues(char *port, struct uicc_vals &values) {
  vector<string> res;
  SIM card;
  string ATR;
  Assert((ATR=card.open(port))!="", "Failed to open %s", port);

  if (!card.verifyChv('\x0a', values.adm)) {
    printf("chv 0a Nok\n");
    return false;
  }

  if (card.debug) {
    // Check the card available AIDs
    vector<string> EFdir=card.readFile("EFDIR");
    card.decodeEFdir(EFdir);
  }

  if (values.iccid.size() > 0)
    Assert(card.writeFile("ICCID", card.encodeICCID(values.iccid)),
           "can't set iccid %s",values.iccid.c_str());

  vector<string> li;
  li.push_back("en");
  Assert(card.writeFile("Extended language preference", li), "can't set language");
  Assert(card.writeFile("language preference", makeBcdVect("01",false)), "can't set language");
  vector<string> ad;
  ad.push_back(makeBcd("810000",false));
  ad[0]+=(char) values.mncLen;
  Assert(card.writeFile("Administrative data", ad),
         "can't set Administrative data");

  if ( values.imsi.size() > 0) {
    Assert(card.writeFile("IMSI", card.encodeIMSI(values.imsi)),
           "can't set imsi %s",values.imsi.c_str());
    string MccMnc=card.encodeMccMnc(values.imsi.substr(0,3),
                                    values.imsi.substr(3,values.mncLen));
    vector<string> VectMccMnc;
    VectMccMnc.push_back(MccMnc);
    Assert(card.writeFile("PLMN selector", VectMccMnc, true), "Can't write PLMN Selector");
    Assert(card.writeFile("Equivalent home PLMN", VectMccMnc), "Can't write Equivalent PLMN");
    vector<string> loci;
    loci.push_back(makeBcd("",true,4));
    loci[0]+=MccMnc;

    if (values.mncLen == 3 )
      loci[0]+=makeBcd("00ff01", false);
    else
      loci[0]+=makeBcd("0000ff01", false);

    Assert(card.writeFile("Location information",
                          loci), "location information");
  }

  if ( values.acc.size() > 0)
    Assert(card.writeFile("Access control class", card.encodeACC(values.acc)),
           "can't set acc %s",values.acc.c_str());

  vector<string> spn;
  spn.push_back(string(u8"\x01",1));
  spn[0]+=values.spn;
  Assert(card.writeFile("Service Provider Name", spn, true), "can't set spn");
  Assert(card.writeFile("Higher Priority PLMN search period",
                        makeBcdVect("02", false)), "can't set plmn search period");
  Assert(card.writeFile("Forbidden PLMN",
                        makeBcdVect(""),true), "can't set forbidden plmn");
  Assert(card.writeFile("Group Identifier Level 1",
                        makeBcdVect(""),true), "can't set GID1");
  Assert(card.writeFile("Group Identifier Level 2",
                        makeBcdVect(""),true), "can't set GID2");
  Assert(card.writeFile("emergency call codes",
                        makeBcdVect(""),true), "can't set emergency call codes");
  // Typical service list, a bit complex to define (see 3GPP TS 51.011)
  Assert(card.writeFile("SIM service table", makeBcdVect("ff33ffff00003f033000f0c3",false)),
         "can't set GSM service table");

  if (values.isdn.size() > 0)
    Assert(card.writeFile("MSISDN",
                          card.encodeISDN(values.isdn, card.fileRecordSize("MSISDN"))),
           "can't set msisdn %s",values.isdn.c_str());

  Assert(card.writeFile("Short Message Service Parameters", makeBcdVect("FFFFFFFFFFFFFFFFFFFFFFFFFFF1FFFFFFFFFFFFFFFFFFFFFFFF 0191",true,40)),
         "can't set SMSC");
  return true;
}

void writeUSIMproprietary(USIM &card, struct uicc_vals &values) {
  if ( values.key.size() > 0)
    // Ki files and Milenage algo parameters are specific to the card manufacturer
    Assert(card.writeFile("GR Ki", card.encodeKi(values.key)),
           "can't set Ki %s",values.key.c_str());

  if (values.opc.size() > 0)
    Assert(card.writeFile("GR OPc", card.encodeOPC(values.opc)),
           "can't set OPc %s",values.opc.c_str());

  //Milenage internal paramters
  card.writeFile("GR R",makeBcdVect("4000204060",false));
  vector<string> C;
  C.push_back(makeBcd("00000000000000000000000000000000",false));
  C.push_back(makeBcd("00000000000000000000000000000001",false));
  C.push_back(makeBcd("00000000000000000000000000000002",false));
  C.push_back(makeBcd("00000000000000000000000000000004",false));
  C.push_back(makeBcd("00000000000000000000000000000008",false));
  card.writeFile("GR C",C);
}

bool writeUSIMvalues(char *port, struct uicc_vals &values) {
  vector<string> res;
  USIM card;
  string ATR;
  Assert((ATR=card.open(port))!="", "Failed to open %s", port);

  if (!card.verifyChv('\x0a', values.adm)) {
    printf("chv 0a Nok\n");
    return false;
  }

  writeUSIMproprietary(card,values);
  vector<string> li;
  li.push_back("en");
  Assert(card.writeFile("language preference", li), "can't set language");
  vector<string> ad;
  ad.push_back(makeBcd("810000",false));
  ad[0]+=(char) values.mncLen;
  Assert(card.writeFile("Administrative data", ad),
         "can't set Administrative data");

  Assert(card.writeFile("Short Message Service Parameters", makeBcdVect("FFFFFFFFFFFFFFFFFFFFFFFFFFF1FFFFFFFFFFFFFFFFFFFFFFFF 0191",true,40)),
         "can't set SMSC");

  if (values.isdn.size() > 0)
    Assert(card.writeFile("MSISDN", card.encodeISDN(values.isdn, card.fileRecordSize("MSISDN"))),
           "can't set msisdn %s",values.isdn.c_str());

  if ( values.acc.size() > 0)
    Assert(card.writeFile("Access control class", card.encodeACC(values.acc)),
           "can't set acc %s",values.acc.c_str());

  if ( values.imsi.size() > 0) {
    Assert(card.writeFile("IMSI", card.encodeIMSI(values.imsi)),
           "can't set imsi %s",values.imsi.c_str());
    string MccMnc=card.encodeMccMnc(values.imsi.substr(0,3),
                                    values.imsi.substr(3,values.mncLen));
    vector<string> VectMccMnc;
    VectMccMnc.push_back(MccMnc);
    vector<string> MccMncWithAct=VectMccMnc;
    // Add EUTRAN access techno only
    // add NG-RAN and satellite NG-RAN
    MccMncWithAct[0]+=string(u8"\x7c\x00",2);
    Assert(card.writeFile("PLMN selector with Access Technology",
                          MccMncWithAct, true), "Can't write PLMN Selector");
    Assert(card.writeFile("Operator controlled PLMN selector with Access Technology",
                          MccMncWithAct, true), "Can't write Operator PLMN Selector");
    Assert(card.writeFile("Home PLMN selector with Access Technology",
                          MccMncWithAct, true), "Can't write home  PLMN Selector");
    Assert(card.writeFile("Equivalent Home PLMN",
                          VectMccMnc), "Can't write Equivalent PLMN");
    vector<string> psloci;
    psloci.push_back(makeBcd("",true,7));
    psloci[0]+=MccMnc;

    if (values.mncLen == 3 )
      psloci[0]+=makeBcd("00ff01", false);
    else
      psloci[0]+=makeBcd("0000ff01", false);

    Assert(card.writeFile("PS Location information",
                          psloci,false),
           "PS location information");
    vector<string> csloci;
    csloci.push_back(makeBcd("",true,4));
    csloci[0]+=MccMnc;

    if (values.mncLen == 3 )
      csloci[0]+=makeBcd("00ff01", false);
    else
      csloci[0]+=makeBcd("0000ff01", false);

    Assert(card.writeFile("CS Location information",
                          csloci, false),
           "CS location information");
  }

  vector<string> spn;
  spn.push_back(string(u8"\x01",1));
  spn[0]+=values.spn;
  Assert(card.writeFile("Service Provider Name", spn, true), "can't set spn");
  Assert(card.writeFile("Higher Priority PLMN search period", makeBcdVect("02", false)), "can't set plmn search period");
  Assert(card.writeFile("Forbidden PLMNs", makeBcdVect("",true,12)), "can't set forbidden plmn");
  Assert(card.writeFile("Group Identifier Level 1", makeBcdVect("",true,4)), "can't set GID1");
  Assert(card.writeFile("Group Identifier Level 2", makeBcdVect("",true,4)), "can't set GID2");
  vector<string> ecc;
  ecc.push_back(makeBcd("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",false));
  ecc.push_back(makeBcd("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",false));
  ecc.push_back(makeBcd("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",false));
  ecc.push_back(makeBcd("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",false));
  Assert(card.writeFile("emergency call codes", ecc), "can't set emergency call codes");
  // Typical service list, a bit complex to define (see 3GPP TS 51.011)
  Assert(card.writeFile("USIM service table", makeBcdVect(values.ust, false)),
         "can't set USIM service table");
  return true;
}

void setOPc(struct uicc_vals &values) {
  string key=hexa(values.key);
  Assert( key.size()  == 16, "can't read a correct key: 16 hexa figures\n");
  string op=hexa(values.op);
  Assert(op.size() == 16, "can't read a correct op: 16 hexa figures\n");
  uint8_t opc[16];
  milenage_opc_gen((const uint8_t *)key.c_str(),
                   (const uint8_t *)op.c_str(),
                   opc);

  for (int i=0 ; i<16; i++) {
    char tmp[8];
    sprintf(tmp,"%02hhx",opc[i]);
    values.opc+= tmp;
  }
}

vector<string> oneAuthentication(USIM &USIMcard,
                                 string &opc, string &key,
                                 uint64_t intSqn, u8 *rand, u8 *amf, u8 *autn, u8 *ik, u8 *ck, u8 *res) {
  union {
    u8 bytes[8];
    uint64_t ll;
  }  simSqn = {0};
  simSqn.ll=htobe64(intSqn);
  Assert(milenage_generate((const uint8_t *)opc.c_str(), amf,
                           (const uint8_t *)key.c_str(), &simSqn.bytes[2],
                           rand,
                           autn, ik, ck, res),
         "Milenage internal failure\n");
  return USIMcard.authenticate(string((char *)rand,16), string((char *)autn,16));
}

void fillRand(u8 *out, int size) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand(tv.tv_usec);

  for (int i=0; i < size; i++)
    out[i]=(u8)(rand()&0xFF);
}

void testAUTN(struct uicc_vals &values) {
  string key=hexa(values.key);
  string opc=hexa(values.opc);

  if (key.size()!= 16 || opc.size()!= 16) {
    printf("Authenticate test require to have key (Ki) and OP or OPc\n");
    return;
  }

  u8 amf[2]= {0x80, 0x00};
  u8 rand[16]= {0};
  memcpy(rand, hexa(values.rand).c_str(), sizeof(rand));
  u8 autn[16]= {0};
  u8 ik[16]= {0};
  u8 ck[16]= {0};
  u8 res[8]= {0};
  uint64_t intSqn=atoi(values.sqn.c_str());
  union {
    u8 bytes[8];
    uint64_t ll;
  }  simSqn = {0};
  simSqn.ll=htobe64(intSqn);
  Assert(milenage_generate((const uint8_t *)opc.c_str(), amf,
                           (const uint8_t *)key.c_str(), &simSqn.bytes[2],
                           rand,
                           autn, ik, ck, res),
         "Milenage internal failure\n");
  cout << "AUTN:";

  for (size_t i=0; i<sizeof(autn) ; i++)
    printf("%02x",autn[i]);

  cout <<endl;
}


void authenticate(char *port, struct uicc_vals &values) {
  string key=hexa(values.key);
  string opc=hexa(values.opc);

  if (key.size()!= 16 || opc.size()!= 16) {
    printf("Authenticate test require to have key (Ki) and OP or OPc\n");
    return;
  }

  USIM USIMcard;
  string ATR;
  Assert((ATR=USIMcard.open(port))!="", "Failed to open %s", port);
  //dump_hex("ATR", ATR);
  USIMcard.openUSIM();
  //USIMcard.debug=false;
  // We don't make proper values for rand, sqn,
  // we perform first authentication only to get the AUTS from the USIM
  u8 amf[2]= {0x80, 0x00};
  u8 rand[16]= {0};
  u8 autn[16]= {0};
  u8 ik[16]= {0};
  u8 ck[16]= {0};
  u8 res[8]= {0};
  uint64_t intSqn=0;
  u8 autsSQN[8]= {0};
  fillRand(rand, sizeof(rand));
  //memset(rand, 0x11, sizeof(rand));
  vector<string> firstCall=oneAuthentication(USIMcard,
                           opc, key,
                           intSqn, rand, amf, autn, ik, ck, res);

  // We should have one LV value returned, the AUTS (or AUTN)
  if (firstCall.size()!=1) {
    printf("The card didn't accept our challenge: OPc or Ki is wrong\n");
    return;
  }

  if ( ! milenage_auts((const uint8_t *)opc.c_str(),
                       (const uint8_t *)key.c_str(),
                       rand,
                       (const uint8_t *)firstCall[0].c_str(),
                       &autsSQN[2]) )
    printf("Warning in AUTS (card serial OC004000 to OC004110, call support), let's check the SQN anyway\n");

  intSqn=be64toh(*(uint64_t *)autsSQN);
  // here we should have the current sqn in the UICC
  intSqn+=32; // according to 3GPP TS 33.102 version 11, annex C. 3.2
  // To make better validation, let's generate a random value in milenage "rand"
  fillRand(rand,sizeof(rand));
  vector<string> returned_newSQN=oneAuthentication(USIMcard,
                                 opc, key,
                                 intSqn, rand, amf, autn, ik, ck, res);

  if (returned_newSQN.size() != 4)
    printf("We tried SQN %" PRId64 ", but the card refused!\n",intSqn);
  else {
    string s_ik((char *)ik,sizeof(ik));
    string s_ck((char *)ck,sizeof(ck));
    string s_res((char *)res,sizeof(res));

    if ( s_res != returned_newSQN[0] ||
         s_ck  != returned_newSQN[1] ||
         s_ik  != returned_newSQN[2] )
      printf("The card sent back vectors, but they are not our milenage computation\n");
    else {
      printf("Succeeded to authentify with SQN: %" PRId64 "\n", intSqn);
      printf("set HSS SQN value as: %" PRId64 "\n", intSqn+32 );
    }
  }
}

int main(int argc, char **argv) {
  char portName[FILENAME_MAX+1] = "/dev/ttyUSB0";
  struct uicc_vals new_vals;
  bool readAfter=true;
  static struct option long_options[] = {
    {"port",  required_argument, 0, 0},
    {"adm",   required_argument, 0, 1},
    {"iccid", required_argument, 0, 2},
    {"imsi",  required_argument, 0, 3},
    {"opc",   required_argument, 0, 4},
    {"isdn",  required_argument, 0, 5},
    {"acc",   required_argument, 0, 6},
    {"key",   required_argument, 0, 7},
    {"MNCsize", required_argument, 0, 8},
    {"xx",    required_argument, 0, 9},
    {"authenticate",  no_argument, 0, 10},
    {"spn", required_argument, 0, 11},
    {"noreadafter", no_argument, 0, 12},
    {"ust", required_argument, 0, 13},
    {"sqn", required_argument, 0, 14},
    {"rand", required_argument, 0, 15},
    {0,       0,                 0, 0}
  };
  static map<string,string> help_text= {
    {"port",  "Linux port to access the card reader (/dev/ttyUSB0)"},
    {"adm",   "The ADM code of the card (the master password)"},
    {"iccid", "the UICC id to set"},
    {"imsi",  "The imsi to set, we automatically set complementary files such as \"home PLMN\""},
    {"opc",   "OPc field: OPerator code: must be also set in HSS (exlusive with OP)"},
    {"isdn",  "The mobile phone number (not used in simple 4G)"},
    {"acc",   "One of the defined security codes"},
    {"key",   "The authentication key (called Ki in 3G/4G, Kc in GSM), must be the same in HSS"},
    {"MNCsize","Mobile network code size in digits (default to 2)"},
    {"xx",    "OP  field: OPerator code: must be also set in HSS (exclusive with OPc)"},
    {"spn",   "service provider name: the name that the UE will show as 'network'"},
    {"authenticate",  "Test the milenage authentication and discover the current sequence number"},
    {"noreadafter", "no read after write"},
    {"ust", "usim service table in hexa decimal (first byte is services 1-8, so 81 enable service 1 and service 8 ...)"},
    {"sqn", "only for test, no UICC dialog, prints the generated AUTN for debugging"},
    {"rand", "only for test, no UICC dialog, prints the generated AUTN for debugging"},
  };
  setbuf(stdout, NULL);
  int c;
  bool correctOpt=true;

  while (correctOpt) {
    int option_index = 0;
    c = getopt_long_only(argc, argv, "",
                         long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        strncpy(portName, optarg, FILENAME_MAX);
        break;

      case 1:
        new_vals.adm=optarg;
        new_vals.setIt=true;
        break;

      case 2:
        new_vals.iccid=optarg;
        break;

      case 3:
        new_vals.imsi=optarg;
        break;

      case 4:
        new_vals.opc=optarg;
        break;

      case 5:
        new_vals.isdn=optarg;
        break;

      case 6:
        new_vals.acc=optarg;
        break;

      case 7:
        new_vals.key=optarg;
        break;

      case 8:
        new_vals.mncLen=atoi(optarg);
        break;

      case 9:
        new_vals.op=optarg;
        break;

      case 10:
        new_vals.authenticate=true;
        break;

      case 11:
        new_vals.spn=optarg;
        break;

      case 12:
        readAfter=false;
        break;

      case 13:
        new_vals.ust=optarg;
        break;

      case 14:
        new_vals.sqn=optarg;
        break;

      case 15:
        new_vals.rand=optarg;
        break;

      default:
        printf("unrecognized option: %d \n", c);
        correctOpt=false;
    };
  }

  if (optind < argc ||  correctOpt==false) {
    printf("non-option ARGV-elements: ");

    while (optind < argc)
      printf("%s ", argv[optind++]);

    printf("Possible options are:\n");

    for (int i=0; long_options[i].name!=NULL; i++)
      printf("  --%-10s %s\n",long_options[i].name, help_text[long_options[i].name].c_str());

    printf("\n");
    exit(1);
  }

  if ( new_vals.op != "") {
    setOPc(new_vals);
    printf("Computed OPc from OP and Ki as: %s\n", new_vals.opc.c_str());
  }

  if (new_vals.sqn.size() && new_vals.rand.size() ) {
    testAUTN(new_vals);
    exit(0);
  }

  int cardVersion=0;
  printf ("\nExisting values in USIM\n");
  Assert(cardVersion=readUSIMvalues(portName), "failed to read UICC");

  if ( new_vals.adm.size() ==16 ) // adm in hexa, convert it to bytes
    new_vals.adm=makeBcd(new_vals.adm);

  if ( new_vals.adm.size() != 8 ) { // must be 8 bytes
    printf ("\nNo ADM code of 8 figures, can't program the UICC\n");
    readAfter=false;
  } else {
    printf("\nSetting new values\n");

    switch (cardVersion) {
      case 1:
        writeSIMvalues(portName, new_vals);
        writeUSIMvalues(portName, new_vals);
        break;

      case 2:
        writeSIMv2values(portName, new_vals);
        break;

      default:
        printf("\nUnknown UICC type\n");
        exit(1);
    }
  }

  if ( readAfter ) {
    printf("\nReading UICC values after uploading new values\n");
    readUSIMvalues(portName);
  }

  if ( new_vals.authenticate) {
    if ( new_vals.opc.size() == 0 || new_vals.key.size() == 0)
      printf("\nNeed the key (Ki) and the OPc to test Milenage and dispaly the SQN\n");
    else
      authenticate(portName, new_vals);
  }

  return 0;
}
