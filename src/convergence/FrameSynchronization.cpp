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

#include <WIFIMAC/convergence/FrameSynchronization.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/convergence/PreambleGenerator.hpp>
#include <WIFIMAC/convergence/ErrorModelling.hpp>
#include <WIFIMAC/convergence/TxDurationSetter.hpp>

#include <WNS/ldk/Layer.hpp>
#include <WNS/ldk/crc/CRC.hpp>
#include <WNS/probe/bus/utils.hpp>
#include <WNS/probe/bus/json/probebus.hpp>

#include <WNS/service/dll/StationTypes.hpp>

#include <DLL/Layer2.hpp>
#include <DLL/StationManager.hpp>

#include <sstream>

using namespace wifimac::convergence;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    FrameSynchronization,
    wns::ldk::probe::Probe,
    "wifimac.convergence.FrameSynchronization",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    FrameSynchronization,
    wns::ldk::FunctionalUnit,
    "wifimac.convergence.FrameSynchronization",
    wns::ldk::FUNConfigCreator);

FrameSynchronization::FrameSynchronization(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config):
    wns::ldk::fu::Plain<FrameSynchronization, FrameSynchronizationCommand>(fun),
    wns::ldk::probe::Probe(),

    logger(config.get("logger")),
    curState(Idle),
    synchronizedToAddress(),
    slcCapture(config.get<wns::Ratio>("myConfig.slcCapture")),
    slgCapture(config.get<wns::Ratio>("myConfig.slgCapture")),
    idleCapture(config.get<wns::Ratio>("myConfig.idleCapture")),
    detectionThreshold(config.get<wns::Ratio>("myConfig.detectionThreshold")),
    signalRxErrorAlthoughNotSynchronized(config.get<bool>("myConfig.signalRxErrorAlthoughNotSynchronized")),
    lastFrameEnd(0),
    managerName(config.get<std::string>("managerName")),
    crcCommandName(config.get<std::string>("crcCommandName")),
    phyUserCommandName(config.get<std::string>("phyUserCommandName")),
    errorModellingCommandName(config.get<std::string>("errorModellingCommandName")),
    txDurationProviderCommandName(config.get<std::string>("txDurationProviderCommandName")),
    sinrMIBServiceName(config.get<std::string>("sinrMIBServiceName")),
    numSpatialStreamsLastPreambleFragment(0)
{
    // read the localIDs from the config
    wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
    {
        std::string key = config.get<std::string>("localIDs.keys()",ii);
        unsigned long int value  = config.get<unsigned long int>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }
    successRateProbe = wns::probe::bus::collector(localContext, config, "successRateProbeName");
    sinrProbe = wns::probe::bus::collector(localContext, config, "sinrProbeName");
    perProbe = wns::probe::bus::collector(localContext, config, "perProbeName");
    jsonTracing = wns::probe::bus::collector(localContext, config, "phyTraceProbeName");
}

FrameSynchronization::~FrameSynchronization()
{

}

void FrameSynchronization::doSendData(const wns::ldk::CompoundPtr& compound)
{
    // Stop any synchronization
    switch(curState)
    {
    case Idle:
        break;
    case Synchronized:
        // signal end of frame to each observer
        MESSAGE_SINGLE(NORMAL, logger, "Send data during decoding -> signal rx error");
        this->wns::Subject<IRxStartEnd>::forEachObserver(OnRxStartEnd(0, false, true));
        // Fall through to set state to Idle
    case waitForFinalDelivery:
    case Garbled:
        MESSAGE_SINGLE(NORMAL, logger, "Send data during reception -> reset curState to idle");
        lastFrameEnd = wns::simulator::getEventScheduler()->getTime();
        cancelTimeout();
        curState = Idle;
        synchronizedToAddress = wns::service::dll::UnicastAddress();
        numSpatialStreamsLastPreambleFragment = 0;
        break;
    default:
        assure(false, "Unknown state");
    }

    // pass through
    getConnector()->getAcceptor(compound)->sendData(compound);
}

void FrameSynchronization::doOnData(const wns::ldk::CompoundPtr& compound)
{
#ifndef NDEBUG
    if(jsonTracing->hasObservers())
        traceIncoming(compound);
#endif

    if(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE)
    {
        this->processPreamble(compound);
    }
    else
    {
        this->processPSDU(compound);
    }
}

