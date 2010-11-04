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
#include <DLL/Layer2.hpp>
#include <WNS/probe/bus/utils.hpp>
#include <WNS/ldk/Port.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::RTSCTS,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.RTSCTS",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::RTSCTS,
    wns::ldk::probe::Probe,
    "wifimac.lowerMAC.RTSCTS",
    wns::ldk::FUNConfigCreator);

RTSCTS::RTSCTS(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<RTSCTS, RTSCTSCommand>(fun),
    phyUserName(config_.get<std::string>("phyUserName")),
    managerName(config_.get<std::string>("managerName")),
    protocolCalculatorName(config_.get<std::string>("protocolCalculatorName")),
    arqName(config_.get<std::string>("arqName")),
    navName(config_.get<std::string>("navName")),
    rxsName(config_.get<std::string>("rxStartName")),
    txStartEndName(config_.get<std::string>("txStartEndName")),

    sifsDuration(config_.get<wns::simulator::Time>("myConfig.sifsDuration")),
    maximumACKDuration(config_.get<wns::simulator::Time>("myConfig.maximumACKDuration")),
    maximumCTSDuration(config_.get<wns::simulator::Time>("myConfig.maximumCTSDuration")),
    preambleProcessingDelay(config_.get<wns::simulator::Time>("myConfig.preambleProcessingDelay")),
    ctsTimeout(config_.get<wns::simulator::Time>("myConfig.ctsTimeout")),
    rtsctsPhyMode(config_.getView("myConfig.rtsctsPhyMode")),
    rtsBits(config_.get<Bit>("myConfig.rtsBits")),
    ctsBits(config_.get<Bit>("myConfig.ctsBits")),
    rtsctsThreshold(config_.get<Bit>("myConfig.rtsctsThreshold")),
    rtsctsOnTxopData(config_.get<bool>("myConfig.rtsctsOnTxopData")),

    nav(false),
    navSetter(),
    logger(config_.get("logger")),

    pendingRTS(),
    pendingCTS(),
    pendingMPDU(),

    state(idle)
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created, threshold: " << rtsctsThreshold << "b");

    protocolCalculator = NULL;
    friends.phyUser = NULL;
    friends.manager = NULL;
    friends.arq = NULL;

    this->ctsPrepared = 0;
    this->lastTimeout = 0;

    // read the local IDs from the config
    wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config_.len("localIDs.keys()"); ++ii)
    {
        std::string key = config_.get<std::string>("localIDs.keys()",ii);
        unsigned long int value  = config_.get<unsigned long int>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }
    rtsSuccessProbe = wns::probe::bus::collector(localContext, config_, "rtsSuccessProbeName");
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

    protocolCalculator = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
}

bool
RTSCTS::doIsAccepting(const wns::ldk::CompoundPtr& compound) const
{
    switch(friends.manager->getFrameType(compound->getCommandPool()))
    {
    case DATA_TXOP:
        if(not this->rtsctsOnTxopData)
        {
            return(getConnector()->hasAcceptor(compound));
        }
    case DATA:
        if(compound->getLengthInBits() < this->rtsctsThreshold)
        {
            return(getConnector()->hasAcceptor(compound));
        }
        else
        {
            return (this->pendingMPDU == wns::ldk::CompoundPtr());
        }
    default:
        throw wns::Exception("Unknown frame type");
        break;
    }
}

void
RTSCTS::doSendData(const wns::ldk::CompoundPtr& compound)
{
    assure(this->pendingMPDU == wns::ldk::CompoundPtr(),
           "Cannot have two MPDUs");
    assure(this->pendingRTS == wns::ldk::CompoundPtr(),
           "Cannot have two RTSs");

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
            this->pendingMPDU = compound;
            this->pendingRTS = this->prepareRTS(this->pendingMPDU);

            // RTS/CTS initializes mini-TXOP for compound, it can be send
            // directly after SIFS
            friends.manager->setFrameType(this->pendingMPDU->getCommandPool(), DATA_TXOP);

            // try to send RTS
            if(getConnector()->hasAcceptor(this->pendingRTS))
            {
                state = transmitRTS;
                getConnector()->getAcceptor(compound)->sendData(this->pendingRTS);
                this->pendingRTS = wns::ldk::CompoundPtr();
            }
            return;
        }
        break;
    default:
        throw wns::Exception("Unknown frame type");
        break;
    }

    // try to send data
    if(getConnector()->hasAcceptor(compound))
    {
        getConnector()->getAcceptor(compound)->sendData(compound);
    }
    else
    {
        this->pendingMPDU = compound;
    }
}

