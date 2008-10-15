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

#include <WIFIMAC/lowerMAC/Aggregation.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::lowerMAC::Aggregation,
	wns::ldk::FunctionalUnit,
	"wifimac.lowerMAC.Aggregation",
	wns::ldk::FUNConfigCreator);

Aggregation::Aggregation(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::concatenation::Concatenation(fun, config_),

    managerName(config_.get<std::string>("managerName")),
    impatientTransmission(config_.get<bool>("myConfig.impatientTransmission")),
    maxDelay(config_.get<wns::simulator::Time>("myConfig.maxDelay")),
    sendNow(false),
    currentReceiver()
{
	MESSAGE_SINGLE(NORMAL, logger, "created");

    friends.manager = NULL;
}


void Aggregation::onFUNCreated()
{
	MESSAGE_SINGLE(NORMAL, logger, "onFUNCreated() started");

    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
}

void Aggregation::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(friends.manager->getFrameType(compound->getCommandPool()) != ACK, "Cannot handle ACK frames");

    if(not (this->currentCompound == wns::ldk::CompoundPtr()))
    {
        // ongoing aggregation
        assure(this->currentReceiver.isValid(), "Has something to send, but not valid receiver");

        if(friends.manager->getReceiverAddress(compound->getCommandPool()) == this->currentReceiver)
        {
            if(friends.manager->getRequiresDirectReply(compound->getCommandPool()))
            {
                // requires reply, close the aggregation
                this->currentEntries = this->maxEntries-1;
                MESSAGE_BEGIN(NORMAL, logger, m, "Compound to ");
                m << friends.manager->getReceiverAddress(compound->getCommandPool());
                m << " requires direct reply -> Close aggregation, fake entries to " << this->currentEntries;
                MESSAGE_END();
                // mark current compound in the same way
                friends.manager->setRequiresDirectReply(this->currentCompound->getCommandPool(), true);
                sendNow = true;
            }
            // accepted compound is for the same receiver as the other ones -->
            // no differentiation
            wns::ldk::concatenation::Concatenation::processOutgoing(compound);
            if((not hasCapacity()) or impatientTransmission)
            {
                // no more capacity -> send, even if patient aggregation
                sendNow = true;
            }
        }
        else
        {
            activateCommand(compound->getCommandPool());

            // accepted compound is for a different receiver --> same as buffer
            // full
            this->nextCompound = createContainer(compound);
            this->sendNow = true;
            MESSAGE_BEGIN(NORMAL, logger, m, "Compound is for receiver ");
            m << friends.manager->getReceiverAddress(compound->getCommandPool());
            m << ", current receiver is " << this->currentReceiver;
            m << " -> No concatenation possible";
            MESSAGE_END();

            this->currentReceiver = friends.manager->getReceiverAddress(compound->getCommandPool());
        }
    }
    else
    {
        this->currentReceiver = friends.manager->getReceiverAddress(compound->getCommandPool());
        if((friends.manager->getFrameType(compound->getCommandPool()) == ACK) or impatientTransmission)
        {
            MESSAGE_BEGIN(NORMAL, logger, m, "New aggregation for receiver ");
            m << this->currentReceiver;
            m << " send impatiently";
            MESSAGE_END();

            this->sendNow = true;
        }
        else
        {
            MESSAGE_BEGIN(NORMAL, logger, m, "New aggregation for receiver ");
            m << this->currentReceiver;
            m << ", wait for more";
            MESSAGE_END();

            setTimeout(this->maxDelay);
            this->sendNow = false;
        }

        wns::ldk::concatenation::Concatenation::processOutgoing(compound);
    }
}

void
Aggregation::onTimeout()
{
    MESSAGE_SINGLE(NORMAL, logger, "Timeout for current aggregation -> send");
    sendNow = true;
    tryToSend();
}

const wns::ldk::CompoundPtr
Aggregation::hasSomethingToSend() const
{
    // wait for sendNow indication
    if(not sendNow)
    {
        return wns::ldk::CompoundPtr();
    }

    return wns::ldk::concatenation::Concatenation::hasSomethingToSend();

} // somethingToSend

wns::ldk::CompoundPtr
Aggregation::getSomethingToSend()
{
    wns::ldk::CompoundPtr it = wns::ldk::concatenation::Concatenation::getSomethingToSend();
    // reset sendNow for next aggregation
    sendNow = false;
    if(hasTimeoutSet())
    {
        cancelTimeout();
    }
    return(it);
}
