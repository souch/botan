#include "tests.h"
#include "test_pubkey.h"

#include <botan/auto_rng.h>
#include <botan/pubkey.h>
#include <botan/gost_3410.h>
#include <botan/oids.h>
#include <botan/hex.h>
#include <iostream>
#include <fstream>

using namespace Botan;

namespace {

size_t gost_verify(const std::string& group_id,
                   const std::string& x,
                   const std::string& hash,
                   const std::string& msg,
                   const std::string& signature)
   {
   AutoSeeded_RNG rng;

   EC_Group group(OIDS::lookup(group_id));
   PointGFp public_point = OS2ECP(hex_decode(x), group.get_curve());

   GOST_3410_PublicKey gost(group, public_point);

   const std::string padding = "EMSA1(" + hash + ")";

   PK_Verifier v(gost, padding);

   if(!v.verify_message(hex_decode(msg), hex_decode(signature)))
      return 1;

   return 0;
   }

}

size_t test_gost_3410()
   {
   std::ifstream ecdsa_sig(TEST_DATA_DIR "/gost.vec");

   size_t fails = 0;

   fails += run_tests_bb(ecdsa_sig, "GOST-34.10 Signature", "Signature", true,
             [](std::map<std::string, std::string> m) -> size_t
             {
             return gost_verify(m["Group"], m["Pubkey"], m["Hash"], m["Msg"], m["Signature"]);
             });

   return fails;
   }