void
RTSCTS::doWakeup()
{
    if(this->pendingCTS)
    {
        throw wns::Exception("Received wakeup, but CTS is pending");

        /*
        MESSAGE_SINGLE(NORMAL, this->logger, "CTS to "<< friends.manager->getReceiverAddress(this->pendingCTS->getCommandPool()) << " pending");
        if(getConnector()->hasAcceptor(this->pendingCTS))
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "Sending CTS frame to "<< friends.manager->getReceiverAddress(this->pendingCTS->getCommandPool()));
            assure(this->ctsPrepared == wns::simulator::getEventScheduler()->getTime(),
                   "ctsPrepared is " << this->ctsPrepared << ", but time is " <<  wns::simulator::getEventScheduler()->getTime());
            getConnector()->getAcceptor(this->pendingCTS)->sendData(this->pendingCTS->copy());
            this->pendingCTS =  wns::ldk::CompoundPtr();
        }
        else
        {
            return;
        }
        */
    }

    if(this->pendingRTS)
    {
        assure(this->pendingMPDU, "pendingRTS but no pendingMPDU");
        MESSAGE_SINGLE(NORMAL, this->logger, "RTS to "<< friends.manager->getReceiverAddress(this->pendingRTS->getCommandPool()) << " pending");
        if(getConnector()->hasAcceptor(this->pendingRTS))
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "Sending RTS frame to "<< friends.manager->getReceiverAddress(this->pendingRTS->getCommandPool()));
            state = transmitRTS;
            getConnector()->getAcceptor(this->pendingRTS)->sendData(this->pendingRTS->copy());
            this->pendingRTS =  wns::ldk::CompoundPtr();
        }
        else
        {
            return;
        }
    }

    if(this->pendingMPDU)
    {
        if(state == idle)
        {
            throw wns::Exception("Received wakeup, but MPDU is pending");
        }
        else
        {
            // waiting for CTS...
            return;
        }
        /*
        MESSAGE_SINGLE(NORMAL, this->logger, "MPDU to "<< friends.manager->getReceiverAddress(this->pendingMPDU->getCommandPool()) << " pending");
        if(getConnector()->hasAcceptor(this->pendingMPDU))
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "Sending MPDU to "<< friends.manager->getReceiverAddress(this->pendingMPDU->getCommandPool()));
            getConnector()->getAcceptor(this->pendingMPDU)->sendData(this->pendingMPDU->copy());
            this->pendingMPDU =  wns::ldk::CompoundPtr();
            getReceptor()->wakeup();
        }
        else
        {
            return;
        }
        */
    }
    getReceptor()->wakeup();
}

void
RTSCTS::doOnData(const wns::ldk::CompoundPtr& compound)
{
    if(not getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this))
    {
        // deliver frame
        MESSAGE_SINGLE(NORMAL, this->logger, "Received frame -> deliver");
        getDeliverer()->getAcceptor(compound)->onData(compound);
        return;
    } // no RTSCTS command

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
                return;
            }
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "Incoming RTS, nav idle -> reply with CTS");
            assure(this->pendingCTS == wns::ldk::CompoundPtr(),
                   "Old pending CTS not transmitted");
            this->pendingCTS = this->prepareCTS(compound); 
        }

        // try to send CTS
        assure(this->pendingCTS, "CTS should be pending now");
        if(getConnector()->hasAcceptor(this->pendingCTS))
        {
            getConnector()->getAcceptor(this->pendingCTS)->sendData(this->pendingCTS);
            this->pendingCTS = wns::ldk::CompoundPtr();
        }
        else
        {
            throw wns::Exception("pending CTS is not accepted");
        }
    } // is RTS
    else
    {
        // received CTS on transmitted RTS --> successfully reserved the channel
        // for data
        assure(this->pendingMPDU, "Received CTS, but no pending MPDU");
        if(state != receptionFinished)
        {
            MESSAGE_BEGIN(NORMAL, this->logger, m, "received CTS although state is not receptionFinished, now: ");
            m << wns::simulator::getEventScheduler()->getTime();
            m << ", last timeout: " << this->lastTimeout << "\n";
            if(state == transmitRTS)
                m << "state is transmitRTS\n";
            if(state == waitForCTS)
                m << "state is waitForCTS\n";
            if(state == receiveCTS)
                m << "state is receiveCTS\n";
            if(state == idle)
                m << "state is idle\n";
            MESSAGE_END();
        }
        assure(state == receptionFinished,
               "received CTS although state is idle, now: " << wns::simulator::getEventScheduler()->getTime() << ", last timeout: " << this->lastTimeout);
        if(friends.manager->getTransmitterAddress(compound->getCommandPool())
           != friends.manager->getReceiverAddress(this->pendingMPDU->getCommandPool()))
        {
                MESSAGE_SINGLE(NORMAL, this->logger,
                               "Incoming CTS does not match the receiver's address on the pending MPDU -> do nothing");
                return;
        }
        else
        {
            assure(this->pendingMPDU, "Received CTS, but no pendingMPDU");
            MESSAGE_SINGLE(NORMAL, this->logger, "Received CTS, sending MPDU to "<< friends.manager->getReceiverAddress(this->pendingMPDU->getCommandPool()));
            rtsSuccessProbe->put(this->pendingMPDU, 1);
            state = idle;
            if(this->hasTimeoutSet())
            {
                this->cancelTimeout();
            }
            if(getConnector()->hasAcceptor(this->pendingMPDU))
            {
                getConnector()->getAcceptor(this->pendingMPDU)->sendData(this->pendingMPDU->copy());
                this->pendingMPDU =  wns::ldk::CompoundPtr();
                // ready for new MPDU
                getReceptor()->wakeup();
            }
            else
            {
                throw wns::Exception("pending MPDU is not accepted");
            }
        }
    }
}

