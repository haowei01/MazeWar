// Copyright(c) 1993-2007, David R. Cheriton, all rights reserved.
// <h2>Fwk::HashMap</h2>
// Revised
// takes const Ptr
// use an iterator
// assignment thru iterator
//
// HashMap of keys to pointers to objects that support smart pointers and
// that provide a "fwkHmNext" attribute that can be used by this implementation.
// The fwkHmNext attribute must accept and return a raw pointer but store a smart
// pointer of type T.
// It also must have a key attribute of type Key which is assumed to support
// returning a hash value and equality operator.
// Only to be used as an internal implementation data structure
// Use the provided iterators to iterate over entries in the map.
// Insert/delete using "newMember" and "deleteMember"
// Otherwise, use "deleteCurrent" in an iterator.
// Intended iterator structure in a for loop is:
// for( FooMap::IteratorConst i=list_.constIterator(); i; ++i){
//    Foo * f = *i;
//    . . . // do something
//  }
// Use the non-const iterator if deleting entries from the map.
// There is no protection against incrementing an iterator that is null and
// it should NEVER be done.
// There is no concurrency control provided.  Moreover, a single client should
// not add or delete objects except through an iterator when any iterator is
// in existence.

// This implementation grows and shrinks the hash table dynamically.
// A std::bad_alloc exception is thrown if memory allocation fails.
// The table initializes with 1 bucket. The table size is doubled when
// occupancies exceeds 4 entries per bucket (on average).  The table
// size is halved when occupancy falls below 1.5 entries per bucket
// (average).  The resize operation happens all in the context of the
// insert/remove operation.  The hash table, once grown, never shrinks
// down below 8 buckets unless memberDelAll is used, which reduces the
// table back to one bucket.
//
// hash table resize operation does not change the set of entries that
// an existing iterator visits.
//
// It is not guaranteed that an outstanding iterator will see any item
// added to the collection after it was created.  If an iterator
// points at an item that gets deleted, then all undeleted elements
// later in the same hash bucket as the deleted element will be
// silently skipped over by the iterator.  The only way to tell that
// this might have happened is to look at the version attribute of the
// hash table.

// In this implementation, the Key type must define operator<.
#ifndef FWK_HASHMAP_H
#define FWK_HASHMAP_H

#include <stdlib.h>
#include <fwk/BaseCollection.h>
#include <fwk/String.h>

namespace Fwk {


// P if defined separately is the pointer type that an iterator should
// return using ".ptr()" or "->".  T must contain a fwkPtr() that
// returns a P *.
//
// V if defined separately is the value type that an iterator should
// return using "*".  T must contain a fwkValue() that returns a V *.

// The hash table is dynamically resizing, in powers of 2.  It grows from 512 to 1024
// when the table gets 513 members in it.
// The table shrinks from 1024 to 512 when there are fewer than 256 members in it

// next largest power of 2
inline int nlpo2( unsigned int x )
{
   x |= (x >> 1);
   x |= (x >> 2);
   x |= (x >> 4);
   x |= (x >> 8);
   x |= (x >> 16);
   return(x+1);
}

unsigned int
inline bitReverse(register U32 x)
{
   register U32 y = 0x55555555;
   x = (((x >> 1) & y) | ((x & y) << 1));
   y = 0x33333333;
   x = (((x >> 2) & y) | ((x & y) << 2));
   y = 0x0f0f0f0f;
   x = (((x >> 4) & y) | ((x & y) << 4));
   y = 0x00ff00ff;
   x = (((x >> 8) & y) | ((x & y) << 8));
   return((x >> 16) | (x << 16));
}

template<typename Key>
inline U32 rhash( Key const & k ) {
   return bitReverse( hash( k ) );
}

template< typename T, typename Key, typename P = T, 
          typename Vconst = P, typename V = Vconst, int bkts = 1,
          int K=4, U32 (*hash)(Key const &) = rhash>
class HashMap : public BaseRefCollection<T> {
 public:
   typedef T MemberType;
   typedef HashMap<T,Key,P,Vconst,V,bkts,K,hash> Self;
   HashMap( U32 _buckets=bkts) : version_(1), members_(0) {
      if( _buckets ) {
         buckets_ = (_buckets == 1) ? 1 : nlpo2( _buckets - 1 ); // round up to power of two
         bucket_ = (Ptr<T> *)
            new U8[ buckets_ * sizeof(Ptr<T>) ];
         memset( bucket_, 0, buckets_ * sizeof(Ptr<T>) );
      }
      else { 
         buckets_ = 0;
         bucket_ = 0;
      }
#ifdef HASHMAP_STATS
      resizeUps_ = resizeDowns_ = 0;
#endif
   }

