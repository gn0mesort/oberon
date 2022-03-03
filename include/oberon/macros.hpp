#ifndef OBERON_MACROS_HPP
#define OBERON_MACROS_HPP

#define OBERON_API_VERSION_V01 20220221ULL

#if !defined(OBERON_API_VERSION)
  #define OBERON_API_VERSION OBERON_API_VERSION_V01
#endif

#define OBERON_INTERFACE(interface, class) \
  static_assert(requires { requires interface<class>; })

#endif
