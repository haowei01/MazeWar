// Copyright (c) 2005-2006 David R. Cheriton.  All rights reserved.

#include <fwk/BaseCollection.h>
#include <fwk/Exception.h>

bool
Fwk::BaseCollection::iteratorMoreLeft( Fwk::BaseCollection::StrepIterator const & ) const {
   throw Fwk::NoImplementationException( "generic iteration is unimplemented for this collection type" );
}

void
Fwk::BaseCollection::iteratorIncr( Fwk::BaseCollection::StrepIterator & ) const {
   throw Fwk::NoImplementationException( "generic iteration is unimplemented for this collection type" );
}

void
Fwk::BaseCollection::iteratorDelete( Fwk::BaseCollection::StrepIterator& ) const {}
// Null action on delete by default.  Throwing an exception in a destructor
// is not allowed.

Fwk::String 
Fwk::BaseCollection::iteratorStrep( Fwk::BaseCollection::StrepIterator const &) const
{
   throw Fwk::NoImplementationException( "generic iteration is unimplemented for this collection type" );
}

void const *
Fwk::BaseCollection::iteratorSpaceCopy( Fwk::BaseCollection::StrepIterator const & other ) const
{
   return other.space_;
}

Fwk::BaseCollection::~BaseCollection() {
}
