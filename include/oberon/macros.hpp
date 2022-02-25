#ifndef OBERON_MACROS_HPP
#define OBERON_MACROS_HPP

#define OBERON_API_VERSION_V_0_0 20220221ULL

#if !defined(OBERON_API_VERSION)
  #define OBERON_API_VERSION OBERON_API_VERSION_V_0_0
#endif

#if OBERON_API_VERSION == OBERON_API_VERSION_V_0_0
  #define OBERON_INLINE_V_0_0 inline
#else
  #define OBERON_INLINE_V_0_0
#endif

#endif
