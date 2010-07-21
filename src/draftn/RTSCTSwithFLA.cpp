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

#include <WIFIMAC/draftn/RTSCTSwithFLA.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <DLL/Layer2.hpp>
#include <WNS/probe/bus/utils.hpp>

using namespace wifimac::draftn;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::RTSCTSwithFLA,
    wns::ldk::FunctionalUnit,
    "wifimac.draftn.RTSCTSwithFLA",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::RTSCTSwithFLA,
    wns::ldk::probe::Probe,
    "wifimac.draftn.RTSCTSwithFLA",
    wns::ldk::FUNConfigCreator);


RTSCTSwithFLA::RTSCTSwithFLA(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<RTSCTSwithFLA, RTSCTSwithFLACommand>(fun),

    phyUserName(config_.get<std::string>("phyUserName")),
    managerName(config_.get<std::string>("managerName")),
    protocolCalculatorName(config_.get<std::string>("protocolCalculatorName")),
    arqName(config_.get<std::string>("arqName")),
    navName(config_.get<std::string>("navName")),
    rxsName(config_.get<std::string>("rxStartName")),
    txStartEndName(config_.get<std::string>("txStartEndName")),
    sinrMIBServiceName(config_.get<std::string>("sinrMIBServiceName")),
    raName(config_.get<std::string>("raName")),

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

    estimatedValidity(100e-6),//config_.get<wns::simulator::Time>("myConfig.estimatedValidity")),
    nav(false),
    navSetter(),
    logger(config_.get("logger")),

    pendingRTS(),
    pendingCTS(),
    pendingMPDU(),

    state(idle)
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created, threshold: " << rtsctsThreshold << "b");
    sinrMIB = NULL;
    protocolCalculator = NULL;
    friends.phyUser = NULL;
    friends.manager = NULL;
    friends.arq = NULL;
    friends.ra = NULL;

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


RTSCTSwithFLA::~RTSCTSwithFLA()
{
}

void RTSCTSwithFLA::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.phyUser = getFUN()->findFriend<wifimac::convergence::PhyUser*>(phyUserName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
    friends.arq = getFUN()->findFriend<wifimac::lowerMAC::ITransmissionCounter*>(arqName);
    friends.ra = getFUN()->findFriend<wifimac::lowerMAC::RateAdaptation*>(raName);

    // Observe NAV
    this->wns::Observer<INetworkAllocationVector>::startObserving
        (getFUN()->findFriend<wifimac::convergence::NAVNotification*>(navName));

    // Observe rxStartEnd
    this->wns::Observer<wifimac::convergence::IRxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::RxStartEndNotification*>(rxsName));

    // Observe txStartEnd
    this->wns::Observer<wifimac::convergence::ITxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::TxStartEndNotification*>(txStartEndName));

    protocolCalculator = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
    sinrMIB = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::draftn::SINRwithMIMOInformationBase>(sinrMIBServiceName);
}

