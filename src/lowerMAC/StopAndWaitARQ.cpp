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
    stationShortRetryCounter(0),
    stationLongRetryCounter(0),
    shortRetryCounter(0),
    longRetryCounter(0),
    rtsctsThreshold(config.get<Bit>("rtsctsThreshold")),
    sifsDuration(config.get<wns::simulator::Time>("sifsDuration")),
    maximumACKDuration(config.get<wns::simulator::Time>("maximumACKDuration")),
    ackTimeout(config.get<wns::simulator::Time>("ackTimeout")),
    ackPhyMode(config.getView("ackPhyMode")),
    bianchiRetryCounter(config.get<bool>("bianchiRetryCounter")),
    ackState(none)
{
    friends.manager = NULL;

    // read the localIDs from the config
    wns::probe::bus::ContextProviderCollection localContext(&fuNet->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config.len("localIDs.keys()"); ++ii)
    {
        std::string key = config.get<std::string>("localIDs.keys()",ii);
        unsigned long int value  = config.get<unsigned long int>("localIDs.values()",ii);
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
    perMIB = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::management::PERInformationBase>(perMIBServiceName);
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

        MESSAGE_SINGLE(NORMAL, logger, "got rxStartIndication, wait for ACK delivery");
        ackState = receiving;
    }
}

void StopAndWaitARQ::onRxEnd()
{
    if (ackState == receiving)
    {
        assure(this->activeCompound, "state is receiving but no active compound");
        ackState = receptionFinished;
        if(not hasTimeoutSet())
        {
            setTimeout(10e-9);
            MESSAGE_SINGLE(NORMAL, logger, "onRxEnd and waiting for ACK -> set short timeout");
        }

    }
}

void StopAndWaitARQ::onRxError()
{
    if(ackState == receiving)
    {
        assure(this->activeCompound, "state is waitForACK/receiving but no active compound");
        MESSAGE_SINGLE(NORMAL, logger, "onRxError and waiting for ACK -> failure");

        // return to waitForACK
        ackState = waitForACK;

        if(not hasTimeoutSet())
        {
            this->onTimeout();
        }
    }
}

void
StopAndWaitARQ::onTxStart(const wns::ldk::CompoundPtr& compound)
{
    // this is just for the resetting of the short retry counters in the case
    // that a long frame used RTS/CTS for its transmission. If the tx starts AND
    // the data is long, the CTS was received and hence the counters can be
    // reset
    if(this->activeCompound and
       ((friends.manager->getFrameType(compound->getCommandPool()) == DATA) or (friends.manager->getFrameType(compound->getCommandPool()) == DATA_TXOP)) and
       (compound->getBirthmark() == this->activeCompound->getBirthmark()))
    {
        Bit commandPoolSize;
        Bit dataSize;
        this->calculateSizes(compound->getCommandPool(), commandPoolSize, dataSize);
        Bit psduSize = commandPoolSize + dataSize;

        if(psduSize > rtsctsThreshold)
        {
            MESSAGE_SINGLE(NORMAL, logger, "Data is sent --> CTS received successfully --> reset short retry counters");
            shortRetryCounter = 0;
            stationShortRetryCounter = 0;
        }
    }

}

