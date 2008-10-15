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

#include <WIFIMAC/lowerMAC/timing/ConstantWait.hpp>

using namespace wifimac::lowerMAC::timing;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::timing::ConstantWait,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.timing.ConstantWait",
    wns::ldk::FUNConfigCreator);

ConstantWait::ConstantWait(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<ConstantWait, wns::ldk::EmptyCommand>(fun),
    wns::ldk::Delayed<ConstantWait>(),
    currentFrame(),
    managerName(config_.get<std::string>("managerName")),
    sifsDuration(config_.get<wns::simulator::Time>("myConfig.sifsDuration")),
    logger(config_.get("logger"))
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");

    friends.manager = NULL;
}


ConstantWait::~ConstantWait()
{
}

void ConstantWait::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
}

void ConstantWait::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    getDeliverer()->getAcceptor(compound)->onData(compound);
}


void ConstantWait::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(hasCapacity(), "called processOutgoing although not capacity");
    MESSAGE_SINGLE(NORMAL, this->logger, "Ougoing frame, delay for " << sifsDuration);
    this->currentFrame = compound;
    this->setTimeout(sifsDuration);
}

bool ConstantWait::hasCapacity() const
{
    // There is room for exactly one compound
    return this->currentFrame == wns::ldk::CompoundPtr();
}

const wns::ldk::CompoundPtr ConstantWait::hasSomethingToSend() const
{
    if(hasTimeoutSet())
    {
        return(wns::ldk::CompoundPtr());
    }
    else
    {
        return(this->currentFrame);
    }
}

wns::ldk::CompoundPtr ConstantWait::getSomethingToSend()
{
    assure(hasSomethingToSend(), "Called getSomethingToSend without something to send");
    wns::ldk::CompoundPtr it = this->currentFrame;
    this->currentFrame = wns::ldk::CompoundPtr();

	MESSAGE_BEGIN(NORMAL, logger, m, "Send frame with");
	m << " tx: " << friends.manager->getTransmitterAddress(it->getCommandPool());
	m << " rx: " << friends.manager->getReceiverAddress(it->getCommandPool());
	MESSAGE_END();

    return it;
}

void
ConstantWait::onTimeout()
{
    tryToSend();
}

