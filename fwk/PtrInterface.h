// Copyright (c) 1993-2007 David R. Cheriton, all rights reserved.
// PtrInterface.h

#ifndef FWK_PTRINTERFACE_H
#define FWK_PTRINTERFACE_H

#include "Types.h"

namespace Fwk {

template <class T>
class PtrInterface {
private:
    long unsigned ref_;
public:
    PtrInterface() : ref_(1) {}
    unsigned long references() const { return ref_; }
    enum Attribute {
      nextAttributeNumber__ = 1
    };
    // DRC - support for templates
     const PtrInterface * newRef() const;
     void deleteRef() const;
     void referencesDec( U32 dec ) const;
protected:
    virtual ~PtrInterface() {}

    virtual void onZeroReferences() { delete this; }
};

template<class T> const PtrInterface<T> *
PtrInterface<T>::newRef() const {
    PtrInterface *me = const_cast<PtrInterface *>( this );
    ++me->ref_;
    return this;
}

template<class T> void
PtrInterface<T>::deleteRef() const {
    PtrInterface *me = const_cast<PtrInterface *>( this );
    if( --me->ref_ == 0 ) me->onZeroReferences();
}

template<class T> void
PtrInterface<T>::referencesDec( U32 dec ) const {
    PtrInterface *me = const_cast<PtrInterface *>( this );
    if((me->ref_-=dec) == 0 ) me->onZeroReferences();
}

}

#endif
