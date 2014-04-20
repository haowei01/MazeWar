// Copyright(c) 1993-2006_2007, David R. Cheriton, all rights reserved.
//
// <h2>ListRaw</h2>
// Linked list of (raw) pointers to objects of type T that support a raw
// "next" attribute (actually lrNext) that can be used by this implementation.
// The next attribute must accept, return and store a raw pointer of type T.
// The T destructor is assumed to provide deletion from list on count going
// to zero.
// This type is required to support smart pointers (for iteration)
// so an member can be destructed during iteration.
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
// There is no concurrency control provided.  Moreover, a single client should
// not add or delete objects except through an iterator when any iterator is
// in existence.
// Note: removal of an member does not clear the next attribute, but relies on
// the client setting it as part of adding to some other data structure.
// This implementation does not allocate any memory or throw any exceptions.
//
//

#ifndef FWK_LISTRAW_H
#define FWK_LISTRAW_H

#include <fwk/Ptr.h>
// #include <fwk/BasePtrIterator.h>
#include <fwk/BaseCollection.h>
#include <fwk/Exception.h>

namespace Fwk {

template<class T>
class ListRaw : public BaseRefCollection<T> {
public:
    ListRaw() : version_(0), members_(0), head_(0), tail_(0) {}

    U32 version() const { return version_; }
    U32 members() const { return members_; }

    T * head() const { return head_; }
    void newHead( const typename T::Ptr& newMember ) {
       newHead( newMember.ptr() );
    }
    void newHead( T * newMember ) {
      if( !head_ ) tail_ = newMember;
      newMember->lrNextIs( head_ );
      head_ = newMember;
      ++version_;
      ++members_;
    }
    void newMember( const typename T::Ptr& newMember ) {
       newMember( newMember.ptr() );
    }
    void newMember( T * newMember ) {
      newMember->lrNextIs(0);
      if( tail_ ) {
	tail_->lrNextIs(newMember);
      }
      else head_ = newMember;
      newMember->lrNextIs( 0 );
      tail_ = newMember;
      ++version_;
      ++members_;
    }
    T * headDel() {
       T * ptr = head_;
      if( ptr ) {
  	 if( tail_ == ptr ) tail_ = 0;
         head_ = ptr->lrNext();
         ++version_;
         --members_;
      }
      return ptr;
    }
    void deleteHead() {
       T * ptr = head_;
      if( ptr ) {
  	 if( tail_ == ptr ) tail_ = 0;
         head_ = ptr->lrNext();
         ++version_;
         --members_;
      }
    }
    void deleteMember( T * member ) {
        T * prev = 0;
        for( T * c = head_; c; c = c->lrNext() ) {
            if( c == member ) {
                if( !c->lrNext() ) tail_ = prev;
                if( prev ) prev->lrNextIs( c->lrNext() );
                else head_ = c->lrNext();
                ++version_;
                --members_;
                return;
	    }
            prev = c;
	}
    }
   
   class LrIteratorConst : public BaseIteratorConst<T> {
    public:
       using BaseIteratorConst<T>::ptr;
      
       LrIteratorConst& operator++() {
	  const T * c = ptr();
          ptrIs( c->lrNext() );
	  return *this;
       }
       ListRaw<T> const * listRaw() const {
          return static_cast<ListRaw<T> const *>(collection_);
        }

    protected:
       friend class ListRaw<T>;
      using BaseIteratorConst<T>::collection_;
       LrIteratorConst( const ListRaw<T> * lr, T * mem ) :
             BaseIteratorConst<T>( const_cast<ListRaw<T>*>(lr), mem ) {}
       LrIteratorConst( ListRaw<T> * lr, T * mem ) :
             BaseIteratorConst<T>(lr,mem) {}
    };

    typedef LrIteratorConst IteratorConst;



    IteratorConst iterator() const {
       return IteratorConst( this, const_cast<T *>(head_) );
    }

    IteratorConst iterator( const Ptr<T>& start ) const {
         T * p = start.ptr();
         bool search = false;
         if( p && (p!=head_) && (p != tail_) ) {
           p = head_;
           search = true;
         }
         IteratorConst ii( this, p );
         if( search ) {
             p = start.ptr();
             while( ii && ii != p ) ++ii;
         }
        return ii;
      }

