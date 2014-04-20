// TypeTraits.h
// Copyright(c) 2005, 2006, David R. Cheriton, all rights reserved.

#ifndef FWK_TYPETRAITS_H
#define FWK_TYPETRAITS_H

namespace Fwk {

struct True { };
struct False { };

template< typename T > struct IsInteger { typedef False Value; };
template<> struct IsInteger< bool > { typedef True Value; };
template<> struct IsInteger< signed char > { typedef True Value; };
template<> struct IsInteger< signed short > { typedef True Value; };
template<> struct IsInteger< signed int > { typedef True Value; };
template<> struct IsInteger< signed long > { typedef True Value; };
template<> struct IsInteger< signed long long > { typedef True Value; };
template<> struct IsInteger< unsigned char > { typedef True Value; };
template<> struct IsInteger< unsigned short > { typedef True Value; };
template<> struct IsInteger< unsigned int > { typedef True Value; };
template<> struct IsInteger< unsigned long > { typedef True Value; };
template<> struct IsInteger< unsigned long long > { typedef True Value; };

template< typename T > struct IsSignedInteger { typedef False Value; };
template<> struct IsSignedInteger< signed char > { typedef True Value; };
template<> struct IsSignedInteger< signed short > { typedef True Value; };
template<> struct IsSignedInteger< signed int > { typedef True Value; };
template<> struct IsSignedInteger< signed long > { typedef True Value; };
template<> struct IsSignedInteger< signed long long > { typedef True Value; };

template< typename T > struct IsUnsignedInteger { typedef False Value; };
template<> struct IsUnsignedInteger< bool > { typedef True Value; };
template<> struct IsUnsignedInteger< unsigned char > { typedef True Value; };
template<> struct IsUnsignedInteger< unsigned short > { typedef True Value; };
template<> struct IsUnsignedInteger< unsigned int > { typedef True Value; };
template<> struct IsUnsignedInteger< unsigned long > { typedef True Value; };
template<> struct IsUnsignedInteger< unsigned long long > { typedef True Value; };

template< typename T > struct IsFloat { typedef False Value; };
template<> struct IsFloat< float > { typedef True Value; };
template<> struct IsFloat< double > { typedef True Value; };
template<> struct IsFloat< long double > { typedef True Value; };

template< typename T > struct IsPointer { typedef False Value; };
template< typename T > struct IsPointer< T * > { typedef True Value; };

// A "POD" type is one that does not initialize itself, and should be
// memset to 0 to initialize properly.  This IsPod doesn't really work
// very well --- it only works for singleton things, not for
// aggregates.  Boost has one that works, except I can't understand
// it.
template< typename T > struct IsPod { typedef False Value; };
template<> struct IsPod< bool > { typedef True Value; };
template<> struct IsPod< signed char > { typedef True Value; };
template<> struct IsPod< signed short > { typedef True Value; };
template<> struct IsPod< signed int > { typedef True Value; };
template<> struct IsPod< signed long > { typedef True Value; };
template<> struct IsPod< signed long long > { typedef True Value; };
template<> struct IsPod< unsigned char > { typedef True Value; };
template<> struct IsPod< unsigned short > { typedef True Value; };
template<> struct IsPod< unsigned int > { typedef True Value; };
template<> struct IsPod< unsigned long > { typedef True Value; };
template<> struct IsPod< unsigned long long > { typedef True Value; };
template<> struct IsPod< float > { typedef True Value; };
template<> struct IsPod< double > { typedef True Value; };
template<> struct IsPod< long double > { typedef True Value; };
template< typename T > struct IsPod< T * > { typedef True Value; };


};


#endif // FWK_TYPETRAITS_H
