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
#include <DLL/Layer2.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::NextFrameGetter,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.NextFrameGetter",
    wns::ldk::FUNConfigCreator);

NextFrameGetter::NextFrameGetter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
            wns::ldk::fu::Plain<NextFrameGetter, wns::ldk::EmptyCommand>(fun),
            storage(),
	    protocolCalculatorName(config_.get<std::string>("protocolCalculatorName")),
            inWakeup(false)
{
    protocolCalculator = NULL;
}

void NextFrameGetter::onFUNCreated()
{
    protocolCalculator = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
}

void
NextFrameGetter::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    getDeliverer()->getAcceptor(compound)->onData(compound);
} // processIncoming


void
NextFrameGetter::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(hasCapacity(), "received PDU although not accepting.");

    storage = compound;
} // processOutgoing


bool
NextFrameGetter::hasCapacity() const
{
    return (storage == wns::ldk::CompoundPtr());
} // hasCapacity


const wns::ldk::CompoundPtr
NextFrameGetter::hasSomethingToSend() const
{
    return storage;
} // hasSomethingToSend

wns::ldk::CompoundPtr
NextFrameGetter::getSomethingToSend()
{
    assure(hasSomethingToSend(), "Called getSomethingToSend, but nothing to send");
    wns::ldk::CompoundPtr it = storage;
    storage = wns::ldk::CompoundPtr();
    return it;
} // getSomethingToSend

Bit 
NextFrameGetter::getNextSize() const
{
	if (storage == wns::ldk::CompoundPtr())
	{
		return 0;
	}
	return protocolCalculator->getFrameLength()->getPSDU(storage->getLengthInBits());
}

void
NextFrameGetter::tryToSend()
{
    // do not allow recursion
    if(inWakeup)
    {
        return;
    }

    // check if the next compound can be send
    const wns::ldk::CompoundPtr testCompound = hasSomethingToSend();
    if(testCompound == wns::ldk::CompoundPtr())
        return;
    if(not getConnector()->hasAcceptor(testCompound))
        return;

    CompoundHandlerInterface* target = getConnector()->getAcceptor(testCompound);
    // store the compound for sending
    wns::ldk::CompoundPtr compound = getSomethingToSend();

    // now the buffer is empty. signal to FU above
    if(hasCapacity())
    {
        inWakeup = true;
        getReceptor()->wakeup();
        inWakeup = false;
    }

    // now, send the stored compound
    target->sendData(compound);

    if(hasSomethingToSend() != wns::ldk::CompoundPtr())
    {
        tryToSend();
    }
} // tryToSend
