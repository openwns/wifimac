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

#include <WIFIMAC/lowerMAC/RTSCTS.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::RTSCTS,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.RTSCTS",
    wns::ldk::FUNConfigCreator);

RTSCTS::RTSCTS(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<RTSCTS, RTSCTSCommand>(fun),

    phyUserName(config_.get<std::string>("phyUserName")),
    managerName(config_.get<std::string>("managerName")),
    arqName(config_.get<std::string>("arqName")),
    navName(config_.get<std::string>("navName")),
    rxsName(config_.get<std::string>("rxStartName")),
    txStartEndName(config_.get<std::string>("txStartEndName")),

    sifsDuration(config_.get<wns::simulator::Time>("myConfig.sifsDuration")),
    expectedACKDuration(config_.get<wns::simulator::Time>("myConfig.expectedACKDuration")),
    expectedCTSDuration(config_.get<wns::simulator::Time>("myConfig.expectedCTSDuration")),
    preambleProcessingDelay(config_.get<wns::simulator::Time>("myConfig.preambleProcessingDelay")),
    phyModeId(config_.get<int>("myConfig.rtsctsPhyModeId")),
    rtsBits(config_.get<Bit>("myConfig.rtsBits")),
    ctsBits(config_.get<Bit>("myConfig.ctsBits")),
    rtsctsThreshold(config_.get<Bit>("myConfig.rtsctsThreshold")),
    rtsctsOnTxopData(config_.get<bool>("myConfig.rtsctsOnTxopData")),

    nav(false),
    navSetter(),
    logger(config_.get("logger")),

    pendingRTS(),
    pendingCTS(),
    pendingMSDU(),

    state(idle)
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created, threshold: " << rtsctsThreshold << "b");

    friends.phyUser = NULL;
    friends.manager = NULL;
    friends.arq = NULL;

    this->ctsPrepared = 0;
    this->lastTimeout = 0;
}


RTSCTS::~RTSCTS()
{
}

void RTSCTS::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");
 
    friends.phyUser = getFUN()->findFriend<wifimac::convergence::PhyUser*>(phyUserName);
    assure(friends.phyUser, "Could not get phyUser from my FUN");

    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

    friends.arq = getFUN()->findFriend<wifimac::lowerMAC::ITransmissionCounter*>(arqName);

    // Observe NAV
    this->wns::Observer<INetworkAllocationVector>::startObserving(getFUN()->findFriend<wifimac::convergence::NAVNotification*>(navName));

    // Observe rxStartEnd
    this->wns::Observer<wifimac::convergence::IRxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::RxStartEndNotification*>(rxsName));

    // Observe txStartEnd
    this->wns::Observer<wifimac::convergence::ITxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::TxStartEndNotification*>(txStartEndName));

}

void
RTSCTS::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    if(getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this))
    {
        if(getCommand(compound->getCommandPool())->peer.isRTS)
        {
            if(nav)
            {
                if(friends.manager->getTransmitterAddress(compound->getCommandPool()) == navSetter)
                {
                    MESSAGE_BEGIN(NORMAL, this->logger, m, "Incoming RTS from ");
                    m << friends.manager->getTransmitterAddress(compound->getCommandPool());
                    m << ", nav busy from " << navSetter;
                    m << " -> reply with CTS";
                    MESSAGE_END();

                    assure(this->pendingCTS == wns::ldk::CompoundPtr(),
                           "Old pending CTS not transmitted");
                    this->pendingCTS = this->prepareCTS(compound);
                }
                else
                {
                    MESSAGE_BEGIN(NORMAL, this->logger, m, "Incoming RTS from ");
                    m << friends.manager->getTransmitterAddress(compound->getCommandPool());
                    m << ", nav busy from " << navSetter;
                    m << " -> Drop";
                    MESSAGE_END();
                }
            } // nav
            else
            {
                MESSAGE_SINGLE(NORMAL, this->logger, "Incoming RTS, nav idle -> reply with CTS");

                assure(this->pendingCTS == wns::ldk::CompoundPtr(),
                       "Old pending CTS not transmitted");
                this->pendingCTS = this->prepareCTS(compound);
            } // not nav
        } // is RTS
        else
        {
            // received CTS on transmitted RTS --> successfully reserved the channel
            // for data
            assure(this->pendingMSDU, "Received CTS, but no pending MSDU");
            if(state == idle)
            {
                std::cout << "received CTS although state is idle, now: " << wns::simulator::getEventScheduler()->getTime() << ", last timeout: " << this->lastTimeout << "\n";
                if(state == transmitRTS)
                    std::cout << "state is transmitRTS\n";
                if(state == waitForCTS)
                    std::cout << "state is waitForCTS\n";
                if(state == receiveCTS)
                    std::cout << "state is receiveCTS\n";
            }
            assure(state != idle, "received CTS although state is idle, now: " << wns::simulator::getEventScheduler()->getTime() << ", last timeout: " << this->lastTimeout);
            if((state == idle)
                or
               (friends.manager->getTransmitterAddress(compound->getCommandPool())
                != friends.manager->getReceiverAddress(this->pendingMSDU->getCommandPool())))
            {
                MESSAGE_SINGLE(NORMAL, this->logger,
                               "Incoming CTS does not match the receiver's address on the pending MSDU -> do nothing");
            }
            else
            {
                assure(this->pendingMSDU, "Received CTS, but no pending MSDU");
                MESSAGE_SINGLE(NORMAL, this->logger,
                               "Incoming awaited CTS -> send data");
                state = idle;
                if(this->hasTimeoutSet())
                {
                    this->cancelTimeout();
                }
            }
        } // is not RTS
    } // is activated
    else
    {
        // deliver frame
        MESSAGE_SINGLE(NORMAL, this->logger, "Received frame -> deliver");
        getDeliverer()->getAcceptor(compound)->onData(compound);
    } // no RTSCTS command
}