void FrameSynchronization::processPreamble(const wns::ldk::CompoundPtr& compound)
{
    assure(friends.manager->getFrameType(compound->getCommandPool()) == PREAMBLE,
           "called processPreamble for non-preamble");

    bool crcOk = getFUN()->getCommandReader(crcCommandName)->readCommand<wns::ldk::crc::CRCCommand>(compound->getCommandPool())->local.checkOK;

    wns::simulator::Time fDur = friends.manager->getFrameExchangeDuration(compound->getCommandPool());

    wns::Ratio sinr = getFUN()->getCommandReader(phyUserCommandName)->
        readCommand<wifimac::convergence::PhyUserCommand>(compound->getCommandPool())->getCIR();

    if(sinr < detectionThreshold)
    {
        MESSAGE_SINGLE(NORMAL, logger, "ProcessPreamble(idle), SINR= " << sinr << " below detection threshold -> DROP");
        return;
    }

    if(curState == Synchronized and
       friends.manager->getTransmitterAddress(compound->getCommandPool()) == this->synchronizedToAddress)
    {
         MESSAGE_BEGIN(NORMAL, logger, m, "Received LTF from ");
         m << friends.manager->getTransmitterAddress(compound->getCommandPool());
         m << " with ";
         m << friends.manager->getPhyMode(compound->getCommandPool()).getNumberOfSpatialStreams();
         m << " spatial streams";
         MESSAGE_END();

        if(friends.manager->getPhyMode(compound->getCommandPool()).getNumberOfSpatialStreams() != (numSpatialStreamsLastPreambleFragment+1))
        {
            MESSAGE_BEGIN(NORMAL, logger, m, "Received LTF with ");
            m << friends.manager->getPhyMode(compound->getCommandPool()).getNumberOfSpatialStreams();
            m << " spatial streams, but waiting for ";
            m << numSpatialStreamsLastPreambleFragment+1;
            m << " -> DROP";
            MESSAGE_END();
            return;
        }
        ++numSpatialStreamsLastPreambleFragment;

        // the preamble comes from the same transmitter as the current
        // synchronization
        assure(wns::simulator::getEventScheduler()->getTime() + fDur <= lastFrameEnd + 1e-9,
               "Received LTF tries to extend the synchronization time: Now" << wns::simulator::getEventScheduler()->getTime() << ", fDur " << fDur << ", lfe " << lastFrameEnd);
        //this->syncToNewPreamble(fDur, synchronizedToAddress);
        // deliver LTF
        getDeliverer()->getAcceptor(compound)->onData(compound);
        return;
    }

    // preamble from difference receiver
    if(friends.manager->getPhyMode(compound->getCommandPool()).getNumberOfSpatialStreams() > 1)
    {
        MESSAGE_SINGLE(NORMAL, logger, "Received LTF, but no preamble with one stream -> DROP");
        return;
    }


    wns::Ratio captureThreshold;
    switch(curState)
    {
    case Idle:
        MESSAGE_SINGLE(NORMAL, logger, "ProcessPreamble(idle), SINR= " << sinr << " CRC " << crcOk);
        assure(lastFrameEnd <= wns::simulator::getEventScheduler()->getTime(), "State is idle, but last frame has not finished");
        assure(!this->hasTimeoutSet(), "State is idle, but timeout is set");
        captureThreshold = idleCapture;
        break;

    case Synchronized:
        MESSAGE_SINGLE(NORMAL, logger, "processPreamble(synchronized), SINR= " << sinr << " CRC " << crcOk);
        assure(lastFrameEnd >= wns::simulator::getEventScheduler()->getTime(), "State is synchronized, but lastFrameEnd is over");
        assure(this->hasTimeoutSet(), "State is decoding, but no timeout set");
        captureThreshold = slcCapture;
        break;

    case waitForFinalDelivery:
        MESSAGE_SINGLE(NORMAL, logger, "processPreamble(waitForFinalDelivery), SINR= " << sinr << " CRC " << crcOk);
        assure(this->hasTimeoutSet(), "State is waitForFinalDelivery, but no timeout set");
        assure(lastFrameEnd+10e-9 >= wns::simulator::getEventScheduler()->getTime(), "State is waitForFinalDelivery, but lastFrameEnd is over");
        captureThreshold = slcCapture;
        break;

    case Garbled:
        MESSAGE_SINGLE(NORMAL, logger, "processPreamble(garbled), SINR= " << sinr << " CRC " << crcOk);
        assure(this->hasTimeoutSet(), "State is garbled, but no timeout set");
        assure(lastFrameEnd >= wns::simulator::getEventScheduler()->getTime(), "State is garbled, but lastFrameEnd is over");
        captureThreshold = slgCapture;
        break;

    default:
        assure(false, "Unknown state");
    }

    if (sinr > captureThreshold)
    {
        // capture has taken place
        if(curState == Synchronized)
        {
            // signal end of frame to each observer
            MESSAGE_SINGLE(NORMAL, logger, "ProcessPreamble -> Capture current decoding, signal rx end");
            this->wns::Subject<IRxStartEnd>::forEachObserver(OnRxStartEnd(0, false, true));
        }

        if(crcOk)
        {
            this->syncToNewPreamble(fDur, friends.manager->getTransmitterAddress(compound->getCommandPool()));
            // deliver preamble
            getDeliverer()->getAcceptor(compound)->onData(compound);
        }
        else
        {
            this->failedSyncToNewPreamble(fDur);
        }
    }
    else
    {
        if ((curState == Synchronized) or (curState == waitForFinalDelivery))
        {
            // special handling if the state is synchronized:
            // Wait for the end of the current frame, and then change into the garbled state if neccessary
            if(wns::simulator::getEventScheduler()->getTime() + fDur > lastFrameEnd)
            {
                lastFrameEnd = wns::simulator::getEventScheduler()->getTime() + fDur;
                MESSAGE_SINGLE(NORMAL, logger, "Rx preamble while synchronized, but no capture, decode frame and then garbled until " << lastFrameEnd);
            }
            else
            {
                MESSAGE_SINGLE(NORMAL, logger, "Rx preamble while synchronized, no capture, frame is shorter than current one");
            }
        }
        else
        {
            this->failedSyncToNewPreamble(fDur);
        }
    }
}

