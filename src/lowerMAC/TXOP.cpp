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

#include <WIFIMAC/lowerMAC/TXOP.hpp>
#include <WIFIMAC/FrameType.hpp>

#include <WNS/probe/bus/utils.hpp>

#include <algorithm>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    TXOP,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.TXOP",
    wns::ldk::FUNConfigCreator);

TXOP::TXOP(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<TXOP, wns::ldk::EmptyCommand>(fun),
    managerName(config_.get<std::string>("managerName")),
    protocolCalculatorName(config_.get<std::string>("protocolCalculatorName")),
    txopWindowName(config_.get<std::string>("txopWindowName")),
    raName(config_.get<std::string>("raName")),
    sifsDuration(config_.get<wns::simulator::Time>("myConfig.sifsDuration")),
    maximumACKDuration(config_.get<wns::simulator::Time>("myConfig.maximumACKDuration")),
    txopLimit(config_.get<wns::simulator::Time>("myConfig.txopLimit")),
    singleReceiver(config_.get<bool>("myConfig.singleReceiver")),
    maxOutTXOP(config_.get<bool>("myConfig.maxOutTXOP")),
   remainingTXOPDuration(0),
    txopReceiver(),
    logger(config_.get("logger"))
{
    // either no TXOP (==0) or TXOP limit long enough to contain SIFS duration and ACK response
/*    if (txopLimit > 0)
    {
        assure (txopLimit > sifsDuration + maximumACKDuration,
                "TXOP limit too small (must be greater than SIFS duration + maximum ACK Duration or 0 for no TXOP)");
    }
*/    MESSAGE_SINGLE(NORMAL, this->logger, "created");

    friends.manager = NULL;
    protocolCalculator = NULL;
    friends.txopWindow = NULL;
    friends.ra = NULL;

    // read the localIDs from the config
    wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config_.len("localIDs.keys()"); ++ii)
    {
        std::string key = config_.get<std::string>("localIDs.keys()",ii);
        unsigned int value  = config_.get<unsigned int>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }
    TXOPDurationProbe = wns::probe::bus::collector(localContext, config_, "TXOPDurationProbeName");
}


TXOP::~TXOP()
{
}

void TXOP::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.manager = getFUN()->findFriend<Manager*>(managerName);
    friends.txopWindow = getFUN()->findFriend<ITXOPWindow*>(txopWindowName);
    friends.ra = getFUN()->findFriend<RateAdaptation*>(raName);
    protocolCalculator = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
}

void
TXOP::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    // reset frame type if necessary
    if(friends.manager->getFrameType(compound->getCommandPool()) == DATA_TXOP)
    {
        friends.manager->setFrameType(compound->getCommandPool(), DATA);
    }
}