   U32 rshift() const { return __builtin_clzl( buckets_ ) + 1; }
   U32 buckets() const { return buckets_; }
   U32 bucket( U32 hashVal ) const { 
      U32 rs = rshift();
      return (rs == 32) ? 0 : hashVal >> rs;
   }
   U32 members() const { return members_; }
   U32 version() const { return version_; }
   void bucketsIs( U32 b ) {
      // This algorithm grows or shrinks the hash table so that the
      // new size is b (rounded up to a power of 2).  When the table
      // grows or shrinks, ordering is preserved.  The min table size is 2
      U32 bb = b ? nlpo2( b - 1 ) : 1;    // round up to power of 2
      if(buckets_ == bb) { 
         return;
      }
      U32 obuckets = buckets_;
      Ptr<T> * obucket = bucket_;
      buckets_ = bb;
      U8 * mem = new U8[ buckets_ * sizeof(Ptr<T>) ];
      memset( mem, 0, buckets_ * sizeof(Ptr<T>) );
      Ptr<T> * nbucket = reinterpret_cast<Ptr<T> *>( mem );
      bucket_ = nbucket;
      for( S32 i=obuckets-1;i>=0;--i) {
         // t points to the head of the old bucket.
         // t1 stops at the last entry with the same new bucket as t.
         // t2 points to the first entry that goes in a different bucket (or null)
         // b1 is the bucket that t through t1 go in
         // b2 is the bucket that t2 and possibly later entries goes in
         T * t = obucket[i].ptr();
         // visit each bucket i and split it across the new buckets.
         // This does not work for a shrink
         U32 b2 = 0; // init to 0 to quiet gcc
         if( t ) {
            U32 b1 = bucket(hash( t->fwkKey() ) );
            while( t ) {
               T * t1 = t;
               T * t2;
               while( (t2 = t1->fwkHmNext()) ) {
                  b2 = bucket(hash(t2->fwkKey()));
                  if( b1 != b2 ) break;
                  t1 = t2;
               }
               Ptr<T> t2a = t2; // anchor t2 before changing t1->fwkHmNext
               t1->fwkHmNextIs( nbucket[ b1 ].ptr() );
               nbucket[b1] = t;
               obucket[i] = t2;
               t = t2;
               b1 = b2;
            }
         }
      }
      delete [] (U8*) obucket;
      version_++;
#ifdef HASHMAP_STATS
      if( buckets_ > obuckets ) {
         resizeUps_++;
      } else {
         resizeDowns_++;
      }
      if( false )
         std::cerr << "resizing " << ((buckets_ > obuckets) ? "up" : "down") 
                   << " from " << obuckets << " to " << buckets_ << " at " << members_ << std::endl;
#endif
   }

   U32 auditErrors( U32 ) const {
      U32 errs = 0;
      U32 count = 0;
      T ** pbp = (T **) bucket_ + buckets_;
      U32 hsh = 0;
      U32 hashVal = 0;
      for( T ** cbp = (T **)bucket_; cbp < pbp; ++cbp, ++hsh ) {
         T* c = *cbp;
         while( c ) {
            U32 h = hash( c->fwkKey() );
            if( hsh != bucket( h ) ) { ++errs; }
            if( h < hashVal ) { ++errs; }
            if( ++count > members_ ) break;
            c = c->fwkHmNext();
            hashVal = h;
         }
      }
      if( count != members_ ) ++errs;
      return errs;
   }
#ifdef HASHMAP_STATS
   U32 resizeUps() const {
      return resizeUps_;
   }
   U32 resizeDowns() const {
      return resizeDowns_;
   }
#endif
    
   U32 size() const { return 1; }
   T const * operator[]( const Key& k ) const {
      U32 i = bucket( hash( k ) );
      for( T * c = bucket_[i].ptr(); c; c = c->fwkHmNext() ) {
         if( c->fwkKey() == k ) return c;
      }
      return 0;
   }        
   T const * member( const Key& k ) const { return operator[](k); }
   // Find first member identified by key k, returning 0 if none.