void FrameSynchronization::failedSyncToNewPreamble(wns::simulator::Time fDur)
{
    if((wns::simulator::getEventScheduler()->getTime() + fDur) > lastFrameEnd)
    {
        lastFrameEnd = wns::simulator::getEventScheduler()->getTime() + fDur;
        this->setNewTimeout(fDur);
        MESSAGE_SINGLE(NORMAL, logger, "Rx preamble, but no sync possible -> garbled until " << lastFrameEnd);
    }
    else
    {
        assure(this->hasTimeoutSet(), "lastFrameEnd is not over, but no timeout set");
        MESSAGE_SINGLE(NORMAL, logger, "Rx another preamble, but no sync possible -> garbled for " << fDur << " and afterwards until " << lastFrameEnd);
    }
    this->synchronizedToAddress = wns::service::dll::UnicastAddress();
    numSpatialStreamsLastPreambleFragment = 0;
    curState = Garbled;
}

void FrameSynchronization::syncToNewPreamble(const wns::simulator::Time fDur, const wns::service::dll::UnicastAddress transmitter)
{
    MESSAGE_SINGLE(NORMAL, logger, "Rx preamble from " << transmitter << ", signal rxStart");
    assure(fDur > 0, "Preamble must have duration larger than zero");
    this->wns::Subject<IRxStartEnd>::forEachObserver(OnRxStartEnd(fDur, true, false));

    this->setNewTimeout(fDur);

    if(((wns::simulator::getEventScheduler()->getTime() + fDur) >= lastFrameEnd)
       or
       (lastFrameEnd - (wns::simulator::getEventScheduler()->getTime() + fDur) < 0.000001))
    {
        // only change lastFrameEnd if the new frame is longer than the current one
        lastFrameEnd = wns::simulator::getEventScheduler()->getTime() + fDur;
        MESSAGE_SINGLE(NORMAL, logger, "Rx preamble, synced until " << lastFrameEnd);
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger, "Rx preamble, synced for " << fDur << ", afterwards garbled until " << lastFrameEnd);
    }
    this->synchronizedToAddress = transmitter;
    curState = Synchronized;
    numSpatialStreamsLastPreambleFragment = 1;
}

void FrameSynchronization::onTimeout()
{
    if(curState == Synchronized)
    {
        // finished reception
        MESSAGE_SINGLE(NORMAL, logger, "End of decoding, signal rxEnd");
        this->wns::Subject<IRxStartEnd>::forEachObserver(OnRxStartEnd(0, false, false));
        curState = waitForFinalDelivery;
        setTimeout(10e-9);
        return;
    }

    // Frame reception as indicated by the preamble is over, stop synchronization
    if(lastFrameEnd > wns::simulator::getEventScheduler()->getTime())
    {
        // there are still active transmissions, but the preamble was missed
        curState = Garbled;
        this->setTimeout(lastFrameEnd-wns::simulator::getEventScheduler()->getTime());
        MESSAGE_SINGLE(NORMAL, logger, "End of frame, change state to garbled until " << lastFrameEnd);
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger, "End of frame, change state to idle");
        curState = Idle;
    }
    this->synchronizedToAddress = wns::service::dll::UnicastAddress();
    numSpatialStreamsLastPreambleFragment = 0;
}

