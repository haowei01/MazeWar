// Copyright (c) 2005-2006_2007 David R. Cheriton.  All rights reserved.

#include <fwk/Exception.h>
#include <assert.h>
#include <iostream>
#include <signal.h>

Fwk::Exception::Id Fwk::Exception::IdInstance( U32 v ) {
   switch( v ) {
    case noException_ : return noException_;
    case unknownException_ : return unknownException_;
    case unknownTypeException_ : return unknownTypeException_;
    case unknownAttrException_ : return unknownAttrException_;
    case unknownDelimiterException_ : return unknownDelimiterException_;
    case unknownArgException_ : return unknownArgException_;
    case internalException_ : return internalException_;
    case rangeException_ : return rangeException_;
    case memoryException_ : return memoryException_;
    case storageException_ : return storageException_;
    case timeoutException_ : return timeoutException_;
    case nameInUseException_ : return nameInUseException_;
    case illegalNameException_ : return illegalNameException_;
    case permissionException_ : return permissionException_;
    case noImplementationException_ : return noImplementationException_;
    case rpcException_ : return rpcException_;
    case rpcConnectionException_ : return rpcConnectionException_;
    case entityNotFoundException_ : return entityNotFoundException_;
    case entityNotDirException_ : return entityNotDirException_;
    case auditException_ : return auditException_;
    case unknownEntityIdException_ : return unknownEntityIdException_;
    case entityIdInUseException_ : return entityIdInUseException_;
    case entityLogMsgLenException_ : return entityLogMsgLenException_;
    case entityLogWriteToConstException_ : return entityLogWriteToConstException_;
    case errnoException_ : return errnoException_;
    case memoryLimitExceededException_ : return memoryLimitExceededException_;
    case noParentException_ : return noParentException_;
    default : throw Fwk::RangeException( "Fwk::Exception::Id" );
   }
}

Fwk::Exception::~Exception() {
}


Fwk::Exception::Id
Fwk::Exception::id() {
   return unknownException_;
}

Fwk::Exception::Id
Fwk::UnknownTypeException::id() {
   return unknownTypeException_;
}

Fwk::Exception::Id
Fwk::UnknownAttrException::id() {
   return unknownAttrException_;
}

Fwk::Exception::Id
Fwk::UnknownDelimiterException::id() {
   return unknownDelimiterException_;
}

Fwk::Exception::Id
Fwk::UnknownArgException::id() {
   return unknownArgException_;
}

Fwk::Exception::Id
Fwk::InternalException::id() {
   return internalException_;
}

Fwk::Exception::Id
Fwk::RangeException::id() {
   return rangeException_;
}

Fwk::Exception::Id
Fwk::MemoryException::id() {
   return memoryException_;
}

Fwk::Exception::Id
Fwk::StorageException::id() {
   return storageException_;
}

Fwk::Exception::Id
Fwk::TimeoutException::id() {
   return timeoutException_;
}

Fwk::Exception::Id
Fwk::NameInUseException::id() {
   return nameInUseException_;
}

Fwk::Exception::Id
Fwk::IllegalNameException::id() {
   return illegalNameException_;
}

Fwk::Exception::Id
Fwk::PermissionException::id() {
   return permissionException_;
}

Fwk::Exception::Id
Fwk::NoImplementationException::id() {
   return noImplementationException_;
}

Fwk::Exception::Id
Fwk::RpcException::id() {
   return rpcException_;
}

Fwk::Exception::Id
Fwk::RpcConnectionException::id() {
   return rpcConnectionException_;
}

Fwk::Exception::Id
Fwk::EntityNotFoundException::id() {
   return entityNotFoundException_;
}

Fwk::Exception::Id
Fwk::EntityNotDirException::id() {
   return entityNotDirException_;
}

Fwk::Exception::Id
Fwk::AuditException::id() {
   return auditException_;
}

Fwk::Exception::Id
Fwk::UnknownEntityIdException::id() {
   return unknownEntityIdException_;
}

Fwk::Exception::Id
Fwk::EntityIdInUseException::id() {
   return entityIdInUseException_;
}

Fwk::Exception::Id
Fwk::EntityLogMsgLenException::id() {
   return entityLogMsgLenException_;
}

Fwk::Exception::Id
Fwk::EntityLogWriteToConstException::id() {
   return entityLogWriteToConstException_;
}

Fwk::Exception::Id
Fwk::ErrnoException::id() {
   return errnoException_;
}

Fwk::Exception::Id
Fwk::ListException::id() {
   return listException_;
}

Fwk::Exception::Id
Fwk::MemoryLimitExceededException::id() {
   return memoryLimitExceededException_;
}
