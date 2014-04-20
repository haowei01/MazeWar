// Copyright(c) 1993-2006_2007, David R. Cheriton, all rights reserved.
//
// <h2>Fwk::LinkedList</h2>
// Linked list of pointers to objects of type T that support smart pointers and
// that provide a "next" attribute that can be used by this list implementation.
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
// There is no concurrency control provided.  Moreover, a single client should
// not add or delete objects except through an iterator when any iterator is
// in existence.
// Note: removal of an member does not clear the next attribute, but relies on
// the client setting it as part of adding to some other data structure.
// This implementation does not allocate any memory or throw any exceptions.
//

#ifndef FWK_LINKEDLIST_H
#define FWK_LINKEDLIST_H

#include <fwk/Ptr.h>
#include <fwk/BaseCollection.h>
#include <fwk/Types.h>

namespace Fwk {

template<class T>
class LinkedList : public BaseRefCollection<T> {
public:
    typedef T MemberType;
    LinkedList() : version_(0),members_(0),head_(0),tail_(0) {}
    LinkedList( LinkedList<T> * ll ) : head_(ll->head()) {}

    U32 version() const { return version_; }
    U32 members() const { return members_; }

    const T * head() const { return head_.ptr(); }
    T * head() { return head_.ptr(); }
    void newMember( T * _newMember ) {
      if( tail_ ) {
         tail_->llNextIs(_newMember);
      }
      else head_ = _newMember;
      _newMember->llNextIs(0);
      tail_ = _newMember;
      ++version_;
      ++members_;
    }
    void newMember( const typename T::Ptr& _newMember ) {
       newMember(_newMember.ptr() );
    }
    T const * operator[]( T const * m ) const { 
      for( T * c=head_.ptr();c;c=c->llNext() ) if( c==m ) return c;
      return 0;
    }
    T const * member( T const * m ) const  { return operator[](m); }

   T const * operator[]( const typename T::Ptr& m ) const {
      return operator[](m.ptr() );
   }
   T const * member( const typename T::Ptr& m) const {return operator[](m); }

   T * operator[]( T const * m ) {
     for( T * c=head_.ptr();c;c=c->llNext() ) if( c==m ) return c;
     return 0;
   }
   T * member( T const * m ) { return operator[](m); }

   T * operator[]( const typename T::Ptr& m ) {
      return operator[](m.ptr() );
   }
   T * member( const typename T::Ptr& m ) { return operator[](m); }
    Ptr<T> headDel() {
      Ptr<T> ptr = head_.ptr();
      if( ptr ) {
  	 if( tail_ == ptr ) tail_ = 0;
         head_ = ptr->llNext();
         ptr->llNextIs( 0 );
         ++version_;
         --members_;
      }
      return ptr;
    }
    void deleteHead() {
       headDel();
    }
    T * deleteMember( T * member ) {
        T * prev = 0;
        for( T * c = head_.ptr(); c; c = c->llNext() ) {
            if( c == member ) {
                Ptr<T> save = member; 
                // 'save' holds a ref on member in case prev or head_ is the last reference
                if( prev ) prev->llNextIs( c->llNext() );
                else head_ = c->llNext();
                if(!save->llNext()) tail_ = prev;
                member->llNextIs( 0 );
                ++version_;
                --members_;
                return c;
	    }
            prev = c;
	}
       return 0;      
    }
    T * deleteMember( const typename T::Ptr& _member ) {
       return deleteMember(_member.ptr() );
    }
    template<typename TT> // TT is either T or const T.
    class ConstLlIterator : public BaseIteratorConst<TT> {
    public:
       ConstLlIterator& operator++() {
	  const TT * c = BaseIteratorConst<TT>::ptr();
          ptrIs( c->llNext() );
	  return *this;
       }
       const LinkedList<T>* list() const {
          return static_cast<const LinkedList<T> *>(this->collection_);
        }
       const TT * operator->() const { return BaseIteratorConst<TT>::ptr(); }
       const TT * ptr() const { return BaseIteratorConst<TT>::ptr(); }
     protected:
       friend class LinkedList<T>; // For access to constructor
       ConstLlIterator( const LinkedList<T>* ll, const T * mem ) :
             BaseIteratorConst<TT>( ll, mem ) {}
    };

    typedef ConstLlIterator<T> IteratorConst;