    class LrIterator : public LrIteratorConst {
    public:
       using BaseIteratorConst<T>::ptr;
       using LrIteratorConst::listRaw;
       T * ptr() { return const_cast<T *>(BaseIteratorConst<T>::ptr()); }
       T * operator->() { return const_cast<T *>(ptr()); }
       ListRaw<T> * listRaw() {
          return const_cast<ListRaw<T> *>(LrIteratorConst::listRaw());
        }
       LrIterator& operator++() {
	  T * c = ptr();
	  prev_ = c;
          ptrIs( c->lrNext() );
	  return *this;
       }
       void fixPrev() {
          // fixPrev is used to fix up the prev_ pointer in case the
          // list has been modified while there is an outstanding
          // iterator
          while( int(listRaw()->version_ - version_) > 0 ) {
             version_ = listRaw()->version_;
             T * prev = 0;
             T * p = listRaw()->head();
             while( p ) {
                if( p == ptr() ) {
                   prev_ = prev;
                   goto done;
                }
                prev = p;
                p = p->lrNext();
             }
             // I am not in the list.  I must have been deleted.
             throw ListException( "Iterator pointing to an element not in the list" );
            done: ;
          }
          // Converting to int handles version wrap as long as there
          // are fewer than 2^31 updates to the list since the
          // iterator was created or last updated.  The while loop
          // handles the case where another thread is concurrently
          // modifying the list.
       }
      void newPtr( T * newMember ) {
         fixPrev();
         // fixPrev throws a ListException if we do not point to a
         // member of the list, which could occur if someone else has
         // deleted the element we're pointing at.
	  T * p = prev_.ptr();
	  if( p ) {
             newMember->lrNextIs( ptr() );
             p->lrNextIs( newMember );
	  }
	  else {
             newMember->lrNextIs( listRaw()->head() );
             listRaw()->head_ = newMember;
	  }
          if( p == listRaw()->tail_ ) listRaw()->tail_ = newMember;
	  ptrIs( newMember );
          ++listRaw()->version_;
          ++listRaw()->members_;
      }
      struct IterDeleter {
	 ~IterDeleter() {}
	 void operator delete( void * thisPtr ) {
            LrIterator * i = (LrIterator *) thisPtr;
            i->deletePtr();
	 }
      };
      operator IterDeleter * () {  return ptr() ? (IterDeleter*) this : 0;  }
      // Conversion required for the delete operator, causing
      // Return null if null so test for null still works.
      // Note: the IterDeleter class is used because operator delete calls
      // the destructor.  delete iter; is just a way of invoking deletePtr.
      // FixMe: can IterDeleter and PointerConversion be combined?

      void deletePtr() {
         try {
            fixPrev();
         } catch( ListException ) {
            return;
         }
	  T * p = prev_.ptr();
          T * c = ptr();
          T * newC = c->lrNext();
          if( !newC ) listRaw()->tail_ = p;
          assert( (p?p->lrNext():listRaw()->head_) == c ); // BUG393
	  if( p ) p->lrNextIs( newC );
	  else listRaw()->head_ = newC;
	  ptrIs( newC );
          ++listRaw()->version_;
          --listRaw()->members_;
      }

      void newMember( T * newMember ) {
         fixPrev();             // check that I'm in the list still.
	  T * c = ptr();
	  newMember->lrNextIs( c->lrNext() );
          c->lrNextIs( newMember );
          if( !newMember->lrNext() ) tail_ = newMember;
          ++listRaw()->version_;
          ++listRaw()->members_;
      }
      // After the iterator pointer
    protected:
       U32 version_;
       Ptr<T> prev_;
       friend class ListRaw<T>;
       LrIterator( const ListRaw<T>* li, T * mem ) :
             LrIteratorConst( li, mem ), version_( li->version() ), prev_(0) {}
       // this isn't quite right -- in a truly concurrency-correct
       // implementation you need to read the version before reading
       // anything else.
       LrIterator( ListRaw<T> * li, T * mem ) :
             LrIteratorConst(li,mem), version_( li->version() ), prev_(0) {}
    };
    typedef LrIterator Iterator;

    Iterator iterator() { return Iterator( this, head_ ); }

    void deleteAll() {
        Ptr<T> c = head_;
        head_ = 0;
        if( c ) {
           for( Fwk::Ptr<T> nextC = c->lrNext(); nextC; c = nextC ) {
	       nextC = c->lrNext();
	       c->lrNextIs( 0 ); // Deference next member
	   }
	}
        ++version_;
	members_ = 0;
    }
   virtual ~ListRaw() { deleteAll(); }
   U32 auditErrors( U32 scope ) const {
      U32 count = 0;
      for( T * c=head_;c;c=c->lrNext() ) ++count;
      return count != members_;
   }
private:
    U32 version_;
    U32 members_;
    T * head_;
    T * tail_;
};

}

#endif
