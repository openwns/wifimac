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

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::TXOP,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.TXOP",
    wns::ldk::FUNConfigCreator);

TXOP::TXOP(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<TXOP, wns::ldk::EmptyCommand>(fun),
    managerName(config_.get<std::string>("managerName")),
    phyUserName(config_.get<std::string>("phyUserName")),
    nextFrameHolderName(config_.get<std::string>("nextFrameHolderName")),
    raName(config_.get<std::string>("raName")),

    sifsDuration(config_.get<wns::simulator::Time>("myConfig.sifsDuration")),
    expectedACKDuration(config_.get<wns::simulator::Time>("myConfig.expectedACKDuration")),
    txopLimit(config_.get<wns::simulator::Time>("myConfig.txopLimit")),
    singleReceiver(config_.get<bool>("myConfig.singleReceiver")),
    remainingTXOPDuration(0),
    txopReceiver(),
    logger(config_.get("logger"))
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");

    friends.manager = NULL;
    friends.phyUser = NULL;
    friends.nextFrameHolder = NULL;
    friends.ra = NULL;
}


TXOP::~TXOP()
{
}

void TXOP::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
    friends.phyUser = getFUN()->findFriend<wifimac::convergence::PhyUser*>(phyUserName);
    friends.nextFrameHolder = getFUN()->findFriend<wns::ldk::DelayedInterface*>(nextFrameHolderName);
    friends.ra = getFUN()->findFriend<wifimac::lowerMAC::RateAdaptation*>(raName);
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
        this->remainingTXOPDuration = this->remainingTXOPDuration
            - friends.phyUser->getPSDUDuration(compound)
            - this->sifsDuration
            - this->expectedACKDuration;

        MESSAGE_SINGLE(NORMAL, this->logger, "Current compound cuts TXOP to " << this->remainingTXOPDuration);

        if(this->remainingTXOPDuration <= 0)
        {
            // no time for additional frames -> no (more) TXOP
            this->remainingTXOPDuration = 0;
            MESSAGE_SINGLE(NORMAL, this->logger, "Current compound fills complete TXOP");
            return;
        }

        // check if next frame would fit into TXOP
        const wns::ldk::CompoundPtr nextCompound = friends.nextFrameHolder->hasSomethingToSend();

        if(not nextCompound)
        {
            // no next compound, no (more) TXOP
            this->remainingTXOPDuration = 0;
            MESSAGE_SINGLE(NORMAL, this->logger, "No next compound, no (more) TXOP");
            return;
        }

        if(singleReceiver and (this->txopReceiver != friends.manager->getReceiverAddress(nextCompound->getCommandPool())))
        {
            this->remainingTXOPDuration = 0;
            MESSAGE_BEGIN(NORMAL, this->logger, m, "TXOP is restricted to receiver ");
            m << this->txopReceiver << " and next compound is addressed to ";
            m << friends.manager->getReceiverAddress(nextCompound->getCommandPool());
            m << "no (more) TXOP";
            MESSAGE_END();
            return;
        }

        wns::simulator::Time nextFrameExchangeDuration =
            this->sifsDuration
            + friends.phyUser->getPSDUDuration(nextCompound, friends.ra->getPhyMode(nextCompound))
            + this->sifsDuration
            + this->expectedACKDuration;

        if (this->remainingTXOPDuration < nextFrameExchangeDuration)
        {
            // next frame does not fit -> no (more) TXOP
            this->remainingTXOPDuration = 0;
            MESSAGE_SINGLE(NORMAL, this->logger, "Next frame has duration " << nextFrameExchangeDuration << ", does not fit");
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