void FrameSynchronization::processPSDU(const wns::ldk::CompoundPtr& compound)
{
    assure(friends.manager->getFrameType(compound->getCommandPool()) != PREAMBLE,
           "called processPSDU for non-psdu");

    MESSAGE_BEGIN(NORMAL, logger, m, "");
    m << "Received psdu. Synchronized: " << (curState == Synchronized or curState == waitForFinalDelivery);
    m << "; Transmitter ok: " << (friends.manager->getTransmitterAddress(compound->getCommandPool()) == this->synchronizedToAddress);
    m << "; CRC ok: " << (getFUN()->getCommandReader(crcCommandName)->
                          readCommand<wns::ldk::crc::CRCCommand>(compound->getCommandPool())->
                          local.checkOK);
    MESSAGE_END();

    wns::Ratio sinr = getFUN()->getCommandReader(phyUserCommandName)->
        readCommand<wifimac::convergence::PhyUserCommand>(compound->getCommandPool())->getCIRwithoutMIMO();
    sinrProbe->put(compound, sinr.get_dB());

    double per = getFUN()->getCommandReader(errorModellingCommandName)->
        readCommand<wifimac::convergence::ErrorModellingCommand>(compound->getCommandPool())->getErrorRate();
    perProbe->put(compound, per);

    if((curState == Synchronized or curState == waitForFinalDelivery) and
       (friends.manager->getTransmitterAddress(compound->getCommandPool()) == this->synchronizedToAddress))
    {
        if(getFUN()->getCommandReader(crcCommandName)->readCommand<wns::ldk::crc::CRCCommand>(compound->getCommandPool())->local.checkOK)
        {
            MESSAGE_SINGLE(NORMAL, logger, "Received matching psdu for current synchronization");
            sinrMIB->putMeasurement(friends.manager->getTransmitterAddress(compound->getCommandPool()), sinr);
            successRateProbe->put(compound, 1);

            MESSAGE_SINGLE(NORMAL,logger,"lastFrameEnd: " << lastFrameEnd << " current time: " << wns::simulator::getEventScheduler()->getTime() << " -> till frame end: " << lastFrameEnd-wns::simulator::getEventScheduler()->getTime());
            if ((lastFrameEnd-wns::simulator::getEventScheduler()->getTime() < 10e-9) 
                and
                (curState == Synchronized))
            {
                MESSAGE_SINGLE(NORMAL,logger," frame end reached, cancel timeout on last PSDU");
                cancelTimeout();
                onTimeout();
            }

            getDeliverer()->getAcceptor(compound)->onData(compound);
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, logger, "Received (synchronized) psdu, but CRC error -> DROP");
            successRateProbe->put(compound, 0);
            // Signal rxError event
            this->wns::Subject<IRxStartEnd>::forEachObserver(OnRxStartEnd(0, false, true));
        }
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, logger, "Received psdu, but not synchronized -> DROP");
        successRateProbe->put(compound, 0);
        // signal rxError only in case the user specifically asked for this kind
        // of strange behavior (i.e. not synchronized --> CRC cannot be checked
        // at all!
        if(this->signalRxErrorAlthoughNotSynchronized)
        {
            this->wns::Subject<IRxStartEnd>::forEachObserver(OnRxStartEnd(0, false, true));
        }
    }

    // delivery occured
    if((curState == waitForFinalDelivery) and (friends.manager->getTransmitterAddress(compound->getCommandPool()) == this->synchronizedToAddress))
    {
        assure(hasTimeoutSet(), "State is waitForFinalDelivery, but no timeout set");
        cancelTimeout();
        onTimeout();
    }
}