void
StopAndWaitARQ::onTxEnd(const wns::ldk::CompoundPtr& compound)
{
    if(this->activeCompound and
       ((friends.manager->getFrameType(compound->getCommandPool()) == DATA) or (friends.manager->getFrameType(compound->getCommandPool()) == DATA_TXOP)) and
       (compound->getBirthmark() == this->activeCompound->getBirthmark()))
    {
        ackState = waitForACK;
        setNewTimeout(ackTimeout);
        MESSAGE_SINGLE(NORMAL, logger, "Data is sent, waiting for ACK for " << ackTimeout);
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

void StopAndWaitARQ::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    if(friends.manager->lifetimeExpired(compound->getCommandPool()))
    {
        MESSAGE_SINGLE(NORMAL, logger, "outgoing compound has expired lifetime -> drop");
    }
    else
    {
        // TODO: Set the frame exchange duration here
        friends.manager->setReplyTimeout(compound->getCommandPool(), ackTimeout);
        wns::ldk::arq::StopAndWait::processOutgoing(compound);

        if(not this->bianchiRetryCounter)
        {
            // According to the standard, the retransmission counter is set by
            // the station (short|long) counters
            getCommand(this->activeCompound->getCommandPool())->localTransmissionCounter =
                stationShortRetryCounter + stationLongRetryCounter + 1;
        }
    }
}

void StopAndWaitARQ::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    wns::ldk::arq::StopAndWaitCommand* command = this->getCommand(compound->getCommandPool());

    if(command->isACK())
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Received ACK frame, consider current frame as done");

        assure(this->ackState == receptionFinished, "Received ACK but not waiting for ack");
        assure(this->activeCompound, "Received ACK but no active compound");

        this->statusCollector->onSuccessfullTransmission(this->activeCompound);
        this->perMIB->onSuccessfullTransmission(friends.manager->getReceiverAddress(this->activeCompound->getCommandPool()));
        numTxAttemptsProbe->put(this->activeCompound, shortRetryCounter + longRetryCounter + 1);

        // received acknowledgement frame for the current compound --> reset counters
        Bit commandPoolSize;
        Bit dataSize;
        this->calculateSizes(activeCompound->getCommandPool(), commandPoolSize, dataSize);
        Bit psduSize = commandPoolSize + dataSize;

        if(psduSize > rtsctsThreshold)
        {
            longRetryCounter = 0;
            stationLongRetryCounter = 0;
        }
        else
        {
            shortRetryCounter = 0;
            stationShortRetryCounter = 0;
        }

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
        friends.manager->setPhyMode(ackPCI, ackPhyMode);
        wns::simulator::Time fxDur = friends.manager->getFrameExchangeDuration(compound->getCommandPool()) - sifsDuration - maximumACKDuration;
        if (fxDur < sifsDuration)
        {
            fxDur = 0;
        }

        MESSAGE_SINGLE(NORMAL,logger,"create ACK with exchange duration : " << fxDur);

        friends.manager->setFrameExchangeDuration(ackPCI,
                                                  fxDur);
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
StopAndWaitARQ::onTransmissionHasFailed(const wns::ldk::CompoundPtr& compound)
{
    // indication from RTS/CTS that the CTS was not received
    ++shortRetryCounter;
    ++stationShortRetryCounter;

    MESSAGE_BEGIN(NORMAL, logger, m, "Missing CTS, retry counters now:");
    m << " src " << shortRetryCounter;
    m << " ssrc " << stationShortRetryCounter;
    m << " (limit " << shortRetryLimit << ")";
    m << " lrc " << longRetryCounter;
    m << " slrc " << stationLongRetryCounter;
    m << " (limit " << longRetryLimit << ")";
    MESSAGE_END();

    this->transmissionHasFailed(compound);
}

void StopAndWaitARQ::onTimeout()
{
    if(ackState == receiving)
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Started reception during wait for ACK -> wait for delivery");
        return;
    }

    // ACK was not received before timeout
    assure(this->activeCompound, "no active compound, no failed transmission");

    // get the PSDU size
    Bit commandPoolSize;
    Bit dataSize;
    this->calculateSizes(activeCompound->getCommandPool(), commandPoolSize, dataSize);
    Bit psduSize = commandPoolSize + dataSize;

    if(psduSize <= rtsctsThreshold)
    {
        ++shortRetryCounter;
        ++stationShortRetryCounter;
    }
    else
    {
        ++longRetryCounter;
        ++stationLongRetryCounter;
    }

    MESSAGE_BEGIN(NORMAL, logger, m, "Timeout of ACK, retry counters now:");
    m << " src " << shortRetryCounter;
    m << " ssrc " << stationShortRetryCounter;
    m << " (limit " << shortRetryLimit << ")";
    m << " lrc " << longRetryCounter;
    m << " slrc " << stationLongRetryCounter;
    m << " (limit " << longRetryLimit << ")";
    MESSAGE_END();

    this->perMIB->onFailedTransmission(friends.manager->getReceiverAddress(activeCompound->getCommandPool()));
    this->transmissionHasFailed(activeCompound);
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

    if((shortRetryCounter == shortRetryLimit) or
       (longRetryCounter == longRetryLimit) or
       (friends.manager->lifetimeExpired(compound->getCommandPool())))
    {
        MESSAGE_SINGLE(NORMAL, logger, "Failed transmission, txCounter has reached limit -> drop packet");
        this->statusCollector->onFailedTransmission(this->activeCompound);
        numTxAttemptsProbe->put(this->activeCompound, shortRetryCounter + longRetryCounter + 1);
        // reset retry counters, but NOT station retry counters!
        shortRetryCounter = 0;
        longRetryCounter = 0;

        this->sendNow = false;
        this->activeCompound = wns::ldk::CompoundPtr();
        this->tryToSend();
    }
    else
    {
        wns::ldk::arq::StopAndWait::onTimeout();
        // The backoff will select the size of the contention window (cw)
        // according to the number or retries. Hence, setting this correctly is
        // important.

        if(this->bianchiRetryCounter)
        {
            // This implements the retry counter as described in the Bianchi DCF
            // model: Every new compound resets the number of retries,
            // independently from the failure of the compound before. Hence, the
            // number of transmissions is the src+lrc+1.

            getCommand(this->activeCompound->getCommandPool())->localTransmissionCounter =
                shortRetryCounter + longRetryCounter + 1;
        }
        else
        {
            // This implements the standard (IEEE 802.11-2007): The cw size is
            // set according to the station (short|long) retry counter, which
            // are the same as the src/lrc as long as every compound is
            // transmitted successful eventually, i.e. before the retry limit.
            // If the retry limit is reached the first time, the cw is reset
            // (and the compound discarded). If even the second compound cannot
            // be transmitted and is discarded, the cw is NOT reset any more.

            if(stationShortRetryCounter < shortRetryLimit and stationLongRetryCounter < longRetryLimit)
            {
                // no retry limit was reached so far
                getCommand(this->activeCompound->getCommandPool())->localTransmissionCounter =
                    stationShortRetryCounter + stationLongRetryCounter + 1;
            }
            else
            {
                int newCounter = 0;
                if(stationShortRetryCounter >= shortRetryLimit)
                {
                    // the shortRetryLimit was reached (at least) once -> reduce
                    // the retry counter
                    newCounter += stationShortRetryCounter - shortRetryLimit;
                }
                else
                {
                    newCounter += stationShortRetryCounter;
                }

                if(stationLongRetryCounter >= longRetryLimit)
                {
                    // the longRetryLimit was reached (at least) once -> reduce
                    // the retry counter
                    newCounter += stationLongRetryCounter - longRetryLimit;
                }
                else
                {
                    newCounter += stationLongRetryCounter;
                }

                // new transmission!
                newCounter += 1;

                assure(newCounter > 0, "Transmission counter must be greater than 0");

                getCommand(this->activeCompound->getCommandPool())->localTransmissionCounter = newCounter;
            }
        }
        MESSAGE_SINGLE(NORMAL, this->logger,
                       "Failed transmission, retransmit");
    }
}

unsigned int
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
