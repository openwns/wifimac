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


#include <WIFIMAC/lowerMAC/StopAndWaitARQ.hpp>
#include <WIFIMAC/FrameType.hpp>

#include <WNS/probe/bus/utils.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::StopAndWaitARQ,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.StopAndWaitARQ",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::StopAndWaitARQ,
    wns::ldk::probe::Probe,
    "wifimac.lowerMAC.StopAndWaitARQ",
    wns::ldk::FUNConfigCreator);

StopAndWaitARQ::StopAndWaitARQ(wns::ldk::fun::FUN* fuNet, const wns::pyconfig::View& config) :
    wns::ldk::arq::StopAndWait(fuNet, config),
    rxsName(config.get<std::string>("rxStartName")),
    txStartEndName(config.get<std::string>("txStartEndName")),
    managerName(config.get<std::string>("managerName")),
    perMIBServiceName(config.get<std::string>("perMIBServiceName")),
    shortRetryLimit(config.get<int>("shortRetryLimit")),
    longRetryLimit(config.get<int>("longRetryLimit")),
    rtsctsThreshold(config.get<Bit>("rtsctsThreshold")),
    sifsDuration(config.get<wns::simulator::Time>("sifsDuration")),
    expectedACKDuration(config.get<wns::simulator::Time>("expectedACKDuration")),
    preambleProcessingDelay(config.get<wns::simulator::Time>("preambleProcessingDelay")),
    ackPhyModeId(config.get<int>("ackPhyModeId")),
    ackState(none)
{
    friends.manager = NULL;

    // read the localIDs from the config
    wns::probe::bus::ContextProviderCollection localContext(&fuNet->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
    {
        std::string key = config.get<std::string>("localIDs.keys()",ii);
        uint32_t value  = config.get<uint32_t>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }
    numTxAttemptsProbe = wns::probe::bus::collector(localContext, config, "numTxAttemptsProbeName");
}

void StopAndWaitARQ::onFUNCreated()
{
    // Observe rxStart
    this->wns::Observer<wifimac::convergence::IRxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::RxStartEndNotification*>(rxsName));

    // Observe txStartEnd
    this->wns::Observer<wifimac::convergence::ITxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::TxStartEndNotification*>(txStartEndName));

    // get manager
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

    // signal packet success/errors to MIB
    perMIB = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::PERInformationBase>(perMIBServiceName);
}


wns::ldk::CompoundPtr StopAndWaitARQ::getData()
{
    sendNow = false;

    // send a copy
    return activeCompound->copy();
}

void StopAndWaitARQ::onRxStart(wns::simulator::Time /*expRxTime*/)
{
    if (ackState == waitForACK)
    {
        assure(this->activeCompound, "state is waitForACK but no active compound");
        assure(hasTimeoutSet(), "ackState is waiting but no timeout set?");
        MESSAGE_SINGLE(NORMAL, logger, "got rxStartIndication, cancel timeout");
        cancelTimeout();
        ackState = receiving;
    }
}

void StopAndWaitARQ::onRxEnd()
{
    if (ackState == receiving)
    {
        assure(this->activeCompound, "state is receiving but no active compound");
        MESSAGE_SINGLE(NORMAL, logger, "onRxEnd and waiting for ACK -> set short timeout");
        setTimeout(10e-9);
    }
}

void StopAndWaitARQ::onRxError()
{
    // we do nothing and let the timeout pass
}

void
StopAndWaitARQ::onTxStart(const wns::ldk::CompoundPtr& /*compound*/)
{

}

void
StopAndWaitARQ::onTxEnd(const wns::ldk::CompoundPtr& compound)
{
    if(this->activeCompound and
       ((friends.manager->getFrameType(compound->getCommandPool()) == DATA) or (friends.manager->getFrameType(compound->getCommandPool()) == DATA_TXOP)) and
       (compound->getBirthmark() == this->activeCompound->getBirthmark()))
    {
        ackState = waitForACK;
        setNewTimeout(sifsDuration + preambleProcessingDelay);
        MESSAGE_SINGLE(NORMAL, logger, "Data is sent, waiting for ACK for " << sifsDuration + preambleProcessingDelay);
    }
    else
    {
        if(this->ackState == sendingACK)
        {
            MESSAGE_SINGLE(NORMAL, logger, "done sending ack");
            this->ackState = none;
            this->tryToSend();
        }
    }
}