   T * operator[]( const Key& k ) {
      U32 i = bucket( hash( k ) );
      for( T * c = bucket_[i].ptr(); c; c = c->fwkHmNext() ) {
         if( c->fwkKey() == k ) return c;
      }
      return 0;
   }        
   T * member( const Key& k )  { return operator[](k); }
   // Find first member identified by key k, returning 0 if none.

   bool isMember( const T * t ) const {
      U32 i = bucket( hash( t->fwkKey() ) );
      for( T * c = bucket_[i].ptr(); c; c = c->fwkHmNext() ) {
         if( c == t ) return true;
      }
      return false;
   }        
   // Return true if t is a member of this hash map, else false.

   void memberIs( const Ptr<T>& t ) {
      // Insert t in the right bucket.  Order within the bucket is
      // smallest-to-largest by hash key.
      newMember( t, hash( t->fwkKey() ) );
   }

   void newMember( T * t ) {
      newMember( t, hash( t->fwkKey() ) );
   }
   void memberIs( const T * t ) { memberIs( const_cast<T *>(t) ); }
   void newMember( const T * t ) { memberIs( const_cast<T *>(t) ); }
   void newMember( const Ptr<T>& t ) {
      memberIs(t);
   }    

   void deleteMember( const Ptr<T>& t ) {
      U32 i = bucket( hash( t->fwkKey() ));
      T * prev = 0;
      for( T * c = bucket_[i].ptr(); c; c = c->fwkHmNext() ) {
         if( c == t.ptr() ) { // Found member
            if( prev ) {
               prev->fwkHmNextIs( c->fwkHmNext());
            }
            else {
               bucket_[i] = c->fwkHmNext();
            }
            --members_;
            ++version_;
            c->fwkHmNextIs( 0 );
            break;
         }
         prev = c;
      }
      maybeShrink();
   }
   // Delete t if in the hash table, i.e. make it not an member if it is.

   Ptr<T> memberDel( Key k ) {
      U32 i = bucket( hash( k ));
      T * prev = 0;
      for( T * c = bucket_[i].ptr(); c; c = c->fwkHmNext() ) {
         if( k == c->fwkKey() ) { // Found member
            Ptr<T> ret = c;
            if( prev ) {
               prev->fwkHmNextIs( c->fwkHmNext() );
            }
            else {
               bucket_[i] = c->fwkHmNext();
            }
            --members_;
            ++version_;
            c->fwkHmNextIs( 0 );
            maybeShrink();
            return ret;
         }
         prev = c;
      }
      return 0;
   }
   Ptr<T> deleteMember( Key k ) {
      return memberDel(k);
   }
   // Provided for backwards compatibilty - remove when not needed.

   void memberDelAll() {
      emptyAllBuckets();
      if( buckets_ != 1 ) bucketsIs( 1 );
      ++version_;
   }
   bool operator==( Self const & v ) const {
      return members() == v.members();
      // FixMe: implement true equality
   }
   bool operator!=( Self const & v ) const {
      return !(v== this);
   }
   // nominal operators for when a hashMap occurs in a value type.
   void deleteAll() {
      // Provided for backwards compat.
      memberDelAll();
   }
 protected:
   
   T * findNext( T const * t, S32 * n ) const { // Move to next non-null bucket if any.
      // Pass t as NULL to find the first non-empty bucket.
      T ** pbp = (T **) (bucket_ + buckets_);
      T ** cbp;
      if( t ) {
         U32 h = hash( t->fwkKey() );
         U32 i = bucket( h );
         cbp = (T **) &bucket_[i];
         if( T * t1 = (*cbp) ) {
            // if t's bucket is non-empty, then search the bucket for
            // an entry with a larger (hash,key) tuple.
            do {
               U32 h1 = hash( t1->fwkKey() );
               if((h < h1) || (h == h1 && t->fwkKey() < t1->fwkKey())) {
                  *n = cbp - (T**)bucket_;
                  return t1;
               }
               t1 = t1->fwkHmNext();
            } while( t1 );
            // fall through to find head of next non-empty bucket
         }
         ++cbp;
      } else {
         cbp = (T **) &bucket_[0];
      }
      // Find the first entry at or after cbp
      T * bkt = 0;
      while( (cbp < pbp) && !(bkt = *cbp) ) ++cbp;
      *n = cbp - (T**)bucket_;
      return bkt;
   }

