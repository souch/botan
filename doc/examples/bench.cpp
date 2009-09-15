#include <botan/benchmark.h>
#include <botan/init.h>
#include <botan/auto_rng.h>
#include <botan/libstate.h>

using namespace Botan;

#include <iostream>

const std::string algos[] = {
   "AES-128",
   "AES-192",
   "AES-256",
   "Blowfish",
   "CAST-128",
   "CAST-256",
   "DES",
   "DESX",
   "TripleDES",
   "GOST",
   "IDEA",
   "KASUMI",
   "Lion(SHA-256,Turing,8192)",
   "Luby-Rackoff(SHA-512)",
   "MARS",
   "MISTY1",
   "Noekeon",
   "RC2",
   "RC5(12)",
   "RC5(16)",
   "RC6",
   "SAFER-SK(10)",
   "SEED",
   "Serpent",
   "Skipjack",
   "Square",
   "TEA",
   "Twofish",
   "XTEA",
   "Adler32",
   "CRC32",
   "FORK-256",
   "GOST-34.11",
   "HAS-160",
   "MD2",
   "MD4",
   "MD5",
   "RIPEMD-128",
   "RIPEMD-160",
   "SHA-160",
   "SHA-256",
   "SHA-384",
   "SHA-512",
   "Skein-512",
   "Tiger",
   "Whirlpool",
   "CMAC(AES-128)",
   "HMAC(SHA-1)",
   "X9.19-MAC",
   "",
};

int main()
   {
   LibraryInitializer init;

   u32bit milliseconds = 1000;
   AutoSeeded_RNG rng;
   Default_Benchmark_Timer timer;

   Algorithm_Factory& af = global_state().algorithm_factory();

   std::vector<std::string> providers = af.providers_of("Serpent");
   for(size_t i = 0; i != providers.size(); ++i)
      std::cout << providers[i].c_str() << "\n";



   for(u32bit i = 0; algos[i] != ""; ++i)
      {
      std::string algo = algos[i];

      std::map<std::string, double> speeds =
         algorithm_benchmark(algos[i], milliseconds, timer, rng, af);

      std::cout << algo << ":";

      for(std::map<std::string, double>::const_iterator i = speeds.begin();
          i != speeds.end(); ++i)
         {
         std::cout << " " << i->second << " [" << i->first << "]";
         }
      std::cout << "\n";
      }
   }
