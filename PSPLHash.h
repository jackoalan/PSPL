//
//  PSPLHash.h
//  PSPL
//
//  Created by Jack Andersen on 5/11/13.
//
//

#ifndef PSPL_PSPLHash_h
#define PSPL_PSPLHash_h

#if defined(PSPL_HASHING_BUILTIN)
#  include "Hash/hash_builtin.h"
#elif defined(PSPL_HASHING_COMMON_CRYPTO)
#  include "Hash/hash_commoncrypto.h"
#elif defined(PSPL_HASHING_OPENSSL)
#  include "Hash/hash_openssl.h"
#elif defined(PSPL_HASHING_WINDOWS)
#  include "Hash/hash_windows.h"
#else
#  error No Hashing Library Set
#endif

#endif