void
RTSCTSwithFLA::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    if(getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this))
    {
        if(getCommand(compound->getCommandPool())->peer.isRTS)
        {
            if(getCommand(compound->getCommandPool())->peer.isFLARequest)
            {
                wns::Ratio sinr = friends.phyUser->getCommand
                    (compound->getCommandPool())->getCIRwithoutMIMO();
                sinrMIB->putMeasurement
                    (friends.manager->getTransmitterAddress(compound->getCommandPool()), sinr, estimatedValidity);
                MESSAGE_SINGLE(NORMAL, this->logger, "Request from " <<  friends.manager->getTransmitterAddress(compound->getCommandPool()) << ", measured SINR " << sinr);
            }

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
            if(getCommand(compound->getCommandPool())->peer.isFLAResponse)
            {
                MESSAGE_BEGIN(NORMAL, this->logger, m, "Reply from ");
                m << friends.manager->getTransmitterAddress(compound->getCommandPool());
                m << ", with SINR ";
                m << getCommand(compound->getCommandPool())->peer.cqi;
                MESSAGE_END();

                // put peer SINR measurement
                sinrMIB->putPeerSINR(friends.manager->getTransmitterAddress(compound->getCommandPool()),
                                     getCommand(compound->getCommandPool())->peer.cqi,
                                     estimatedValidity);

                // put peer mimo factors measurement
                for(std::vector< std::vector<wns::Ratio> >::iterator it = getCommand(compound->getCommandPool())->peer.mimoFactors.begin();
                    it != getCommand(compound->getCommandPool())->peer.mimoFactors.end();
                    ++it)
                {
                    sinrMIB->putPeerFactor(friends.manager->getTransmitterAddress(compound->getCommandPool()),
                                           *it);
#ifndef WNS_NO_LOGGING
                    MESSAGE_BEGIN(NORMAL, logger, m, "Contains peer factors");
                    for(std::vector<wns::Ratio>::iterator itFactors = it->begin();
                        itFactors != it->end();
                        ++itFactors)
                    {
                        m << " " << *itFactors;
                    }
                    MESSAGE_END();
#endif
                }

                wifimac::convergence::PhyMode pm = friends.ra->getPhyMode(friends.manager->getTransmitterAddress(compound->getCommandPool()),1);
                friends.manager->setPhyMode(this->pendingMPDU->getCommandPool(), pm);
            }

            // received CTS on transmitted RTS --> successfully reserved the channel
            // for data
            assure(this->pendingMPDU, "Received CTS, but no pending MPDU");
            if(state == idle)
            {
                MESSAGE_BEGIN(NORMAL, this->logger, m, "received CTS although state is idle, now: ");
                m << wns::simulator::getEventScheduler()->getTime();
                m << ", last timeout: " << this->lastTimeout << "\n";
                if(state == transmitRTS)
                    m << "state is transmitRTS\n";
                if(state == waitForCTS)
                    m << "state is waitForCTS\n";
                if(state == receiveCTS)
                    m << "state is receiveCTS\n";
                MESSAGE_END();
            }
            assure(state == receptionFinished,
                   "received CTS although state is idle, now: " << wns::simulator::getEventScheduler()->getTime() << ", last timeout: " << this->lastTimeout);

            if(friends.manager->getTransmitterAddress(compound->getCommandPool())
               != friends.manager->getReceiverAddress(this->pendingMPDU->getCommandPool()))
            {
                MESSAGE_SINGLE(NORMAL, this->logger,
                               "Incoming CTS does not match the receiver's address on the pending MPDU -> do nothing");
            }
            else
            {
                assure(this->pendingMPDU, "Received CTS, but no pending MPDU");
                MESSAGE_SINGLE(NORMAL, this->logger,
                               "Incoming awaited CTS -> send data");
                rtsSuccessProbe->put(this->pendingMPDU, 1);
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
RTSCTSwithFLA::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(this->pendingMPDU == wns::ldk::CompoundPtr(),
           "Cannot have two MPDUs");
    assure(this->pendingRTS == wns::ldk::CompoundPtr(),
           "Cannot have two RTSs");

    this->pendingMPDU = compound;

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
            this->pendingRTS = this->prepareRTS(this->pendingMPDU);

            // RTS/CTS initializes mini-TXOP for compound, it can be send
            // directly after SIFS
            friends.manager->setFrameType(this->pendingMPDU->getCommandPool(), DATA_TXOP);
        }
        break;
    default:
        throw wns::Exception("Unknown frame type");
        break;
    }
}

const wns::ldk::CompoundPtr
RTSCTSwithFLA::hasSomethingToSend() const
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
        return(this->pendingMPDU);
    }
    return wns::ldk::CompoundPtr();

}

wns::ldk::CompoundPtr
RTSCTSwithFLA::getSomethingToSend()
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

    it = this->pendingMPDU;
    this->pendingMPDU = wns::ldk::CompoundPtr();
    MESSAGE_SINGLE(NORMAL, this->logger, "Send MPDU to "<< friends.manager->getReceiverAddress(it->getCommandPool()));
    return(it);
}

bool
RTSCTSwithFLA::hasCapacity() const
{
    return(this->pendingMPDU == wns::ldk::CompoundPtr());
}

void
RTSCTSwithFLA::onTxStart(const wns::ldk::CompoundPtr& /*compound*/)
{

}

void
RTSCTSwithFLA::onTxEnd(const wns::ldk::CompoundPtr& compound)
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
RTSCTSwithFLA::onRxStart(wns::simulator::Time /*expRxTime*/)
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
RTSCTSwithFLA::onRxEnd()
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
RTSCTSwithFLA::onRxError()
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

void RTSCTSwithFLA::onNAVBusy(const wns::service::dll::UnicastAddress setter)
{
    nav = true;
    navSetter = setter;
    MESSAGE_SINGLE(NORMAL, logger, "onNAVBusy from " << navSetter);
}

void RTSCTSwithFLA::onNAVIdle()
{
    nav = false;
    MESSAGE_SINGLE(NORMAL, logger, "onNAVIdle");
}

void
RTSCTSwithFLA::onTimeout()
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

    this->tryToSend();
}