   T * findNextBucket( S32 * bkt ) const {
      T ** bp = (T **) bucket_;
      for(U32 i=(*bkt)+1; i<buckets_; ++i) {
         if( bp[i] ) {
            *bkt = i;
            return bp[i];
         }
      }
      *bkt = -1;
      return 0;
   }
   
 public:

   class IteratorConstBase : public BaseIteratorConst<T> {
    protected:
      using BaseIteratorConst<T>::collection_;
      IteratorConstBase() : BaseIteratorConst<T>( 0, 0 ) {
         versionIs( 0 );
         bucketIs( 0 );
         slowIncrsIs( 0 );
      }
      void advance() {
         T const * c = _ptr();
         T const * newC = c->fwkHmNext();
         if( newC ) {
            ptrIs( newC );
         } else if( version() == hashMap()->version() ) {
            ptrIs( hashMap()->findNextBucket( (S32*)&data1_ ) );
         } else {
            S32 bkt;
            ptrIs( hashMap()->findNext( c, &bkt ) );
            bucketIs(bkt);
            versionIs( hashMap()->version() );
            slowIncrsIs( slowIncrs() + 1 );
         }
      }
      T const * _ptr() const { return BaseIteratorConst<T>::ptr(); }
      Self const * hashMap() const { return static_cast<Self const *>( collection_ ); }
      IteratorConstBase( Self const * hm, T const * t, S32 bkt = -1 ) :
            BaseIteratorConst<T>(hm, t) {
         versionIs((bkt >= 0) ? hm->version() : 0);
         bucketIs(bkt);
         slowIncrsIs( 0 );
      }
      using BaseIteratorConst<T>::data0_;
      using BaseIteratorConst<T>::data1_;
      void bucketIs(S32 b) { data1_ = (U32)b; }
      void versionIs(U32 v) { data0_ = v; }
    public:
      S32 bucket() const { return (S32)data1_; }
      U32 version() const{ return data0_; }
#ifdef HASHMAP_STATS
      U32 slowIncrs() const { return slowIncrs_; }
      void slowIncrsIs( U32 c ) { slowIncrs_ = c; }
    private:
      U32 slowIncrs_;
#else
      void slowIncrsIs( U32 c ) {}
      U32 slowIncrs() const { return 0; }
#endif
   };


   class IteratorConst : public IteratorConstBase {
    public:
      IteratorConst() {}
      using BaseIteratorConst<T>::collection_;
      IteratorConst const & operator++() {
         IteratorConstBase::advance();
         return *this;
      }
      using IteratorConstBase::_ptr;
      Vconst operator*() const { return _ptr()->fwkValue(); }
      P const * operator->() const { return _ptr()->fwkPtr(); }
      P const * ptr() const { return _ptr()->fwkPtr(); }
      Key key() const { return _ptr()->fwkKey(); }
      bool markedForDeletion() const { return _ptr()->tacMarkedForDeletion(); }
          
    protected:
      friend class HashMap<T,Key,P,Vconst,V,bkts,K,hash>; // For access to constructor
      IteratorConst( Self const * hm, T const * t, S32 bkt=-1 ) : IteratorConstBase( hm, t, bkt ) {}
   };
   
   IteratorConst iterator() const {
      S32 bkt;
      T * t = findNext( 0, &bkt );
      return IteratorConst( this, t, bkt );
   }
   IteratorConst iterator( const Ptr<T>& start ) const {
      return IteratorConst( this, start.ptr() );
   }
   // Position at specified start object, or end if null
   
   IteratorConst iterator( const Key & k ) const {
      // could be optimized to return the bucket
      return IteratorConst( this,operator[](k));
   }
   // Position at object with specified start key, or end if null
   
   template<typename DT>
   class DerIteratorConst : public IteratorConstBase {
    public:
      DerIteratorConst() {}
      using BaseIteratorConst<T>::collection_;
      DerIteratorConst const & operator++() {
         do {
            IteratorConstBase::advance();
         } while(ptr() && !dynamic_cast<DT const *>(ptr()));
         return *this;
      }
      Fwk::Ptr<DT const> operator*() const { return _ptr(); }
      DT const * operator->() const { return _ptr()->fwkPtr(); }
      DT const * ptr() const { return _ptr(); }
      Key key() const { return _ptr()->fwkKey(); }
      bool markedForDeletion() const { return _ptr()->tacMarkedForDeletion(); }
          
