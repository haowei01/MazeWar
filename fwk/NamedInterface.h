// Copyright (c) 1993-2007 David R. Cheriton, all rights reserved.
// PtrInterface.h

#ifndef FWK_NAMEDINTERFACE_H
#define FWK_NAMEDINTERFACE_H

#include "PtrInterface.h"
#include "BaseNotifiee.h"

namespace Fwk {

class NamedInterface : public PtrInterface<NamedInterface>
{
public:
	String name() const { return name_; }

	class NotifieeConst : virtual public RootNotifiee {
	public:
		typedef Fwk::Ptr<NotifieeConst const> PtrConst;
		typedef Fwk::Ptr<NotifieeConst> Ptr;
	};

	class Notifiee : virtual public NotifieeConst {
	public:
		typedef Fwk::Ptr<Notifiee const> PtrConst;
		typedef Fwk::Ptr<Notifiee> Ptr;
	};

protected:
	NamedInterface(const String& name) : name_(name) { }

private:
	String name_;
};

}

#endif