void
RTSCTS::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(this->pendingMSDU == wns::ldk::CompoundPtr(),
           "Cannot have two MSDUs");
    assure(this->pendingRTS == wns::ldk::CompoundPtr(),
           "Cannot have two RTSs");

    this->pendingMSDU = compound;

    switch(friends.manager->getFrameType(compound->getCommandPool()))
    {
    case DATA_TXOP:
        if(not this->rtsctsOnTxopData)
        {
            break;
        }
        // fall through to DATA if RTS/CTS during TXOP is activ
    case DATA:
        if(compound->getLengthInBits() < this->rtsctsThreshold)
        {
            MESSAGE_SINGLE(NORMAL, this->logger,
                           "Outgoing DATA with size " << compound->getLengthInBits() << ", below threshold");
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, this->logger,
                           "Outgoing DATA with size " << compound->getLengthInBits() << "-> Save and send RTS");
            this->pendingRTS = this->prepareRTS(this->pendingMSDU);

            // RTS/CTS initializes mini-TXOP for compound, it can be send
            // directly after SIFS
            friends.manager->setFrameType(this->pendingMSDU->getCommandPool(), DATA_TXOP);
        }
        break;
    default:
        throw wns::Exception("Unknown frame type");
        break;
    }
}

const wns::ldk::CompoundPtr
RTSCTS::hasSomethingToSend() const
{

    if(this->pendingCTS)
    {
        return(this->pendingCTS);
    }
    if(this->pendingRTS)
    {
        return(this->pendingRTS);
    }
    if(state == idle)
    {
        return(this->pendingMSDU);
    }
    return wns::ldk::CompoundPtr();

}

wns::ldk::CompoundPtr
RTSCTS::getSomethingToSend()
{
    assure(this->hasSomethingToSend(), "Called getSomethingToSend without pending compound");

    wns::ldk::CompoundPtr it;
    if(this->pendingCTS)
    {
        it = this->pendingCTS;
        assure(this->ctsPrepared == wns::simulator::getEventScheduler()->getTime(),
               "ctsPrepared is " << this->ctsPrepared << ", but time is " <<  wns::simulator::getEventScheduler()->getTime());
        this->pendingCTS = wns::ldk::CompoundPtr();
        MESSAGE_SINGLE(NORMAL, this->logger, "Sending CTS frame to "<< friends.manager->getReceiverAddress(it->getCommandPool()));
        return(it);
    }
    if(this->pendingRTS)
    {
        it = this->pendingRTS;
        this->pendingRTS = wns::ldk::CompoundPtr();
        state = transmitRTS;
        MESSAGE_SINGLE(NORMAL, this->logger, "Send RTS frame to "<< friends.manager->getReceiverAddress(it->getCommandPool()));
        return(it);
    }

    it = this->pendingMSDU;
    this->pendingMSDU = wns::ldk::CompoundPtr();
    MESSAGE_SINGLE(NORMAL, this->logger, "Send MSDU to "<< friends.manager->getReceiverAddress(it->getCommandPool()));
    return(it);
}

bool
RTSCTS::hasCapacity() const
{
    return(this->pendingMSDU == wns::ldk::CompoundPtr());
}

void
RTSCTS::onTxStart(const wns::ldk::CompoundPtr& /*compound*/)
{

}

void
RTSCTS::onTxEnd(const wns::ldk::CompoundPtr& compound)
{
    if(this->pendingMSDU and
       (getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this)) and
       (getCommand(compound->getCommandPool())->peer.isRTS) and
       (state == transmitRTS))
    {
        state = waitForCTS;
        setNewTimeout(sifsDuration + preambleProcessingDelay);
        MESSAGE_BEGIN(NORMAL, logger, m, "RTS to ");
        m << friends.manager->getReceiverAddress(compound->getCommandPool());
        m << " is sent, waiting for CTS for ";
        m << sifsDuration + preambleProcessingDelay;
        MESSAGE_END();
    }
}

void
RTSCTS::onRxStart(wns::simulator::Time /*expRxTime*/)
{
    if(state == waitForCTS)
    {
        assure(this->hasTimeoutSet(),
               "state if waitForCTS but no timeout set?");
        MESSAGE_SINGLE(NORMAL, logger,
                       "got rxStartIndication, set timeout to expected cts duration " << this->expectedCTSDuration);
        setNewTimeout(this->expectedCTSDuration + 10e-9);
        state = receiveCTS;
    }
}

