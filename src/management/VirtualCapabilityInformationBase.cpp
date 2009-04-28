/******************************************************************************
 * WiFiMac                                                                    *
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 16, D-52074 Aachen, Germany
 * phone: ++49-241-80-27910,
 * fax: ++49-241-80-22242
 * email: info@openwns.org
 * www: http://www.openwns.org
 * _____________________________________________________________________________
 *
 * openWNS is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 2 as published by the
 * Free Software Foundation;
 *
 * openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

#include <WNS/Exception.hpp>
#include <WNS/pyconfig/Sequence.hpp>
#include <WNS/Ttos.hpp>

using namespace wifimac::management;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::management::VirtualCapabilityInformationBase,
	wns::node::component::Interface,
	"wifimac.management.VirtualCapabilityInformationBase",
	wns::node::component::ConfigCreator);


VirtualCapabilityInformationBaseService::VirtualCapabilityInformationBaseService():
	logger("WIFIMAC", "VCIB", wns::simulator::getMasterLogger()),
	vcib(NULL)
{
	MESSAGE_SINGLE(NORMAL, logger, "Starting VCIB Service.");
}

void
VirtualCapabilityInformationBaseService::setVCIB(VirtualCapabilityInformationBase* _vcib)
{
	assure(vcib == NULL, "setVCIB already called, cannot call twice");
	this->vcib = _vcib;
}

VirtualCapabilityInformationBase*
VirtualCapabilityInformationBaseService::getVCIB()
{
	assureNotNull(this->vcib);
	return(this->vcib);
}

VirtualCapabilityInformationBase::VirtualCapabilityInformationBase(wns::node::Interface* _node,	const wns::pyconfig::View& _config) :
	wns::node::component::Component(_node, _config),
	logger(_config.get("logger")),
    nodeInformationBase(new NodeBase),
    defaultValues(new InformationBase)
{
	TheVCIBService::Instance().setVCIB(this);
	MESSAGE_SINGLE(NORMAL, logger, "Starting Virtual Capability Information Base.");

    // currently, default information must have type int
    for (int i=0; i< _config.get<int>("defaultValues.size()"); ++i)
    {
        std::string s = "defaultValues.get("+ wns::Ttos(i) + ")";
        wns::pyconfig::Sequence entry = _config.getSequence(s);
        assure(entry.size() == 2, "Entry has wrong size");

        std::string key = entry.at<std::string>(0);
        int value = entry.at<int>(1);

        defaultValues->insert<int>(key, value);

        MESSAGE_SINGLE(NORMAL, logger, "Inserted key " << key << " with value " << value << " into default information base");
    }
}

bool
VirtualCapabilityInformationBase::knows(const wns::service::dll::UnicastAddress adr, const std::string& key) const
{
    if(defaultValues->knows(key))
    {
        // at least default information is available
        return true;
    }

    if(not nodeInformationBase->knows(adr))
    {
        // address is not known
        return false;
    }

    if(not nodeInformationBase->find(adr)->knows(key))
    {
        // address is known, but no information about the capability
        return false;
    }

    return true;
} // VirtualCapabilityInformationBase::knows

bool
VirtualCapabilityInformationBase::knows(const wns::service::dll::UnicastAddress adr) const
{
    if(nodeInformationBase->knows(adr))
    {
        // address is not known
        return true;
    }
    else
    {
        return false;
    }
}


InformationBase*
VirtualCapabilityInformationBase::getAll(const wns::service::dll::UnicastAddress adr) const
{
    assure(nodeInformationBase->knows(adr), "Address " << adr << "not stored in capability information base");

    InformationBase* myIB = nodeInformationBase->find(adr);

    return(myIB);
}