void StopAndWaitARQ::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    wns::ldk::arq::StopAndWaitCommand* command = this->getCommand(compound->getCommandPool());

    if(command->isACK())
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Received ACK frame, consider current frame as done");

        assure(this->ackState == waitForACK or this->ackState == receiving, "Received ACK but not waiting for ack");
        assure(this->activeCompound, "Received ACK but no active compound");

        // received acknowledgement frame for the current compound

        this->statusCollector->onSuccessfullTransmission(this->activeCompound);
        this->perMIB->onSuccessfullTransmission(friends.manager->getReceiverAddress(this->activeCompound->getCommandPool()));
        numTxAttemptsProbe->put(this->activeCompound, getCommand(this->activeCompound->getCommandPool())->localTransmissionCounter);

        this->activeCompound = wns::ldk::CompoundPtr();
        this->ackState = none;

        if(this->hasTimeoutSet())
        {
            this->cancelTimeout();
        }
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Received DATA frame, reply with ACK");

        // send ACK
        wns::ldk::CommandPool* ackPCI = this->getFUN()->getProxy()->createReply(compound->getCommandPool(), this);
        this->ackCompound = wns::ldk::CompoundPtr(new wns::ldk::Compound(ackPCI));
        friends.manager->setFrameType(ackPCI, ACK);
        friends.manager->setPhyMode(ackPCI, ackPhyModeId);
        friends.manager->setFrameExchangeDuration(ackPCI,
                                                  friends.manager->getFrameExchangeDuration(compound->getCommandPool()) - this->sifsDuration - this->expectedACKDuration);
        wns::ldk::arq::StopAndWaitCommand* ackCommand = this->activateCommand(ackPCI);
        ackCommand->peer.type = wns::ldk::arq::StopAndWaitCommand::RR;
        this->ackState = sendingACK;
        assure(getConnector()->hasAcceptor(this->ackCompound), "No acceptor for ACK");

        // deliver frame
        getDeliverer()->getAcceptor(compound)->onData(compound);
    }
}

bool
StopAndWaitARQ::hasCapacity() const
{
    return (!this->activeCompound) and (!this->ackCompound) and (this->ackState != sendingACK);
}

void
StopAndWaitARQ::transmissionHasFailed(const wns::ldk::CompoundPtr& compound)
{
    assure(this->activeCompound, "no active compound, no failed transmission");
    assure(friends.manager->getFrameType(compound->getCommandPool()) == DATA, "compound must have type DATA");
    assure(compound->getBirthmark() == this->activeCompound->getBirthmark(), "compound has not same birthmark as active one");

    if(hasTimeoutSet())
    {
        cancelTimeout();
    }

    ackState = none;

    wns::ldk::arq::StopAndWaitCommand* command = this->getCommand(this->activeCompound);

    // get the PSDU size
    Bit commandPoolSize;
    Bit dataSize;
    this->calculateSizes(activeCompound->getCommandPool(), commandPoolSize, dataSize);
    Bit psduSize = commandPoolSize + dataSize;

    // number of retries depends on the psduSize
    uint32_t retryLimit = (psduSize <= rtsctsThreshold) ? shortRetryLimit : longRetryLimit;

    // we count the transmissions (starting from 1), whereas the retry limit
    // counts the retries --> +1
    assure(command->localTransmissionCounter <= (retryLimit + 1),
           "localTransmissionCounter " << command->localTransmissionCounter << " is > retryLimit+1 " << (retryLimit+1));

    if(command->localTransmissionCounter == (retryLimit+1))
    {
        MESSAGE_SINGLE(NORMAL, logger, "Failed transmission, txCounter has reached limit of " << retryLimit << " retransmissions -> drop packet");
        this->statusCollector->onFailedTransmission(this->activeCompound);
        numTxAttemptsProbe->put(this->activeCompound, getCommand(this->activeCompound->getCommandPool())->localTransmissionCounter);
        this->sendNow = false;
        this->activeCompound = wns::ldk::CompoundPtr();
        this->tryToSend();
    }
    else
    {
        wns::ldk::arq::StopAndWait::onTimeout();
        MESSAGE_SINGLE(NORMAL, this->logger,
                       "Failed transmission, increase transmission counter to " << getCommand(activeCompound)->localTransmissionCounter);
    }
}

void StopAndWaitARQ::onTimeout()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "Timeout of ACK")
        this->perMIB->onFailedTransmission(friends.manager->getReceiverAddress(activeCompound->getCommandPool()));
    this->transmissionHasFailed(activeCompound);
}

size_t
StopAndWaitARQ::getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const
{
    if(getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this))
    {
        return(this->getCommand(compound)->localTransmissionCounter);
    }
    else
    {
        return 1;
    }
}

void
StopAndWaitARQ::copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst)
{
    wns::ldk::arq::StopAndWaitCommand* command = this->activateCommand(dst->getCommandPool());
    command->localTransmissionCounter = this->getCommand(src)->localTransmissionCounter;
}
