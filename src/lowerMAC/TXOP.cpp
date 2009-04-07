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
    expectedACKDuration(config_.get<wns::simulator::Time>("myConfig.expectedACKDuration")),
    txopLimit(config_.get<wns::simulator::Time>("myConfig.txopLimit")),
    singleReceiver(config_.get<bool>("myConfig.singleReceiver")),
    impatient(config_.get<bool>("myConfig.impatient")),
    remainingTXOPDuration(0),
    txopReceiver(),
    logger(config_.get("logger"))
{
    // either no TXOP (==0) or TXOP limit long enough to contain SIFS duration and ACK response
    if (txopLimit > 0)
    {
        assure (txopLimit > sifsDuration + expectedACKDuration,
                "TXOP limit too small (must be greater than SIFS duration + expected ACK Duration or 0 for no TXOP)");
    }
    MESSAGE_SINGLE(NORMAL, this->logger, "created");

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
    protocolCalculator = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);

    // initial triggering of timeWindow FU letting thru compounds
    if (impatient == true) 
    {
    	friends.txopWindow->setDuration(std::max(this->txopLimit - this->sifsDuration - this->expectedACKDuration,0.0));
    }
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
    wns::simulator::Time duration = 0;
    wifimac::convergence::PhyMode phyMode;
 
    switch(friends.manager->getFrameType(compound->getCommandPool()))
    {
    case DATA:
    {
        if((this->remainingTXOPDuration == 0) and (impatient == true))
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
	    assure(this->remainingTXOPDuration > 0,"tried to send data outside TXOP window");
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
            this->remainingTXOPDuration = this->remainingTXOPDuration - this->sifsDuration;
        }

        // cut TXOP duration by current frame
        phyMode = friends.manager->getPhyMode(compound->getCommandPool());
        duration = protocolCalculator->getDuration()->getMPDU_PPDU(compound->getLengthInBits(),
                                                                   phyMode.getDataBitsPerSymbol(),
                                                                   phyMode.getNumberOfSpatialStreams(),
                                                                   20, std::string("Basic"));

        this->remainingTXOPDuration = this->remainingTXOPDuration
            - duration
            - this->sifsDuration
            - this->expectedACKDuration;

        if(this->remainingTXOPDuration <= 2 * this->sifsDuration + this->expectedACKDuration)
        {
            // no time for additional frames -> no (more) TXOP
            this->remainingTXOPDuration = 0;
            MESSAGE_SINGLE(NORMAL, this->logger, "Current compound fills complete TXOP");
            TXOPDurationProbe->put(std::max(this->txopLimit - this->sifsDuration - this->expectedACKDuration,0.0));
            // reset duration of time window FU -> let frames leave buffer for new TXOP round
	    if (impatient == true) 
	    {
	            friends.txopWindow->setDuration(std::max(this->txopLimit - this->sifsDuration - this->expectedACKDuration,0.0));
	    }
            return;
        }


        // check for next frame data
        duration = friends.txopWindow->getActualDuration(this->remainingTXOPDuration - 2*this->sifsDuration - this->expectedACKDuration);

        if(duration == 0)
        {
            // no next compound, no (more) TXOP
            this->remainingTXOPDuration = 0;
            MESSAGE_SINGLE(NORMAL, this->logger, "No next compound, no (more) TXOP");
            // reset duration of time window FU -> let frames leave buffer for new TXOP round
            TXOPDurationProbe->put(this->txopLimit - this->remainingTXOPDuration);
            // reset duration of time window FU -> let frames leave buffer for new TXOP round
	    if (impatient == true) 
	    {
	            friends.txopWindow->setDuration(std::max(this->txopLimit - this->sifsDuration - this->expectedACKDuration,0.0));
	    }
            return;
        }

        const wns::ldk::CompoundPtr nextCompound = friends.txopWindow->hasSomethingToSend();
        if(singleReceiver and (this->txopReceiver != friends.manager->getReceiverAddress(nextCompound->getCommandPool())))
        {
            this->remainingTXOPDuration = 0;
            MESSAGE_BEGIN(NORMAL, this->logger, m, "TXOP is restricted to receiver ");
            m << this->txopReceiver << " and next compound is addressed to ";
            m << friends.manager->getReceiverAddress(nextCompound->getCommandPool());
            m << "no (more) TXOP";
            MESSAGE_END();
            TXOPDurationProbe->put(this->txopLimit - this->remainingTXOPDuration);
            // reset duration of time window FU -> let frames leave buffer for new TXOP round
	    if (impatient == true) 
	    {
	            friends.txopWindow->setDuration(std::max(this->txopLimit - this->sifsDuration - this->expectedACKDuration,0.0));
	    }
            return;
        }
        wns::simulator::Time nextFrameExchangeDuration =
            this->sifsDuration
            + duration
            + this->sifsDuration
            + this->expectedACKDuration;

        // next frame fits -> extend frame exchange duration by complete
        // next frame exchange
        // next frame always fits, thanx to timeWindow FU
        friends.manager->setFrameExchangeDuration(compound->getCommandPool(),
                                                  friends.manager->getFrameExchangeDuration(compound->getCommandPool()) + nextFrameExchangeDuration);

        MESSAGE_BEGIN(NORMAL, this->logger, m,  "Next frame has duration ");
        m << nextFrameExchangeDuration;
        m << ", fit into TXOP, set NAV to ";
        m << friends.manager->getFrameExchangeDuration(compound->getCommandPool()) + nextFrameExchangeDuration;
        MESSAGE_END();
        friends.txopWindow->setDuration(this->remainingTXOPDuration - 2*this->sifsDuration - this->expectedACKDuration);
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

void TXOP::startTXOP(wns::simulator::Time duration) 
{
	if (friends.txopWindow->getActualDuration(duration - this->sifsDuration - this->expectedACKDuration) > 0)
	{
		this->txopLimit = duration;
		this->remainingTXOPDuration = this->txopLimit;
		friends.txopWindow->setDuration(std::max(this->txopLimit - this->sifsDuration - this->expectedACKDuration,0.0));
	}
}
