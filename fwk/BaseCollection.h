// BaseCollection - base class for type-specific collections
// Copyright(c) 2003-2006, David R. Cheriton, all rights reserved.

#ifndef FWK_BASECOLLECTION_H
#define FWK_BASECOLLECTION_H

#include <fwk/Types.h>
#include <fwk/Ptr.h>
#include <fwk/PtrInterface.h>

namespace Fwk {

class BaseCollection {
public:

   virtual ~BaseCollection();

   class StrepIterator {
    public:
      ~StrepIterator() { if( collection_ ) collection_->iteratorDelete( *this ); }
      StrepIterator( StrepIterator const & other ) :
            collection_( other.collection_ ), space_( 
                  other.collection_ ? other.collection_->iteratorSpaceCopy( other ) : 
                  other.space_ ), data0_(other.data0_), data1_(other.data1_) {}
      StrepIterator const & operator=( StrepIterator const & other ) {
         if ( collection_ ) {
            collection_->iteratorDelete( *this );
         }

         collection_ = other.collection_;
         if ( other.collection_ ) {
            space_ = other.collection_->iteratorSpaceCopy( other );
         } else {
            space_ = other.space_;
         }
         data0_ = other.data0_;
         data1_ = other.data1_;
         return *this;
      }
      struct BoolConversion { int x; }; // better than operator bool
      operator int BoolConversion::*() const {
         return collection_->iteratorMoreLeft( *this ) ? &BoolConversion::x : NULL; 
      }
      StrepIterator& operator++() {
         collection_->iteratorIncr( *this );
         return *this;
      }
      String strep() const { 
         return collection_->iteratorStrep( *this ); 
      }
    protected:
      StrepIterator( BaseCollection const * col, void const * space ) :
            collection_( col ), space_( space ) {
         // Derived class is responsible for incrementing the refcount on
         // space_ if needed
      }
      StrepIterator() : collection_(0), space_(0) { }
      friend class BaseCollection;
      BaseCollection const * collection_;
      void const * space_;
      U32 data0_;
      U32 data1_;
   };
 protected:
   void const * space( StrepIterator const & it  ) const { return it.space_; }

 private:
   friend class StrepIterator;
   virtual bool iteratorMoreLeft( StrepIterator const & ) const;
   virtual void iteratorIncr( StrepIterator& ) const;
   virtual void iteratorDelete( StrepIterator& ) const;
   virtual String iteratorStrep( StrepIterator const &) const;
   virtual void const * iteratorSpaceCopy( StrepIterator const & ) const;
};

inline void _newRef( void const volatile *  t ) {}
inline void _deleteRef( void const volatile *  t ) {}
template<class T> inline void _newRef( PtrInterface<T> const *  t ) { t->newRef(); }
template<class T> inline void _deleteRef( PtrInterface<T> const *  t ) { t->deleteRef(); }
// _newRef and _deleteRef update the refcount for PtrInterface types,
// and do nothing for other types.  NOTE: these declarations are a
// little bit fragile.  They are carefully designed, taking into
// account the C++ overload resolution rules, to ensure that when
// passed a pointer to an object deriving from PtrInterface<T>, that
// they will select the second (PtrInterface<T> const *) function
// rather than the first (void const volatile *) version.  So take
// care if you change these.
// The _newRef overload is used below rather than calling
// t->newRef() directly. This allows us to write one collection class
// implementation that handles both refcounted and non-refcounted
// pointers.

template<typename T>
class BaseRefCollection : public BaseCollection {
 public:
   virtual void const * iteratorSpaceCopy( StrepIterator const & it ) const { 
      T const * t = static_cast<T const *>( space( it ) );
      if( t ) { _newRef( t ); }
      return t;
   }
   virtual void iteratorDelete( StrepIterator & it ) const { 
      T const * t = static_cast<T const *>( space( it ) );
      if( t ) { _deleteRef( t ); }
   }
};
// BaseRefCollection is intended to be used when a collection might
// store a pointer to TT, derived from PtrInterface<T>, and we want
// the iterator to hold a refcount to the pointed-to object in that
// case.
//
// This class provides implementations of
// iteratorDelete and iteratorSpaceCopy that call newRef and deleteRef on
// the underlying ptr.  Normally, if a concrete collection class (like
// HashMap<T>) derives from BaseRefCollection, and its iterator
// derives from BaseIteratorConst<T>, then that is sufficient to keep
// a reference to the underlying pointer.  A derived class may, in
// some cases, need to provide its own reference count management (see
// OrderedMap, for instance), and in that case would not derive from
// BaseRefCollection.


template<typename T, bool refcount=true>
class BaseIteratorConst : public BaseCollection::StrepIterator {
public:
   BaseIteratorConst( const BaseCollection * col, T const * t ) :
         BaseCollection::StrepIterator( col, (void *) t ) {
       if( refcount && ptr() ) { _newRef( ptr() ); }
    }
   bool operator==( const BaseIteratorConst& mp ) const { 
      return (mp.ptr() == ptr()) && (mp.collection_ == collection_);
   }
   bool operator!=(const BaseIteratorConst& mp ) const {
      return !((mp.ptr() == ptr()) && (mp.collection_ == collection_));
   }
   BaseIteratorConst<T>& operator=( const BaseIteratorConst<T>& mp ) {
      collection_ = mp.collection_;
      ptrIs( mp.ptr() );
      data0_ = mp.data0_;
      data1_ = mp.data1_;
      return *this;
   }
   operator int BoolConversion::*() const { return space_ ? &BoolConversion::x : NULL; }

 protected:
   T const * ptr() const { return (T const *) space_; }
   void ptrIs( T const * t ) { 
      if( refcount && t ) _newRef( t );
      if( refcount && ptr() ) { _deleteRef( ptr() ); }
      space_ = (void *) t;
   }
};
// BaseIteratorConst<T> adds useful functionality to
// BaseCollection::StrepIterator and is used in the implementation of
// most collection classes.  The refcount parameter is true if we want
// this iterator implementation to refcount the underlying object that
// the iterator points to, (assuming it derives from PtrInterface<T>).


template <class Collection >
void orphanAllMembers( Collection & c ) {
   typename Collection::Iterator i( c.iterator() );
   for( ; i; ++i ) {
      i->parentIs(0);
   }
}
// quasi-private helper function used by tacc generated code. It
// zeroes out the parent attributes of all collection members, right
// before a collection of entities is emptied.

};

#endif
