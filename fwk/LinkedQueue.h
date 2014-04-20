// Copyright(c) 1993-2006_2007, David R. Cheriton, all rights reserved.
//
// <h2>LinkedQueue</h2>
// Linked queue of pointers to objects of type T that support smart pointers
// and that provide a "next" attribute (actually lqNext)
// that can be used by this implementation.
// The next attribute must accept and return a raw pointer but store a smart
// pointer of type T.
// Only to be used as an internal implementation data structure
// Use the provided iterators to iterate over entries in a list.
// Insert/delete at the head using "newHead" and "deleteHead"
// Otherwise, use "newCurrent" or "newNext" in an iterator.
// Intended iterator structure in a for loop is:
// for( FooList::IteratorConst i=list_.constIterator(); i; ++i){
//    Foo * f = *i;
//    . . . // do something
//  }
// Use the non-const iterator if adding and deleting entries in the list.
// There is no protection against increments an iterator that is null and
// it should not be done.
// The virtual function onDeleteMemberNotFound is called if an member to be deleted
// was not found in the list.  Default definition does nothing.
// There is no concurrency control provided.  Moreover, a single client should
// not add or delete objects except through an iterator when any iterator is
// in existence.
// Note: removal of an member does not clear the next attribute, but relies on
// the client setting it as part of adding to some other data structure.
// This implementation does not allocate any memory or throw any exceptions.
//

#ifndef FWK_LINKEDQUEUE_H
#define FWK_LINKEDQUEUE_H

#include <fwk/Ptr.h>
#include <fwk/BaseCollection.h>

namespace Fwk {

template< typename T, typename P = T, typename V = T * >
class LinkedQueue : public BaseRefCollection<T> {
 public:
   typedef LinkedQueue< T, P, V > Self;
   LinkedQueue() : version_(0), members_(0),head_(0), tail_(0) {}
   U32 version() const { return version_; }
   U32 members() const { return members_; }
   T * head() const { return head_.ptr(); }
   T * tail() const { return tail_; }
   void newHead( const typename T::Ptr& newMember ) {
      newHead( newMember.ptr() );
   }
   void newHead( T * newMember ) {
      if( !head_ ) tail_ = newMember;
      newMember->lqNextIs( head_.ptr() );
      head_ = newMember;
      ++version_;
      ++members_;
   }
   void newMember( T * _newMember ) {
      if( tail_ ) {
         tail_->lqNextIs(_newMember);
      }
      else head_ = _newMember;
      _newMember->lqNextIs(0);
      tail_ = _newMember;
      ++version_;
      ++members_;
   }
   void newMember( const Ptr<T>& _newMember ) {
      newMember(_newMember.ptr());
   }
   Ptr<T> headDel() {
      Ptr<T> ptr = head_.ptr();
      if( ptr ) {
  	 if( tail_ == ptr ) tail_ = 0;
         head_ = ptr->lqNext();
         ptr->lqNextIs(0);
         ++version_;
         --members_;
      }
      return ptr;
   }
   void deleteHead() {
      T * ptr = head_.ptr();
      if( ptr ) {
  	 if( tail_ == ptr ) tail_ = 0;
         head_ = ptr->lqNext();
         ptr->lqNextIs(0);
         ++version_;
         --members_;
      }
   }
   T const * operator[]( T const * m ) const {
      for( T * c=head_.ptr();c;c=c->lqNext() ) if( c==m ) return c;
      return 0;
   }
   T const * member( T const * m ) const { return operator[](m); }
   T * operator[]( T const * m ) {
      for( T * c=head_.ptr();c;c=c->lqNext() ) if( c==m ) return c;
      return 0;
   }
   T const * operator[]( const typename T::Ptr& m ) const {
      return operator[](m.ptr());
   }
   T * operator[]( const typename T::Ptr& m ) { return operator[](m.ptr()); }

   Ptr<T> deleteMember( T * member ) {
      T * prev = 0;
      for( T * c = head_.ptr(); c; c = c->lqNext() ) {
         if( c == member ) {
            if( !c->lqNext() ) tail_ = prev;
            if( prev ) prev->lqNextIs( c->lqNext() );
            else head_ = c->lqNext();
            c->lqNextIs( 0 );
            ++version_;
            --members_;
            return c;
         }
         prev = c;
      }
      onDeleteMemberNotFound( member );
      return 0;
   }
   Ptr<T> deleteMember( const typename T::Ptr& _member ) {
      return deleteMember(_member.ptr() );
   }
   virtual void onDeleteMemberNotFound( T * ) {}
   // Override if some action required.

   class IteratorConst : public BaseIteratorConst<T> {
    public:
      P const * ptr() const { return _ptr()->fwkPtr(); }
      P const * operator->() const { return _ptr()->fwkPtr(); }
      V operator*() const { 
         T * p = const_cast< T * >( _ptr() );
         // Cast away const-ness because for invasive collections, operator* usually
         // returns "this", so it's more convenient if the user doesn't have to declare
         // fwkValue() const; if they did, they'd have to const_cast "this" themselves.
         return p->fwkValue(); 
      }
      IteratorConst& operator++() {
         const T * c = _ptr();
         ptrIs( c->lqNext() );
         return *this;
      }
      Self * list() const {
         return static_cast<Self *>(collection_);
      }
      T const * _ptr() const { return BaseIteratorConst<T>::ptr(); }
      // proivde access to this internal pointer for DelAlias impl
      // FixMe: provide "direct" iterator like with HashMap.
    protected:
      using BaseIteratorConst<T>::collection_;
      friend class LinkedQueue< T, P, V >;
      IteratorConst( Self const * lq, T * mem ) :
            BaseIteratorConst<T>( const_cast<Self*>(lq), mem ) {}
      IteratorConst( Self * lq, T * mem ) :
            BaseIteratorConst<T>(lq,mem) {}
   };