    protected:
      friend class HashMap<T,Key,P,Vconst,V,bkts,K,hash>; // For access to constructor
      DT const * _ptr() const { return dynamic_cast<DT const *>(BaseIteratorConst<T>::ptr()); }
      Self const * hashMap() const { return static_cast<Self const *>(collection_); }
      DerIteratorConst( Self const * hm, DT const * t, S32 bkt=-1 ) : 
            IteratorConstBase( hm, t, bkt ) {
         if(t&&!dynamic_cast<DT const *>(t)) {
            operator++();
            // advance to first element of this type.
         }
      }
   };
   template<typename DT>
   DerIteratorConst<DT> iterator() const {
      S32 bkt;
      T const * t = findNext(0, &bkt);
      DT const * dt = dynamic_cast<DT const *>(t);
      return DerIteratorConst<DT>(this,dt);
   }
   template<typename DT>
   DerIteratorConst<DT> iterator( const Ptr<T>& start ) const {
      return DerIteratorConst<DT>(this,start.ptr());
   }
   // Position at specified start object, or end if null
   
   template<typename DT>
   DerIteratorConst<DT> iterator( const Key & k ) const {
      DT const * dt = dynamic_cast<DT const *>(operator[](k));
      return DerIteratorConst<DT>(this,dt);
   }

   // Direct iteratorConst - used when a direct implementation of attr
   // and to iterate over the implementation type when indirect, i.e. entry
   // or reactor entry types.
   // FixMe: other iterator should be renamed as an indirect iterator
   class DIteratorConst : public IteratorConstBase {
    public:
      DIteratorConst() {} 
      using BaseIteratorConst<T>::collection_;
      DIteratorConst const & operator++() {
         IteratorConstBase::advance();
         return *this;
      }
      using IteratorConstBase::_ptr;
      T const * operator*() const { return _ptr(); }
      T const * operator->() const { return _ptr(); }
      T const * ptr() const { return _ptr(); }
      Key key() const { return _ptr()->fwkKey(); }
      bool markedForDeletion() const { return _ptr()->tacMarkedForDeletion(); }
          
    protected:
      friend class HashMap<T,Key,P,Vconst,V,bkts,K,hash>; // For access to constructor
//      T const * _ptr() const { return BaseIteratorConst<T>::ptr(); }
      Self const * hashMap() const { return static_cast<Self const *>( collection_ ); }
      DIteratorConst( Self const * hm, T const * t, S32 bkt=-1 ) : IteratorConstBase( hm, t, bkt ) {}
   };

   DIteratorConst dIterator() const {
      S32 bkt;
      T * t = findNext( 0, &bkt );
      return DIteratorConst(this,t,bkt);
   }
   DIteratorConst dIterator( const Ptr<T>& start ) const {
      return DIteratorConst(this,start.ptr());
   }
   // Position at specified start object, or end if null
   
   DIteratorConst dIterator( const Key & k ) const {
      return DIteratorConst(this,operator[](k));
   }

   // Non-const iterators
   class Iterator : public IteratorConst {
    public:
      Iterator() {}
      Iterator( Self * hm, T * t ) : IteratorConst(hm, t) {}
      using IteratorConst::_ptr;
      P * ptr() const { return const_cast< P * >( _ptr()->fwkPtr() ); }
      P * operator->() const { return const_cast< P * >( _ptr()->fwkPtr() ); }
      V operator*() const { return _ptr()->fwkValue(); }

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
         T * t = _ptr();
         ++(*this);
         hashMap()->deleteMember( t );
      }
      using IteratorConst::markedForDeletion;
      void markedForDeletion( bool on ) {
         _ptr()->tacMarkedForDeletionIs(on);
      }
    protected:
      friend class HashMap<T,Key,P,Vconst,V,bkts,K,hash>; // For access to constructor
      Iterator( Self * hm, T * t, S32 bkt ) : IteratorConst(hm, t, bkt) {}
      Self * hashMap() const { return const_cast<Self *>( IteratorConst::hashMap() ); }
      T * _ptr() const { return const_cast< T * >( BaseIteratorConst<T>::ptr() ); }
   };

   Iterator iterator() { 
      S32 bkt;
      T * t = findNext( 0, &bkt );
      return Iterator( this, t, bkt );
   }
   Iterator iterator( const Ptr<T>& start ) { 
      return Iterator( this, start.ptr() );
   }
   Iterator iterator( const Key & k ) {
      return Iterator( this,operator[](k));
   }
   // Position at object with specified start key, or end if null