void
RTSCTS::onTxStart(const wns::ldk::CompoundPtr& /*compound*/)
{

}

void
RTSCTS::onTxEnd(const wns::ldk::CompoundPtr& compound)
{
    if(this->pendingMPDU and
       (getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this)) and
       (getCommand(compound->getCommandPool())->peer.isRTS) and
       (state == transmitRTS))
    {
        state = waitForCTS;
        setNewTimeout(ctsTimeout);
        MESSAGE_BEGIN(NORMAL, logger, m, "RTS to ");
        m << friends.manager->getReceiverAddress(compound->getCommandPool());
        m << " is sent, waiting for CTS for ";
        m << ctsTimeout;
        MESSAGE_END();
    }
}

void
RTSCTS::onRxStart(wns::simulator::Time /*expRxTime*/)
{
    if(state == waitForCTS)
    {
        assure(this->hasTimeoutSet(), "state waitForCTS but no timeout set?");
        assure(this->pendingMPDU, "state waitForCTS but no pendingMPDU?");
        //cancelTimeout();
        MESSAGE_SINGLE(NORMAL, logger,
                       "got rxStartIndication, cancel timeout");
        state = receiveCTS;
    }
}

void
RTSCTS::onRxEnd()
{
    if(state == receiveCTS)
    {
        assure(this->pendingMPDU, "state receiveCTS but no pendingMPDU?");
        MESSAGE_SINGLE(NORMAL, logger,
                       "onRxEnd and waiting for CTS -> set short timeout");
        state = receptionFinished;

        // wait some time for the delivery
        setNewTimeout(10e-9);
    }
}

void
RTSCTS::onRxError()
{
    if(state == receiveCTS)
    {
        assure(this->pendingMPDU, "state waitForCTS/receiveCTS but no pendingMPDU?");
        MESSAGE_SINGLE(NORMAL, logger,
                       "onRxError and waiting for CTS -> failure!");
        state = waitForCTS;

        if(not hasTimeoutSet())
        {
            // waiting period is over
            this->onTimeout();
        }
    }
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
    if(state == receiveCTS)
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Started reception during wait for CTS -> wait for delivery");
        return;
    }
    assure(state != idle, "onTimeout although state is idle");

    // reception of cts has failed --> frame has failed
    MESSAGE_SINGLE(NORMAL, this->logger, "No CTS received -> transmission has failed");

    rtsSuccessProbe->put(this->pendingMPDU, 0);

    // re-convert MPDU type from DATA_TXOP to DATA
    friends.manager->setFrameType(this->pendingMPDU->getCommandPool(), DATA);
    friends.arq->onTransmissionHasFailed(this->pendingMPDU);

    this->pendingMPDU = wns::ldk::CompoundPtr();
    state = idle;

    this->lastTimeout = wns::simulator::getEventScheduler()->getTime();

    getReceptor()->wakeup();
}

wns::ldk::CompoundPtr
RTSCTS::prepareRTS(const wns::ldk::CompoundPtr& mpdu)
{
    // Calculate duration of the mpdu for NAV setting

    wns::simulator::Time duration =
        protocolCalculator->getDuration()->MPDU_PPDU(mpdu->getLengthInBits(),
                                                     friends.manager->getPhyMode(mpdu->getCommandPool()));

    wns::simulator::Time nav =
        sifsDuration
        + maximumCTSDuration
        + sifsDuration
        + duration
        + sifsDuration
        + maximumACKDuration;

    wns::ldk::CompoundPtr rts =
        friends.manager->createCompound(friends.manager->getTransmitterAddress(mpdu->getCommandPool()),   // tx address
                                        friends.manager->getReceiverAddress(mpdu->getCommandPool()),      // rx address
                                        friends.manager->getFrameType(mpdu->getCommandPool()),            // frame type
                                        nav,               // NAV
                                        ctsTimeout);  // requires direct reply after timeout

    wns::ldk::CommandPool* rtsCP = rts->getCommandPool();
    friends.manager->setPhyMode(rtsCP, rtsctsPhyMode);
    RTSCTSCommand* rtsctsC = this->activateCommand(rtsCP);
    rtsctsC->peer.isRTS = true;

    /* set the transmission counter to the same value as the mpdu */
    friends.arq->copyTransmissionCounter(mpdu, rts);

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
    wns::simulator::Time nav = friends.manager->getFrameExchangeDuration(rtsCP) - sifsDuration - maximumCTSDuration;
    wns::ldk::CompoundPtr cts = friends.manager->createCompound(friends.manager->getReceiverAddress(rtsCP),
                                                                friends.manager->getTransmitterAddress(rtsCP),
                                                                ACK,
                                                                nav,
                                                                sifsDuration + preambleProcessingDelay);
    friends.manager->setPhyMode(cts->getCommandPool(), rtsctsPhyMode);
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