void
RTSCTS::onRxEnd()
{
    if(state == receiveCTS)
    {
        MESSAGE_SINGLE(NORMAL, logger,
                       "onRxEnd and waiting for CTS -> set short timeout");
        assure(this->hasTimeoutSet(),
               "onRxEnd, but no timeout set");
        // wait some time for the delivery
        setNewTimeout(10e-9);
    }
}

void
RTSCTS::onRxError()
{
    // do nothing, there will be a timeout
}

void RTSCTS::onNAVBusy(const wns::service::dll::UnicastAddress setter)
{
    nav = true;
    navSetter = setter;
    MESSAGE_SINGLE(NORMAL, logger, "onNAVBusy from " << navSetter);
}

void RTSCTS::onNAVIdle()
{
    nav = false;
    MESSAGE_SINGLE(NORMAL, logger, "onNAVIdle");
}

void
RTSCTS::onTimeout()
{
    assure(state != idle, "onTimeout although state is idle");

    // reception of cts has failed --> frame has failed
    MESSAGE_SINGLE(NORMAL, this->logger, "No CTS received -> transmission has failed");

    // re-convert MSDU type from DATA_TXOP to DATA
    friends.manager->setFrameType(this->pendingMSDU->getCommandPool(), DATA);
    friends.arq->transmissionHasFailed(this->pendingMSDU);

    this->pendingMSDU = wns::ldk::CompoundPtr();
    state = idle;

    this->lastTimeout = wns::simulator::getEventScheduler()->getTime();

    this->tryToSend();
}

wns::ldk::CompoundPtr
RTSCTS::prepareRTS(const wns::ldk::CompoundPtr& msdu)
{
    // Calculate duration of the msdu for NAV setting

    wns::simulator::Time nav = sifsDuration
        + expectedCTSDuration
        + sifsDuration
        + friends.phyUser->getPSDUDuration(msdu)
        + sifsDuration
        + expectedACKDuration;

    wns::ldk::CompoundPtr rts =
        friends.manager->createCompound(friends.manager->getTransmitterAddress(msdu->getCommandPool()),   // tx address
                                        friends.manager->getReceiverAddress(msdu->getCommandPool()),      // rx address
                                        friends.manager->getFrameType(msdu->getCommandPool()),            // frame type
                                        nav,                                                              // NAV
                                        true);                                                            // requires direct reply

    wns::ldk::CommandPool* rtsCP = rts->getCommandPool();
    friends.manager->setPhyMode(rtsCP, friends.phyUser->getPhyModeProvider()->getPhyMode(phyModeId));
    RTSCTSCommand* rtsctsC = this->activateCommand(rtsCP);
    rtsctsC->peer.isRTS = true;

    /* set the transmission counter to the same value as the msdu */
    friends.arq->copyTransmissionCounter(msdu, rts);

    MESSAGE_BEGIN(NORMAL, this->logger, m, "Prepare RTS frame");
    m << " to " << friends.manager->getReceiverAddress(rtsCP);
    m << " with NAV " << nav;
    MESSAGE_END();

    return(rts);
}

wns::ldk::CompoundPtr
RTSCTS::prepareCTS(const wns::ldk::CompoundPtr& rts)
{
    wns::ldk::CommandPool* rtsCP = rts->getCommandPool();

    // calculate nav from rts
    wns::simulator::Time nav = friends.manager->getFrameExchangeDuration(rtsCP) - sifsDuration - expectedCTSDuration;
    wns::ldk::CompoundPtr cts = friends.manager->createCompound(friends.manager->getReceiverAddress(rtsCP),
                                                                friends.manager->getTransmitterAddress(rtsCP),
                                                                ACK,
                                                                nav,
                                                                true);
    friends.manager->setPhyMode(cts->getCommandPool(), friends.phyUser->getPhyModeProvider()->getPhyMode(phyModeId));
    RTSCTSCommand* rtsctsC = this->activateCommand(cts->getCommandPool());
    rtsctsC->peer.isRTS = false;

    MESSAGE_BEGIN(NORMAL, this->logger, m, "Prepare CTS frame");
    m << " to " << friends.manager->getTransmitterAddress(rtsCP);
    m << " with NAV " << nav;
    MESSAGE_END();

    this->ctsPrepared = wns::simulator::getEventScheduler()->getTime();

    return(cts);
}

void
RTSCTS::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
{
    if(getFUN()->getProxy()->commandIsActivated(commandPool, this))
    {
        if(getCommand(commandPool)->peer.isRTS)
        {
            commandPoolSize = this->rtsBits;
            dataSize = 0;
            MESSAGE_SINGLE(VERBOSE, this->logger, "Calculate size for RTS: " << rtsBits);
        }
        else
        {
            commandPoolSize = this->ctsBits;
            dataSize = 0;
            MESSAGE_SINGLE(VERBOSE, this->logger, "Calculate size for CTS: " << ctsBits);
        }
    }
    else
    {
        getFUN()->getProxy()->calculateSizes(commandPool, commandPoolSize, dataSize, this);
    }
}