wns::ldk::CompoundPtr
RTSCTSwithFLA::prepareRTS(const wns::ldk::CompoundPtr& mpdu)
{
    // Calculate duration of the mpdu for NAV setting

    wns::simulator::Time duration =
        protocolCalculator->getDuration()->MPDU_PPDU(mpdu->getLengthInBits(),
                                                     friends.manager->getPhyMode(mpdu->getCommandPool()));
    wns::service::dll::UnicastAddress receiver = friends.manager->getReceiverAddress(mpdu->getCommandPool());

    wns::simulator::Time nav = sifsDuration
        + maximumCTSDuration
        + sifsDuration;

    wns::ldk::CompoundPtr rts =
        friends.manager->createCompound(friends.manager->getTransmitterAddress(mpdu->getCommandPool()),   // tx address
                                        receiver,                                                         // rx address
                                        friends.manager->getFrameType(mpdu->getCommandPool()),            // frame type
                                        nav,               // NAV
                                        ctsTimeout);  // requires direct reply after timeout

    wns::ldk::CommandPool* rtsCP = rts->getCommandPool();
    friends.manager->setPhyMode(rtsCP, rtsctsPhyMode);
    RTSCTSwithFLACommand* rtsctsC = this->activateCommand(rtsCP);
    rtsctsC->peer.isRTS = true;

    /* set the transmission counter to the same value as the mpdu */
    friends.arq->copyTransmissionCounter(mpdu, rts);

    MESSAGE_BEGIN(NORMAL, this->logger, m, "Prepare RTS frame + FLA request");
    m << " to " << friends.manager->getReceiverAddress(rtsCP);
    m << " with NAV " << nav;
    MESSAGE_END();

    rtsctsC->peer.isFLARequest = true;
    rtsctsC->peer.isFLAResponse = false;
    rtsctsC->magic.frameSize = mpdu->getLengthInBits();

    return(rts);
}

wns::ldk::CompoundPtr
RTSCTSwithFLA::prepareCTS(const wns::ldk::CompoundPtr& rts)
{
    wns::ldk::CommandPool* rtsCP = rts->getCommandPool();
    wns::service::dll::UnicastAddress peer = friends.manager->getTransmitterAddress(rtsCP);

    // Simulate the RA process at the transmitter: First, put the measured SINR
    // as the (faked) peer SINR
    sinrMIB->putFakePeerSINR(peer, sinrMIB->getMeasuredSINR(peer));
    sinrMIB->putFakePeerMIMOFactors(peer, sinrMIB->getAllMeasuredFactors(peer));

    // then, the RA uses the (faked) peer SINR to compute the phyMode
    wifimac::convergence::PhyMode pm = friends.ra->getPhyMode(peer, 1);

    // finaly, this allows for computing the correct NAV
    wns::simulator::Time nav = sifsDuration
        + protocolCalculator->getDuration()->MPDU_PPDU(
            getCommand(rts->getCommandPool())->magic.frameSize, pm)
        + sifsDuration
        + maximumACKDuration;

    wns::ldk::CompoundPtr cts = friends.manager->createCompound(friends.manager->getReceiverAddress(rtsCP),
                                                                peer,
                                                                ACK,
                                                                nav,
                                                                sifsDuration + preambleProcessingDelay);
    friends.manager->setPhyMode(cts->getCommandPool(), rtsctsPhyMode);
    RTSCTSwithFLACommand* rtsctsC = this->activateCommand(cts->getCommandPool());
    rtsctsC->peer.isRTS = false;

    MESSAGE_BEGIN(NORMAL, this->logger, m, "Prepare CTS frame");
    m << " to " << friends.manager->getTransmitterAddress(rtsCP);
    m << " with NAV " << nav;
    MESSAGE_END();

    this->ctsPrepared = wns::simulator::getEventScheduler()->getTime();

    // Add measurements as reply
    if(sinrMIB->knowsMeasuredSINR(peer))
    {
        rtsctsC->peer.isFLARequest = false;
        rtsctsC->peer.isFLAResponse = true;
        rtsctsC->peer.cqi = sinrMIB->getMeasuredSINR(peer);

        for(unsigned int numSS = 1; numSS <= friends.manager->getNumAntennas(); ++numSS)
        {
            if(sinrMIB->knowsMeasuredFactor(peer, numSS))
            {
                rtsctsC->peer.mimoFactors.push_back(sinrMIB->getMeasuredFactor(peer, numSS));
            }
        }

#ifndef WNS_NO_LOGGING
        MESSAGE_BEGIN(NORMAL, this->logger, m, "Outgoing CTS/FLA response to " << peer);
        m << ", piggyback measured SINR " << rtsctsC->peer.cqi;
        m << ", pF:";
        for(unsigned int numSS = 1; numSS <= friends.manager->getNumAntennas(); ++numSS)
        {
            m << "(";
            if(sinrMIB->knowsMeasuredFactor(peer, numSS))
            {
                std::vector<wns::Ratio> pF = sinrMIB->getMeasuredFactor(peer, numSS);
                for(std::vector<wns::Ratio>::iterator it = pF.begin();
                    it != pF.end();
                    ++it)
                {
                    m << " " << *it;
                }
            }
            m << ")";
        }
        MESSAGE_END();
#endif
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "FLA response to: " << peer << ", but SINR is not known!");
    }
    return(cts);
}

void
RTSCTSwithFLA::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
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