void FrameSynchronization::onFUNCreated()
{
    sinrMIB = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::management::SINRInformationBase>(sinrMIBServiceName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

}

bool FrameSynchronization::doIsAccepting(const wns::ldk::CompoundPtr& compound) const
{
    return getConnector()->hasAcceptor(compound);
}

void FrameSynchronization::doWakeup()
{
    getReceptor()->wakeup();
}

void
FrameSynchronization::traceIncoming(wns::ldk::CompoundPtr compound)
{
    wns::probe::bus::json::Object objdoc;

    // Can be invalid => Broadcast
    wns::service::dll::UnicastAddress dstAdr = 
        friends.manager->getReceiverAddress(compound->getCommandPool());

    wns::service::dll::UnicastAddress srcAdr = 
        friends.manager->getTransmitterAddress(compound->getCommandPool());
    assure(srcAdr.isValid(), "Source address not set");

    wns::service::dll::UnicastAddress myAdr = friends.manager->getMACAddress();
    assure(srcAdr.isValid(), "MAC address not set");

    // Can be used to distiguish hop and e2e
    wns::service::dll::UnicastAddress senderAdr = 
        friends.manager->getTransmitterAddress(compound->getCommandPool());
    assure(senderAdr.isValid(), "Sender address not set");

    dll::ILayer2* srcLayer;
    dll::ILayer2* dstLayer;
    dll::ILayer2* senderLayer;
    dll::ILayer2* myLayer;

    myLayer = getFUN()->getLayer<dll::ILayer2*>();
    srcLayer = myLayer->getStationManager()->getStationByMAC(srcAdr);
    if(dstAdr.isValid())
        dstLayer = myLayer->getStationManager()->getStationByMAC(dstAdr);
    senderLayer = myLayer->getStationManager()->getStationByMAC(senderAdr);

    std::string src;
    std::string dst("Broadcast");
    std::string me;
    std::string sender;

    if(myLayer->getStationType() == wns::service::dll::StationTypes::UT())
    {
        std::stringstream s;
        s << "UT" << myAdr;
        me = s.str();
    }
    else
    {
        std::stringstream s;
        s << "BS" << myAdr;
        me = s.str();
    }
    if(srcLayer->getStationType() == wns::service::dll::StationTypes::UT())
    {
        std::stringstream s;
        s << "UT" << srcAdr;
        src = s.str();
    }
    else
    {
        std::stringstream s;
        s << "BS" << srcAdr;
        src = s.str();
    }
    if(dstAdr.isValid())
    {
        std::stringstream s;
        if(dstLayer->getStationType() == wns::service::dll::StationTypes::UT())
        {
            s << "UT" << dstAdr;
            dst = s.str();
        }
        else
        {
            s << "BS" << srcAdr;
            dst = s.str();
        }
    }
    if(senderLayer->getStationType() == wns::service::dll::StationTypes::UT())
    {
        std::stringstream s;
        s << "UT" << senderAdr;
        sender = s.str();
    }
    else
    {
        std::stringstream s;
        s << "BS" << senderAdr;
        sender = s.str();
    }

    objdoc["Transmission"]["ReceiverID"] = wns::probe::bus::json::String(me);
    objdoc["Transmission"]["SenderID"] = wns::probe::bus::json::String(sender);
    objdoc["Transmission"]["SourceID"] = wns::probe::bus::json::String(src);
    objdoc["Transmission"]["DestinationID"] = wns::probe::bus::json::String(dst);

    wns::simulator::Time now = wns::simulator::getEventScheduler()->getTime();
    wns::simulator::Time frameTxDuration = 
        getFUN()->getCommandReader(txDurationProviderCommandName)->
            readCommand<wifimac::convergence::TxDurationProviderCommand>(
                compound->getCommandPool())->getDuration();

    objdoc["Transmission"]["Start"] =
         wns::probe::bus::json::Number(now - frameTxDuration);
        
    objdoc["Transmission"]["Stop"] = wns::probe::bus::json::Number(now);

    // We abuse this to watch per station results
    objdoc["Transmission"]["Subchannel"] = wns::probe::bus::json::Number(srcAdr.getInteger());
    

    wns::Power rxPower = getFUN()->getCommandReader(phyUserCommandName)->
        readCommand<wifimac::convergence::PhyUserCommand>(
            compound->getCommandPool())->local.rxPower;
    wns::Power interference = getFUN()->getCommandReader(phyUserCommandName)->
        readCommand<wifimac::convergence::PhyUserCommand>(
            compound->getCommandPool())->local.interference;

    objdoc["Transmission"]["TxPower"] = 
        wns::probe::bus::json::Number(0.0); // Unknown
    objdoc["Transmission"]["RxPower"] = 
        wns::probe::bus::json::Number(rxPower.get_dBm());
    objdoc["Transmission"]["InterferencePower"] = 
        wns::probe::bus::json::Number(interference.get_dBm());

    wns::probe::bus::json::probeJSON(jsonTracing, objdoc);
}

