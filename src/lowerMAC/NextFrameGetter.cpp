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

#include <WIFIMAC/lowerMAC/NextFrameGetter.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::lowerMAC::NextFrameGetter,
	wns::ldk::FunctionalUnit,
	"wifimac.lowerMAC.NextFrameGetter",
	wns::ldk::FUNConfigCreator);


void
NextFrameGetter::processIncoming(const wns::ldk::CompoundPtr& compound)
{
	getDeliverer()->getAcceptor(compound)->onData(compound);
} // processIncoming


void
NextFrameGetter::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
	assure(hasCapacity(), "received PDU although not accepting.");

    storage.push_back(compound);
    flowTime = wns::simulator::getEventScheduler()->getTime();

} // processOutgoing


bool
NextFrameGetter::hasCapacity() const
{
	return (storage.empty() or (wns::simulator::getEventScheduler()->getTime() == flowTime));
} // hasCapacity


const wns::ldk::CompoundPtr
NextFrameGetter::hasSomethingToSend() const
{
    if(storage.empty())
    {
        return(wns::ldk::CompoundPtr());
    }
    else
    {
        return (*(storage.begin()));
    }
} // hasSomethingToSend

wns::ldk::CompoundPtr
NextFrameGetter::getSomethingToSend()
{
    assure(hasSomethingToSend(), "Called getSomethingToSend, but nothing to send");
	wns::ldk::CompoundPtr it = *(storage.begin());
    storage.pop_front();
    flowTime = wns::simulator::getEventScheduler()->getTime();
	return it;
} // getSomethingToSend