    IteratorConst iterator() const {
       return IteratorConst(this,head_.ptr());
     }

    template<typename TT> // TT is either T or const T.
    class LlIterator : public ConstLlIterator<TT> {
       using ConstLlIterator<TT>::collection_;
    public:
       using ConstLlIterator<TT>::ptr;
       TT * ptr() { return const_cast< TT * >( ConstLlIterator<TT>::ptr() ); }

       const LlIterator& operator++() {
	 if( TT * c = ptr() ) {
	    prev_ = c;
            ptrIs( c->llNext() );
	 }
	 return *this;
       }
      void newPtr( T * newMember ) {
	  TT * p = prev_.ptr();
  	  list()->newMemberThruIterator( p, newMember );
	  if( p ) {
	      newMember->llNextIs( ptr() );
	      p->llNextIs( newMember );
	  }
	  else {
	    newMember->llNextIs( list()->head() );
	    list()->head_ = newMember;
	  }
	  ptrIs( newMember );
          ++list()->version_;
          ++list()->members_;
      }
      void deletePtr() {	
          TT * p = prev_.ptr();
          Ptr<TT> c = ptr();
  	  list()->deleteMemberThruIterator( c.ptr() );
          TT * newC = c->llNext();
          if( !newC ) list()->tail_ = p;
	  if( p ) p->llNextIs( newC );
	  else list()->head_ = newC;
          c->llNextIs( 0 );
          ++list()->version_;
          --list()->members_;
	  ptrIs( newC );
      }
      void newMember( T * newMember ) {
	  TT * c = ptr();
  	  list()->newMemberThruIterator( c, newMember );
          if( c ) {
	      newMember->llNextIs( c->llNext() );
              c->llNextIs( newMember );
	      prev_ = c;
	  }
	  else { 
	    list()->newMember( newMember );
	  }
         ptrIs( newMember );
          ++list()->version_;
          ++list()->members_;
      }
       LinkedList<T>* list() {
          return static_cast<LinkedList<T> *>( const_cast< BaseCollection * >( collection_ ));
       }
#ifdef CRUD
        template <class OtherType>
        operator LlIterator<OtherType>() const {
             return LlIterator<OtherType>( collection_, ptr() );
         }
#endif
       TT * operator->() const { 
          return const_cast<TT*>( BaseIteratorConst<TT>::ptr() );
       }
    protected:
       friend class LinkedList<T>; // For access to constructor
       LlIterator( LinkedList<T> * ll, T * mem ) :
          ConstLlIterator<TT>(ll, mem), prev_(0) {}
       // Fix me - ensure mem is in the list
    private:
       Ptr<TT> prev_;
    };

    typedef LlIterator<T> Iterator;

    Iterator iterator() {
       return Iterator(this,head_.ptr());
     }

    void deleteAll() {
        Ptr<T> c = head_.ptr();
	head_ = 0;
        if( c ) {
           for( Ptr<T> nextC = c->llNext(); nextC; c = nextC ) {
	       nextC = c->llNext();
	       c->llNextIs( 0 ); // Deference next member
	   }
	}
        ++version_;
	members_ = 0;
    }
    virtual ~LinkedList() { deleteAll(); }
   U32 auditErrors( U32 scope ) const {
      U32 count = 0;
      for( T * c=head_.ptr();c;c=c->llNext() ) ++count;
      return count != members_;
   }
private:
   typedef BaseCollection::StrepIterator StrepIterator;
   virtual bool iteratorMoreLeft( StrepIterator const& iter ) const {
      IteratorConst const * i=(IteratorConst const *) &iter;
      return *i ? true : false;
   }
   virtual void iteratorIncr( StrepIterator& iter ) const {
      IteratorConst * i = (IteratorConst *) &iter;
      ++(*i);
   }
   virtual String iteratorStrep( StrepIterator const& iter ) const {
      IteratorConst const * i = static_cast<IteratorConst const *>(&iter);
      return valueToStrep((*i).ptr());
   }
   U32 version_;
   U32 members_;
   Ptr<T> head_;
   T * tail_; // depend on the head/next reference to entry, not tail
   virtual void newMemberThruIterator( T * prevMember, T * newMember ) {}
   virtual void deleteMemberThruIterator( T * newMember ) {}
};

}

#endif