   template<typename DT>
   class DerIterator : public DerIteratorConst<DT> {
    public:
      DerIterator() {}
      DerIterator( Self * hm, DT * t ) : DerIteratorConst<DT>(hm,t) {}
      using DerIteratorConst<DT>::_ptr;
//      DT * ptr() const { return const_cast<DT *>( _ptr()->fwkPtr() ); }
      DT * ptr() const { return const_cast<DT *>(_ptr()); }
      DT * operator->() const { return const_cast<DT * >(_ptr()->fwkPtr()); }
      Fwk::Ptr<DT> operator*() { return _ptr(); }

      struct IterDeleter {
	 ~IterDeleter() {}
	 void operator delete( void * thisPtr ) {
            DerIterator * i = (DerIterator *) thisPtr;
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
         DT * t = _ptr();
         ++(*this);
         hashMap()->deleteMember( t );
      }
      using DerIteratorConst<DT>::markedForDeletion;
      void markedForDeletion( bool on ) {
         _ptr()->tacMarkedForDeletionIs(on);
      }
    protected:
      friend class HashMap<T,Key,P,Vconst,V,bkts,K,hash>; // For access to constructor
      DerIterator( Self * hm, DT * t, S32 bkt ) : DerIteratorConst<DT>(hm,t,bkt) {}
      Self * hashMap() const {
         return const_cast<Self *>(DerIteratorConst<DT>::hashMap() );
      }
      DT * _ptr() const {return const_cast<DT*>(DerIteratorConst<DT>::ptr());}
   };

   template<typename DT> 
   DerIterator<DT> iterator() { 
      S32 bkt;
      T * t = findNext(0, &bkt);
      DT * dt = dynamic_cast<DT *>(t);
      return DerIterator<DT>(this,dt,bkt);
   }
   template<typename DT> 
   DerIterator<DT> iterator( const Ptr<T>& start ) { 
      return DerIterator<DT>( this, start.ptr() );
   }
   template<typename DT>
   DerIterator<DT>  iterator( const Key & k ) {
      DT * dt = dynamic_cast<DT *>(operator[](k));
      return DerIterator<DT>(this,dt);
   }

   // Direct non-const iterator
   class DIterator : public DIteratorConst {
    public:
      DIterator() {}
      DIterator( Self * hm, T * t ) : DIteratorConst(hm, t) {}
      using DIteratorConst::_ptr;
      T * ptr() const { return const_cast<T *>( _ptr()); }
      T * operator->() const { return const_cast<T *>( _ptr()); }
      T * operator*() const { return _ptr(); }

      struct IterDeleter {
	 ~IterDeleter() {}
	 void operator delete( void * thisPtr ) {
            DIterator * i = (DIterator *) thisPtr;
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
         T * t = _ptr();
         ++(*this);
         hashMap()->deleteMember( t );
      }
      using DIteratorConst::markedForDeletion;
      void markedForDeletion( bool on ) {
         _ptr()->tacMarkedForDeletionIs(on);
      }
    protected:
      friend class HashMap<T,Key,P,Vconst,V,bkts,K,hash>; // For access to constructor
      DIterator( Self * hm, T * t, S32 bkt ) : DIteratorConst(hm, t, bkt) {}
      Self * hashMap() const { return const_cast<Self *>(DIteratorConst::hashMap() ); }
      T * _ptr() const { return const_cast< T * >( BaseIteratorConst<T>::ptr() ); }
   };

   DIterator dIterator() { 
      S32 bkt;
      T * t  = findNext(0, &bkt );
      return DIterator(this,t,bkt);
   }
   DIterator dIterator( const Ptr<T>& start ) { 
      return DIterator(this,start.ptr());
   }
   DIterator dIterator( const Key & k ) {
      return DIterator(this,operator[](k));
   }

