/*
* ECDSA implemenation
* (C) 2007 Manuel Hartl, FlexSecure GmbH
*     2007 Falko Strenzke, FlexSecure GmbH
*     2008-2010,2015,2016,2018 Jack Lloyd
*     2016 René Korthaus
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/ecdsa.h>
#include <botan/internal/pk_ops_impl.h>
#include <botan/keypair.h>
#include <botan/reducer.h>
#include <botan/emsa.h>

#if defined(BOTAN_HAS_RFC6979_GENERATOR)
  #include <botan/rfc6979.h>
#endif

#if defined(BOTAN_HAS_BEARSSL)
  #include <botan/internal/bearssl.h>
#endif

#if defined(BOTAN_HAS_OPENSSL)
  #include <botan/internal/openssl.h>
#endif

namespace Botan {

bool ECDSA_PrivateKey::check_key(RandomNumberGenerator& rng,
                                 bool strong) const
   {
   if(!public_point().on_the_curve())
      return false;

   if(!strong)
      return true;

   return KeyPair::signature_consistency_check(rng, *this, "EMSA1(SHA-256)");
   }

namespace {

/**
* ECDSA signature operation
*/
class ECDSA_Signature_Operation final : public PK_Ops::Signature_with_EMSA
   {
   public:

      ECDSA_Signature_Operation(const ECDSA_PrivateKey& ecdsa,
                                const std::string& emsa) :
         PK_Ops::Signature_with_EMSA(emsa),
         m_group(ecdsa.domain()),
         m_x(ecdsa.private_value())
         {
#if defined(BOTAN_HAS_RFC6979_GENERATOR)
         m_rfc6979_hash = hash_for_emsa(emsa);
#endif
         }

      size_t max_input_bits() const override { return m_group.get_order_bits(); }

      secure_vector<uint8_t> raw_sign(const uint8_t msg[], size_t msg_len,
                                      RandomNumberGenerator& rng) override;

   private:
      const EC_Group m_group;
      const BigInt& m_x;

#if defined(BOTAN_HAS_RFC6979_GENERATOR)
      std::string m_rfc6979_hash;
#endif

      std::vector<BigInt> m_ws;
   };

secure_vector<uint8_t>
ECDSA_Signature_Operation::raw_sign(const uint8_t msg[], size_t msg_len,
                                    RandomNumberGenerator& rng)
   {
   const BigInt m(msg, msg_len);

#if defined(BOTAN_HAS_RFC6979_GENERATOR)
   const BigInt k = generate_rfc6979_nonce(m_x, m_group.get_order(), m, m_rfc6979_hash);
#else
   const BigInt k = BigInt::random_integer(rng, 1, m_group.get_order());
#endif

   const BigInt k_inv = inverse_mod(k, m_group.get_order());
   const PointGFp k_times_P = m_group.blinded_base_point_multiply(k, rng, m_ws);
   const BigInt r = m_group.mod_order(k_times_P.get_affine_x());
   const BigInt s = m_group.multiply_mod_order(k_inv, mul_add(m_x, r, m));

   // With overwhelming probability, a bug rather than actual zero r/s
   if(r.is_zero() || s.is_zero())
      throw Internal_Error("During ECDSA signature generated zero r/s");

   return BigInt::encode_fixed_length_int_pair(r, s, m_group.get_order_bytes());
   }

/**
* ECDSA verification operation
*/
class ECDSA_Verification_Operation final : public PK_Ops::Verification_with_EMSA
   {
   public:
      ECDSA_Verification_Operation(const ECDSA_PublicKey& ecdsa,
                                   const std::string& emsa) :
         PK_Ops::Verification_with_EMSA(emsa),
         m_group(ecdsa.domain()),
         m_public_point(ecdsa.public_point())
         {
         }

      size_t max_input_bits() const override { return m_group.get_order_bits(); }

      bool with_recovery() const override { return false; }

      bool verify(const uint8_t msg[], size_t msg_len,
                  const uint8_t sig[], size_t sig_len) override;
   private:
      const EC_Group m_group;
      const PointGFp& m_public_point;
   };

bool ECDSA_Verification_Operation::verify(const uint8_t msg[], size_t msg_len,
                                          const uint8_t sig[], size_t sig_len)
   {
   if(sig_len != m_group.get_order_bytes() * 2)
      return false;

   const BigInt e(msg, msg_len);

   const BigInt r(sig, sig_len / 2);
   const BigInt s(sig + sig_len / 2, sig_len / 2);

   if(r <= 0 || r >= m_group.get_order() || s <= 0 || s >= m_group.get_order())
      return false;

   const BigInt w = inverse_mod(s, m_group.get_order());

   const BigInt u1 = m_group.multiply_mod_order(e, w);
   const BigInt u2 = m_group.multiply_mod_order(r, w);
   const PointGFp R = m_group.point_multiply(u1, m_public_point, u2);

   if(R.is_zero())
      return false;

   const BigInt v = m_group.mod_order(R.get_affine_x());
   return (v == r);
   }

}

std::unique_ptr<PK_Ops::Verification>
ECDSA_PublicKey::create_verification_op(const std::string& params,
                                        const std::string& provider) const
   {
#if defined(BOTAN_HAS_BEARSSL)
   if(provider == "bearssl" || provider.empty())
      {
      try
         {
         return make_bearssl_ecdsa_ver_op(*this, params);
         }
      catch(Lookup_Error& e)
         {
         if(provider == "bearssl")
            throw;
         }
      }
#endif

#if defined(BOTAN_HAS_OPENSSL)
   if(provider == "openssl" || provider.empty())
      {
      try
         {
         return make_openssl_ecdsa_ver_op(*this, params);
         }
      catch(Lookup_Error& e)
         {
         if(provider == "openssl")
            throw;
         }
      }
#endif

   if(provider == "base" || provider.empty())
      return std::unique_ptr<PK_Ops::Verification>(new ECDSA_Verification_Operation(*this, params));

   throw Provider_Not_Found(algo_name(), provider);
   }

std::unique_ptr<PK_Ops::Signature>
ECDSA_PrivateKey::create_signature_op(RandomNumberGenerator& /*rng*/,
                                      const std::string& params,
                                      const std::string& provider) const
   {
#if defined(BOTAN_HAS_BEARSSL)
   if(provider == "bearssl" || provider.empty())
      {
      try
         {
         return make_bearssl_ecdsa_sig_op(*this, params);
         }
      catch(Lookup_Error& e)
         {
         if(provider == "bearssl")
            throw;
         }
      }
#endif

#if defined(BOTAN_HAS_OPENSSL)
   if(provider == "openssl" || provider.empty())
      {
      try
         {
         return make_openssl_ecdsa_sig_op(*this, params);
         }
      catch(Lookup_Error& e)
         {
         if(provider == "openssl")
            throw;
         }
      }
#endif

   if(provider == "base" || provider.empty())
      return std::unique_ptr<PK_Ops::Signature>(new ECDSA_Signature_Operation(*this, params));

   throw Provider_Not_Found(algo_name(), provider);
   }

}
