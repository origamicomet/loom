//===-- loom/prng.c -------------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#include "loom/prng.h"

#include "loom/support.h"

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  #include <windows.h>
  #include <wincrypt.h>
#endif

LOOM_BEGIN_EXTERN_C

struct loom_prng {
  loom_uint32_t state;
};

loom_prng_t *loom_prng_create(void) {
  loom_prng_t *prng =
    (loom_prng_t *)calloc(1, sizeof(loom_prng_t));

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  HCRYPTPROV csp;
  CryptAcquireContext(&csp, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  CryptGenRandom(csp, sizeof(prng->state), (BYTE *)&prng->state);
  CryptReleaseContext(csp, 0);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  // TODO(mtwilliams): Seed from `/dev/urandom`.
#endif

  return prng;
}

void loom_prng_destroy(loom_prng_t *prng) {
  loom_assert_debug(prng != NULL);
  free((void *)prng);
}

loom_uint32_t loom_prng_grab_u32(loom_prng_t *prng) {
  loom_assert_debug(prng != NULL);

  // Dead simple xorshift.
  loom_uint32_t x = prng->state;
  
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  
  prng->state = x;
  
  return x;
}

LOOM_END_EXTERN_C