   HashMap( const Self & hm ) {
      version_ = hm.version_;      
      members_ = hm.members_;      
      buckets_ = hm.buckets_;      
      bucket_ = (Ptr<T> *) calloc(buckets_, sizeof(Ptr<T>));

      // Clone all the members.
      T ** pbp = (T **) hm.bucket_ + hm.buckets_;
      T ** newPbp = (T **) bucket_ + hm.buckets_;
      Fwk::Ptr<T> * newCbp = bucket_;
      for( T ** cbp = (T **)hm.bucket_; cbp < pbp; ++cbp, ++newCbp ) {
         T * c = *cbp;
         T * newPrev = 0;
         if( !c ) { *newCbp = 0; continue; }
         do {
            if( newPrev ) {
               newPrev->fwkHmNextIs( new T(*c) );
               newPrev = newPrev->fwkHmNext();
	    }
            else {  
               *newCbp = newPrev = new T(*c);
            }
	    c = c->fwkHmNext();
	 } while( c );
      }
   }
   // Value/subentity-oriented copy constructor.
   HashMap( const char *, const char * ) : version_(0), members_(0),
                                buckets_(bkts) {
      // Required for STREP code 
      // FixMe: determine bkts from string.
      if( buckets_) {
         bucket_ = (Ptr<T> *) new U8[ buckets_ * sizeof(Ptr<T>) ];
         memset( bucket_, 0, buckets_ * sizeof(Ptr<T>) );
      }
      else bucket_ = 0;
   }
   virtual ~HashMap() {  // Deference all items in table and free buckets.
      emptyAllBuckets();
      U8 * bucketMemory = (U8 *) bucket_;
      if( bucketMemory ) delete [] bucketMemory;
   }
   
 private:
   void maybeGrow() {
      // Double the hash table size when the number of entries per
      // bucket exceeds K.
      if( members_ > (K * buckets_) ) { 
         if( K >= 2 ) {
            U32 zero = 0; // quiet a spurious compiler warning
            bucketsIs( members_ / ((K+zero)/2));
         } else {
            bucketsIs( (members_-1) * (2/K) );
         }
      }
   }
   void maybeShrink() {
      if( buckets_ <= 8 ) return; // don't shrink below 8 buckets
      if( members_ * 4 < K * (buckets_ + buckets_ / 2) ) {
         bucketsIs( members_ / K );
      }
   }
   void newMember( const Ptr<T>& t, U32 hashVal ) {
      U32 i = bucket( hashVal );
      assert( t->fwkHmNext() == 0 );
      T * prev = 0;
      T * t1 = bucket_[i].ptr();
      while( t1 ) {
         U32 h1 = hash( t1->fwkKey() );
         if( hashVal < h1 ) break;
         if( h1 == hashVal && t->fwkKey() < t1->fwkKey()) break;
         prev = t1;
         t1 = t1->fwkHmNext();
      }
      // advance prev and t1 until prev points at the last entry with
      // a hash key that is smaller than t's hash.a
      t->fwkHmNextIs( t1 );
      if( prev ) {
         prev->fwkHmNextIs( t.ptr() );
      } else {
         bucket_[i] = t;
      }
      ++members_;
      ++version_;
      maybeGrow();
   }
   // Insert t as member in the hash map according to its key value.
   // This does not check for an member of this key already present.

   typedef BaseCollection::StrepIterator StrepIterator;
   virtual bool iteratorMoreLeft( StrepIterator const & bi ) const {
      IteratorConst const * tmp = static_cast<IteratorConst const *>( &bi );
      return *tmp;
   }
   virtual void iteratorIncr( StrepIterator& bi ) const {
      IteratorConst * tmp = static_cast<IteratorConst *>( &bi );
      ++(*tmp);
   }
   virtual String iteratorStrep( StrepIterator const & bi ) const {
      IteratorConst const * tmp = static_cast<IteratorConst const *>( &bi );
      return valueToStrep( tmp->key() );
   }
   void emptyAllBuckets() {
      Ptr<T> * pbp = bucket_ + buckets_;
      for( Ptr<T> * cbp = bucket_; cbp < pbp; ++cbp ) {
         Ptr<T> c = *cbp;
         *cbp = 0;
         if( c ) {
            for( Ptr<T> nextC = c->fwkHmNext(); c; c = nextC ) {
               nextC = c->fwkHmNext();
               c->fwkHmNextIs( 0 ); // Deference next member
            }
         }
      }
      members_ = 0;
   }

   U32 version_;
   U32 members_;
   U32 buckets_;
   Ptr<T> *bucket_; // pointer to vector of buckets.
#ifdef HASHMAP_STATS
   U32 resizeDowns_;
   U32 resizeUps_;
#endif
};

}

#endif
