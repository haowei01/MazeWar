// Basic notifiee class template
// Copyright(c) 1993-2006, 2007, David R. Cheriton, all rights reserved.

#ifndef FWK_BASENOTIFIEE_H
#define FWK_BASENOTIFIEE_H

#include <fwk/PtrInterface.h>
#include <fwk/Exception.h>
#include <fwk/Ptr.h>
#include <fwk/Types.h>
#include <fwk/String.h>

namespace Fwk {

class RootNotifiee : public Fwk::PtrInterface<RootNotifiee> {
public:
   typedef Fwk::Ptr<RootNotifiee const> PtrConst;
   typedef Fwk::Ptr<RootNotifiee> Ptr;

   enum AttributeId {
     nullNotification_ = 0,
     multipleAttributes__ = -1,
     initialNotification__ = -2,
     this__ = 1,

     notificationException__ = 8,
     notificationAttribute__ = 9,
     deleteRef__ = 10,
     references__ = 11,
     auditErrors__ = 12,
     // Entity-level attributes
     name__ = 2,
     version__ = 3,
     clone__ = 4,
     entityRef__ = 5,
     attribute__ = 6,
     parent__ = 16,
     syncMode__ = 17,
     orphan__ = 20,
     entityId__ = 22,
     cloneState__ = 23,
     // FixMe: remove - not used.
     nextAttributeNumber__,
     tacNextAttributeId__ = nextAttributeNumber__,
     negativeAttr__ = 0x80000000, 
   };
   AttributeId notificationAttribute() const { 
      return notificationAttribute_;
    }
   static String attributeString( AttributeId );
   RootNotifiee const * lqNext()  const { return lqNext_.ptr(); }
   virtual String name() const;
   // Non-const interface =================================================

   RootNotifiee * fwkValue() { return this; }
   void notificationAttribute(AttributeId _notificationAttribute ) {
      notificationAttribute_ = _notificationAttribute;
   }
   AttributeId tacKeyForNotificationException() const {
      return tacKeyForNotificationException_;
   }
   void tacKeyForNotificationExceptionIs( AttributeId aid ){
      tacKeyForNotificationException_ = aid;
   }
   U8 tacNotificationExceptionChanges() const {
      return tacNotificationExceptionChanges_;
   }
   void tacNotificationExceptionChangesIs( U8 tnec ) {
      tacNotificationExceptionChanges_ = tnec;
   }
   RootNotifiee * lqNext()  { return lqNext_.ptr(); }
   void lqNextIs( RootNotifiee* _lqNext ) { lqNext_ = _lqNext; }
   // linkedQueue use is for deferred notification

   virtual void isNonReferencingIs( bool b );

   // Constructors ========================================================
   RootNotifiee(): notificationAttribute_(
                   AttributeId(nullNotification_)) {}
   virtual void handleNotification( Activity * );
   virtual void handleDefault( Activity *, AttributeId, bool );
   virtual void onAttribute( AttributeId );
   // Notification of update with specified attributeId for this notifiee.

   virtual void onNotificationException();
   virtual void onNotificationException( AttributeId );
   virtual void handleNotificationException( AttributeId aid );
   virtual void handleNotificationException();
   virtual void onDelete();
   virtual void onNotification();
   virtual void onCollectionNotification( String );
   virtual U32 auditErrors( U32 ) const;
protected:
   AttributeId notificationAttribute_;
   Ptr    lqNext_;
   AttributeId tacKeyForNotificationException_;
   U8 tacNotificationExceptionChanges_;
};

}

#endif /* BASENOTIFIEE_H */
