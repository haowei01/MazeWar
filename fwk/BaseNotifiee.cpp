// Copyright (c) 2005-2006_2007 David R. Cheriton.  All rights reserved.

#include <fwk/BaseNotifiee.h>
#include <iostream>

namespace Fwk {


class Activity;
String RootNotifiee::name() const { return "unknown"; }
void RootNotifiee::isNonReferencingIs( bool b ) { assert( false ); }
void RootNotifiee::handleNotification( Activity * ) {}
void RootNotifiee::handleDefault( Activity *, AttributeId, bool ) {}
void RootNotifiee::onAttribute( AttributeId ) {}
void RootNotifiee::onNotificationException() {}

void RootNotifiee::onNotificationException( AttributeId ) { 
   onNotificationException();
}

void
RootNotifiee::handleNotificationException( AttributeId aid ) {
   onNotificationException(aid);
}
void
RootNotifiee::handleNotificationException() {
   onNotificationException();
}
void RootNotifiee::onDelete( ) {}
U32 RootNotifiee::auditErrors( U32 ) const { return 0; }

// support for generic TacNotifieeAdapter
void RootNotifiee::onNotification() {}
void RootNotifiee::onCollectionNotification( String ) {}

String RootNotifiee::attributeString( RootNotifiee::AttributeId a )
{
   Fwk::String str = "unknown";
   switch (a) {
     case nullNotification_: str = "nullNotification_"; break;
     case multipleAttributes__: str = "multipleAttributes__"; break;
     case initialNotification__: str = "initialNotification__"; break;
     case this__: str = "this__"; break;
     case notificationException__: str = "notificationException__"; break;
     case notificationAttribute__: str = "notificationAttribute__"; break;
     case deleteRef__: str = "deleteRef__"; break;
     case references__: str = "references__"; break;
     case auditErrors__: str = "auditErrors__"; break;
     case name__: str = "name__"; break;
     case version__: str = "version__"; break;
     case clone__: str = "clone__"; break;
     case entityRef__: str = "entityRef__"; break;
     case attribute__: str = "attribute__"; break;
     case parent__: str = "parent__"; break;
     case syncMode__: str = "syncMode__"; break;
     case orphan__: str = "orphan__"; break;
     case entityId__: str = "entityId__"; break;
     case cloneState__: str = "cloneState__"; break;
     default: /* nothing */;
   }
   return str;
}


} // namespace Fwk
