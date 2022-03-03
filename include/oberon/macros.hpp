#ifndef OBERON_MACROS_HPP
#define OBERON_MACROS_HPP

#define OBERON_API_VERSION_V0_0 20220221ULL

#if !defined(OBERON_API_VERSION)
  #define OBERON_API_VERSION OBERON_API_VERSION_V0_0
#endif

#if OBERON_API_VERSION == OBERON_API_VERSION_V0_0
  #define OBERON_INLINE_V0_0 inline
#else
  #define OBERON_INLINE_V0_0
#endif

#define OBERON_INTERFACE(interface, class) \
  static_assert(requires { requires interface<class>; })

#endif
