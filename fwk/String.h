// Copyright (c) 1993-2007 David R. Cheriton, all rights reserved.

#ifndef FWK_STRING_H
#define FWK_STRING_H

#include <stdarg.h>
#include <string>
#include <fwk/Types.h>

namespace Fwk {

inline U32 
hash( String const & s )
{
	U32 h = 0;
	for (String::size_type i = 0; i < s.size(); ++i)
		h = 5 * h + s[i];
	return h;
}

class StringBuf {
 public:
   template< typename T >
   inline StringBuf & operator<<( const T & t ) {
      ss_ << t;
      return *this;
   }
   operator String() const { return ss_.str(); }
 private:
   std::stringstream ss_;
};

}

#endif
