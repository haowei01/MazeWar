// Array.h
// Copyright (c) 2005, 2006 David R. Cheriton.  All rights reserved.

#ifndef FWK_ARRAY_H
#define FWK_ARRAY_H

// We use fixed-size built-in array types for arrays, but we need an
// iterator.

#include <fwk/Types.h>
#include <fwk/BaseCollection.h>
#include <fwk/TypeTraits.h>

namespace Fwk {

inline void
zeroIfTrue( True, void * ptr, size_t size ) {
   memset( ptr, 0, size );
}

inline void
zeroIfTrue( False, void * ptr, size_t size ) {
}

template< typename T, U32 N, typename K=U32 >
class Array : public BaseRefCollection<T> {
 public:
   Array() { zeroIfTrue( typename IsPod<T>::Value(), array_, sizeof( array_ )); }
   enum { maxIndex = N };
   T const & operator[]( unsigned n ) const { return array_[ n ]; }
   T & operator[]( unsigned n ) { return array_[ n ]; }
   bool operator==( Array< T, N, K > const & other ) const {
      for( unsigned i = 0; i < N; i++ ) {
         if( array_[i] != other[i] ) {
            return false;
         }
      }
      return true;
   }
   bool operator!=( Array< T, N, K > const & other ) const { return !(*this == other); } 
   
   class IteratorConst : public BaseIteratorConst<T> {
   public:
      IteratorConst( Array< T, N, K > const & array, U32 start = 0 ) : 
         BaseIteratorConst< T >( &array, &array[start] ) {}
      using BaseIteratorConst< T >::space_;
      typedef typename BaseIteratorConst< T >::BoolConversion BoolConversion;
      operator int BoolConversion::*() const {
         return space_ ? &BoolConversion::x : NULL; 
      }
      T const * ptr() const { return BaseIteratorConst<T>::ptr(); }
      // We cannot implement a fast (non-virtual) version of "++" because
      // we have no place to store "N", so we can't tell if we hit the end.
      // We have to rely on the virtual function Array<T,N,K>::iteratorIncr()
      // to know what "N" is.
      T const & operator*() const { return *ptr(); }

      // Should be used only by Array<T,N,K>.  I can't see any way to declare 
      // Array<T,N,K> a friend.  -kduda 2005-04-20
      void privatePtrIs( T const * p ) { ptrIs( p ); }
   };
   class Iterator : public IteratorConst {
      using IteratorConst::space_;
    public:
      Iterator( Array<T,N,K> const & array, U32 start = 0 ) : 
         IteratorConst( array, start ) {}
      T & operator *() { return * const_cast<T *>( space_ ); }
      Iterator & operator++() {
         this->IteratorConst::operator++(); return *this;
      }
   };
   IteratorConst iterator() const {
      return IteratorConst(this,0);
   }

   private:
      T array_[ N ];
   typedef BaseCollection::StrepIterator StrepIterator;
   virtual bool iteratorMoreLeft( StrepIterator const & ) const;
   virtual void iteratorIncr( StrepIterator& ) const;
   virtual String iteratorStrep( StrepIterator const &) const;
};

template< typename T, typename K=U32 > 
class ArrayIteratorConst : public BaseIteratorConst< T > {
 public:
   template< U32 N > ArrayIteratorConst( Array< T, N, K > const & array, U32 start = 0 ) : 
         BaseIteratorConst< T >( &array, &array[start] ) {}
   using BaseIteratorConst< T >::space_;
   typedef typename BaseIteratorConst< T >::BoolConversion BoolConversion;
   operator int BoolConversion::*() const {
      return space_ ? &BoolConversion::x : NULL; 
   }
   T const * ptr() const { return BaseIteratorConst<T>::ptr(); }
   // We cannot implement a fast (non-virtual) version of "++" because
   // we have no place to store "N", so we can't tell if we hit the end.
   // We have to rely on the virtual function Array<T,N,K>::iteratorIncr()
   // to know what "N" is.
   T const & operator*() const { return *ptr(); }

   // Should be used only by Array<T,N,K>.  I can't see any way to declare 
   // Array<T,N,K> a friend.  -kduda 2005-04-20
   void privatePtrIs( T const * p ) { ptrIs( p ); }
};

template< typename T, U32 N, typename K >
bool Array< T, N, K >::iteratorMoreLeft( BaseCollection::StrepIterator const & iter ) const {
   ArrayIteratorConst<T,K> const * i = (ArrayIteratorConst<T,K> const *) &iter;
   return *i ? true : false;
}

template< typename T, U32 N, typename K >
void Array< T, N, K >::iteratorIncr( BaseCollection::StrepIterator & iter ) const {
   ArrayIteratorConst<T,K> * i = (ArrayIteratorConst<T,K> *) &iter;
   T const * p = i->ptr();
   ++p;
   U32 idx = p - array_;
   if( idx == N ) {
      i->privatePtrIs( NULL );
   } else {
      assert( idx < N );
      i->privatePtrIs( p );
   }
}

}

//#include <fwk/SerDes.h>

namespace Fwk {

template< typename T, U32 N, typename K >
String Array< T, N, K >::iteratorStrep( BaseCollection::StrepIterator const & iter ) const {
   ArrayIteratorConst<T,K> const * i = (ArrayIteratorConst<T,K> const *) &iter;
   T const * p = i->ptr();
   U32 idx = p - array_;
   assert( idx < N );
   // WARNING!  The below assumes that the array is zero based.  That
   // is, it assumes that K(0) returns the key for the first element
   // of the array.  For arrays of enums, this will be the case only
   // if the first enumerator has a value of 0.
   return valueToStrep( K( idx ));
}


template< typename T, typename K=U32 > 
class ArrayIterator : public ArrayIteratorConst< T, K > {
   using ArrayIteratorConst<T,K>::space_;
 public:
   template< U32 N > ArrayIterator( Array<T,N,K> const & array, U32 start = 0 ) : 
         ArrayIteratorConst<T,K>( array, start ) {}
   T & operator *() { return * const_cast<T *>( space_ ); }
   ArrayIterator & operator++() { this->ArrayIteratorConst<T,K>::operator++(); return *this; }
};

}

#endif // FWK_ARRAY_H