   IteratorConst iterator() const {
      return IteratorConst( this, const_cast<T *>(head_.ptr()) );
   }

   IteratorConst iterator( const Ptr<T>& start ) const {
      T * p = start.ptr();
      bool search = false;
      if( p && (p!=head_) && (p != tail_) ) {
         p = head_.ptr();
         search = true;
      }
      IteratorConst ii( this, p );
      if( search ) {
         p = start.ptr();
         while( ii && ii != p ) ++ii;
      }
      return ii;
   }

   class Iterator : public IteratorConst {
    public:
      using IteratorConst::_ptr;
      P * ptr() const { return const_cast< P * >( _ptr()->fwkPtr() ); }
      P * operator->() const { return const_cast< P * >( _ptr()->fwkPtr() ); }
      V operator*() const { return _ptr()->fwkValue(); }

      Self* list() const {
         return const_cast<Self *>(
               static_cast<Self const *>( collection_ ));
      }
      Iterator& operator++() {
         T * c = _ptr();
         prev_ = c;
         ptrIs( c->lqNext() );
         return *this;
      }
      void newPtr( T * newMember ) {
         T * p = prev_.ptr();
         if( p ) {
            newMember->lqNextIs( ptr() );
            p->lqNextIs( newMember );
         }
         else {
	    newMember->lqNextIs( list()->head() );
	    list()->head_ = newMember;
         }
         if( p == list()->tail_ ) list()->tail_ = newMember;
         ptrIs( newMember );
         ++list()->version_;
         ++list()->members_;
      }
      struct PointerConversion {
         int valid;
      };
      // For conversion to bool/int to ensure the following deleter does not
      // take precedent.
      operator int PointerConversion::*() const { 
         return _ptr() ? &PointerConversion::valid : 0; 
      }

      struct IterDeleter {
	 ~IterDeleter() {}
	 void operator delete( void * thisPtr ) {
            Iterator * i = (Iterator *) thisPtr;
            i->deletePtr();
	 }
      };
      operator IterDeleter * () {  return _ptr() ? (IterDeleter*) this : 0;  }
      // Conversion required for the delete operator, causing
      // Return null if null so test for null still works.
      // Note: the IterDeleter class is used because operator delete calls
      // the destructor.  delete iter; is just a way of invoking deletePtr.
      // FixMe: can IterDeleter and PointerConversion be combined?

      void deletePtr() {
         T * p = prev_.ptr();
         T * c = _ptr();
         T * newC = c->lqNext();
         if( !newC ) list()->tail_ = p;
         if( p ) p->lqNextIs( newC );
         else list()->head_ = newC;
         ptrIs( newC );
         ++list()->version_;
         --list()->members_;
      }
      void newMember( T * newMember ) {
         T * c = _ptr();
         newMember->lqNextIs( c->lqNext() );
         c->lqNextIs( newMember );
         if( !newMember->lqNext() ) tail_ = newMember;
         ++version_;
         ++members_;
      }
      // After the iterator pointer
      T * _ptr() const { return const_cast<T *>(BaseIteratorConst<T>::ptr()); }
    protected:
      using BaseIteratorConst<T>::collection_;
      Ptr<T> prev_;
      friend class LinkedQueue< T, P, V >;
      Iterator( const Self* lq, T * mem ) :
            IteratorConst( lq, mem ), prev_(0) {}
      Iterator( Self * lq, T * mem ) :
            IteratorConst(lq,mem), prev_(0) {}
   };

   Iterator iterator() {
      return Iterator( this, head_.ptr() );
   }

   void deleteAll() {
      Ptr<T> c = head_.ptr();
      head_ = 0;
      if( c ) {
         for( Fwk::Ptr<T> nextC = c->lqNext(); nextC; c = nextC ) {
            nextC = c->lqNext();
            c->lqNextIs( 0 ); // Deference next member
         }
      }
      tail_ = 0;
      ++version_;
      members_ = 0;
   }
   virtual ~LinkedQueue() { deleteAll(); }
   U32 auditErrors( U32 scope ) const {
      U32 count = 0;
      for( T * c=head_.ptr();c;c=c->lqNext() ) ++count;
      return count != members_;
   }

 private:
   typedef BaseCollection::StrepIterator StrepIterator;
   virtual bool iteratorMoreLeft( StrepIterator const & bi ) const {
      IteratorConst const * tmp = static_cast<IteratorConst const *>( &bi );
      return *tmp;
   }
   virtual void iteratorIncr( StrepIterator& bi ) const {
      IteratorConst * tmp = static_cast<IteratorConst *>( &bi );
      ++(*tmp);
   }
   // virtual void iteratorDelete( StrepIterator& ) const {}
   virtual String iteratorStrep( StrepIterator const & bi ) const {
      IteratorConst const * tmp = static_cast<IteratorConst const *>( &bi );
      return valueToStrep( **tmp );
   }

 private:
   U32 version_;
   U32 members_;
   Ptr<T> head_;
   T * tail_; // depend on the head/next reference to packet, not tail
};

}

#endif
