#ifndef EXCEPTION_H
#define EXCEPTION_H
#include <string>
#include "fwk/Ptr.h"
#include "fwk/PtrInterface.h"
#include <iostream>
using namespace std;

//Base exception type.
class Exception {
public:
	string what() const {return what_;}
	virtual ~Exception(){};
    Exception( char const * str ) : what_(str) {}
    Exception( string str ) : what_(str) {}

private:
	string what_;
};


class RangeException : public Exception {
public:
	RangeException(string info) : Exception(info){};
	~RangeException(){};

};


class NameInUseException : public Exception {
public:
	NameInUseException(string info) : Exception(info){};
	~NameInUseException(){};
};

class PermissionException : public Exception{
public:
	PermissionException(string info) : Exception(info){};
	~PermissionException(){};
};

class NoImplementationException : public Exception {
public:
	NoImplementationException(string info) : Exception(info){};
	~NoImplementationException(){};
};

class AttributeNotSupportedException : public NoImplementationException {
public:
	AttributeNotSupportedException(string info) : NoImplementationException(info){};
	~AttributeNotSupportedException(){};
};

class EntityNotFoundException : public Exception {
public:
   EntityNotFoundException(string info) : Exception(info) {};
   ~EntityNotFoundException(){};
};



#endif