void
TXOP::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    switch(friends.manager->getFrameType(compound->getCommandPool()))
    {
    case DATA:
    {
        if(this->remainingTXOPDuration == 0)
        {
            // start of the TXOP
            this->remainingTXOPDuration = this->txopLimit;
            if(singleReceiver)
            {
                this->txopReceiver = friends.manager->getReceiverAddress(compound->getCommandPool());
            }
            MESSAGE_BEGIN(NORMAL, this->logger, m, "Outgoing data compound to ");
            m << friends.manager->getReceiverAddress(compound->getCommandPool());
            m << ", starting TXOP with duration " << this->remainingTXOPDuration;
            MESSAGE_END();
        }
        else
        {
            // we have an ongoing TXOP

            // no need check if frame fits into current TXOP:
            // Either, this frame is the expected frame and fits perfectly, or
            // the frame is a retransmission which is allowed to be send without
            // looking at the txop limit
            friends.manager->setFrameType(compound->getCommandPool(), DATA_TXOP);
            MESSAGE_BEGIN(NORMAL, this->logger, m, "Outgoing data compound to ");
            m << friends.manager->getReceiverAddress(compound->getCommandPool());
            m << ", continue TXOP with duration " << this->remainingTXOPDuration;
            MESSAGE_END();
        }

	 // cut TXOP duration by current frame
        wifimac::convergence::PhyMode phyMode = friends.manager->getPhyMode(compound->getCommandPool());
        wns::simulator::Time duration = protocolCalculator->getDuration()->MPDU_PPDU(compound->getLengthInBits(),
                                                                                     phyMode);

        this->remainingTXOPDuration = this->remainingTXOPDuration
                                        - duration
                                        - this->sifsDuration
                                        - this->maximumACKDuration;

        MESSAGE_SINGLE(NORMAL, this->logger, "Current compound cuts TXOP to " << this->remainingTXOPDuration);

        if(this->remainingTXOPDuration <= 0)
        {
            // no time for additional frames -> no (more) TXOP
            MESSAGE_SINGLE(NORMAL, this->logger, "Current compound fills complete TXOP");
            closeTXOP();
	    return;
        }

        // check if next frame would fit into TXOP
        wns::simulator::Time nextDuration = friends.txopWindow->getNextTransmissionDuration();

        if(nextDuration == wns::simulator::Time())
        {
            // no next compound, no (more) TXOP
            MESSAGE_SINGLE(NORMAL, this->logger, "No next compound, no (more) TXOP");
	    closeTXOP();
            return;
        }

        if(singleReceiver and (this->txopReceiver != friends.txopWindow->getNextReceiver()))
        {
            MESSAGE_BEGIN(NORMAL, this->logger, m, "TXOP is restricted to receiver ");
            m << this->txopReceiver << " and next compound is addressed to ";
            m << friends.txopWindow->getNextReceiver();
            m << "no (more) TXOP";
            MESSAGE_END();
	    closeTXOP();
            return;
        }

        wns::simulator::Time nextFrameExchangeDuration;

        if (not maxOutTXOP)
        {
            nextFrameExchangeDuration =  this->sifsDuration
                + nextDuration
                + this->sifsDuration
                + this->maximumACKDuration;
        }
        else
        {
            nextFrameExchangeDuration = this->remainingTXOPDuration;
        }

        if (this->remainingTXOPDuration < nextFrameExchangeDuration)
        {
            // next frame does not fit -> no (more) TXOP
            MESSAGE_SINGLE(NORMAL, this->logger, "Next frame has duration " << nextFrameExchangeDuration << ", does not fit");
	    closeTXOP();
            return;
        }

        // next frame fits -> extend frame exchange duration by complete
        // next frame exchange
        friends.manager->setFrameExchangeDuration(compound->getCommandPool(),
                                                  friends.manager->getFrameExchangeDuration(compound->getCommandPool()) + nextFrameExchangeDuration);

        MESSAGE_BEGIN(NORMAL, this->logger, m,  "Next frame has duration ");
        m << nextFrameExchangeDuration;
        m << ", fit into TXOP, set NAV to ";
        m << friends.manager->getFrameExchangeDuration(compound->getCommandPool()) + nextFrameExchangeDuration;
        MESSAGE_END();

        break;
    }

    case ACK:
    {
        // do not change the ACK in any way before transmission
        break;
    }
    default:
    {
        throw wns::Exception("Unknown frame type");
        break;
    }
    }


}

bool TXOP::doIsAccepting(const wns::ldk::CompoundPtr& compound) const
{
    if((friends.manager->getFrameType(compound->getCommandPool()) == DATA) and
       (this->remainingTXOPDuration > 0))
    {
        // we have an ongoing TXOP

        // no need check if frame fits into current TXOP:
        // Either, this frame is the expected frame and fits perfectly, or
        // the frame is a retransmission which is allowed to be send without
        // looking at the txop limit

        // copy the compound and change type so that the right path in the FUN
        // is taken
        wns::ldk::CompoundPtr txopCompound = compound->copy();
        friends.manager->setFrameType(txopCompound->getCommandPool(), DATA_TXOP);

        return wns::ldk::Processor<TXOP>::doIsAccepting(txopCompound);
    }
    else
    {
        // no special handling
        return wns::ldk::Processor<TXOP>::doIsAccepting(compound);
    }
}


void TXOP::closeTXOP() 
{
    this->remainingTXOPDuration = 0;
    TXOPDurationProbe->put(this->txopLimit - this->remainingTXOPDuration);
    for(int i=0; i < observers.size();i++)
    {
	observers[i]->onTXOPClosed();
    }
}


void TXOP::setTXOPLimit(wns::simulator::Time limit)
{
/*    if (limit > 0)
    {
        assure (txopLimit > sifsDuration + maximumACKDuration,
                "TXOP limit too small (must be greater than SIFS duration + maximum ACK Duration or 0 for no TXOP)");
    }
*/    this->txopLimit = limit;
    this->remainingTXOPDuration = 0;
}
