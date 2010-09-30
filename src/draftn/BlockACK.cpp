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

#include <WIFIMAC/draftn/BlockACK.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/probe/bus/utils.hpp>

using namespace wifimac::draftn;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::BlockACK,
    wns::ldk::FunctionalUnit,
    "wifimac.draftn.BlockACK",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::draftn::BlockACK,
    wns::ldk::probe::Probe,
    "wifimac.draftn.BlockACK",
    wns::ldk::FUNConfigCreator);

BlockACK::BlockACK(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::arq::ARQ(config_),
    wns::ldk::fu::Plain<BlockACK, BlockACKCommand>(fun),

    managerName(config_.get<std::string>("managerName")),
    rxStartEndName(config_.get<std::string>("rxStartEndName")),
    txStartEndName(config_.get<std::string>("txStartEndName")),
    sendBufferName(config_.get<std::string>("sendBufferName")),
    perMIBServiceName(config_.get<std::string>("perMIBServiceName")),
    sifsDuration(config_.get<wns::simulator::Time>("myConfig.sifsDuration")),
    maximumACKDuration(config_.get<wns::simulator::Time>("myConfig.maximumACKDuration")),
    ackTimeout(config_.get<wns::simulator::Time>("myConfig.ackTimeout")),
    blockACKPhyMode(config_.getView("myConfig.blockACKPhyMode")),
    capacity(config_.get<Bit>("myConfig.capacity")),
    maxOnAir(config_.get<size_t>("myConfig.maxOnAir")),
    baBits(config_.get<Bit>("myConfig.blockACKBits")),
    baReqBits(config_.get<Bit>("myConfig.blockACKRequestBits")),
    maximumTransmissions(config_.get<size_t>("myConfig.maximumTransmissions")),
    impatientBAreqTransmission(config_.get<bool>("myConfig.impatient")),
    currentTxQueue(NULL),
    rxQueues(),
    hasACKfor(),
    nextTransmissionSN(),
    baState(idle),

    logger(config_.get("logger"))
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");

    // read the localIDs from the config
    wns::probe::bus::ContextProviderCollection localContext(&fun->getLayer()->getContextProviderCollection());
    for (int ii = 0; ii<config_.len("localIDs.keys()"); ++ii)
    {
        std::string key = config_.get<std::string>("localIDs.keys()",ii);
        unsigned int value  = config_.get<unsigned int>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }
    numTxAttemptsProbe = wns::probe::bus::collector(localContext, config_, "numTxAttemptsProbeName");
    friends.manager = NULL;

    std::string pluginName = config_.get<std::string>("myConfig.sizeUnit");
    sizeCalculator = std::auto_ptr<wns::ldk::buffer::SizeCalculator>(wns::ldk::buffer::SizeCalculator::Factory::creator(pluginName)->create());
}

BlockACK::BlockACK(const BlockACK& other) :
    //CompoundHandlerInterface(other),
    CommandTypeSpecifierInterface(other),
    HasReceptorInterface(other),
    HasConnectorInterface(other),
    HasDelivererInterface(other),
    CloneableInterface(other),
    IOutputStreamable(other),
    PythonicOutput(other),
    FunctionalUnit(other),
    DelayedInterface(other),
    wns::ldk::arq::ARQ(other),
    wns::ldk::fu::Plain<BlockACK, BlockACKCommand>(other),
    wns::ldk::Delayed<BlockACK>(other),
    managerName(other.managerName),
    rxStartEndName(other.rxStartEndName),
    txStartEndName(other.txStartEndName),
    sendBufferName(other.sendBufferName),
    perMIBServiceName(other.perMIBServiceName),
    sifsDuration(other.sifsDuration),
    maximumACKDuration(other.maximumACKDuration),
    ackTimeout(other.ackTimeout),
    capacity(other.capacity),
    maxOnAir(other.maxOnAir),
    baBits(other.baBits),
    baReqBits(other.baReqBits),
    maximumTransmissions(other.maximumTransmissions),
    impatientBAreqTransmission(other.impatientBAreqTransmission),
    sizeCalculator(wns::clone(other.sizeCalculator))
{
}

BlockACK::~BlockACK()
{
    // clear queues
    rxQueues.clear();
    if (currentTxQueue != NULL)
    {
       delete currentTxQueue;
       currentTxQueue = NULL;
    }
}

void BlockACK::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");

    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);

    // Observe rxStart
    this->wns::Observer<wifimac::convergence::IRxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::RxStartEndNotification*>(rxStartEndName));

    // Observe txStartEnd
    this->wns::Observer<wifimac::convergence::ITxStartEnd>::startObserving
        (getFUN()->findFriend<wifimac::convergence::TxStartEndNotification*>(txStartEndName));

    // signal packet success/errors to MIB
    perMIB = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::PERInformationBase>(perMIBServiceName);
    friends.sendBuffer = getFUN()->findFriend<wns::ldk::DelayedInterface*>(sendBufferName);
} // BlockACK::onFUNCreated

unsigned int
BlockACK::storageSize() const
{
    unsigned int size = 0;

    if (currentTxQueue != NULL)
    {
        size = currentTxQueue->storageSize();
    }

    for(wns::container::Registry<wns::service::dll::UnicastAddress, ReceptionQueue*>::const_iterator it = rxQueues.begin();
        it != rxQueues.end();
        it++)
    {
        size += (*it).second->storageSize();
    }

    return(size);
} // BlockACK::size()


bool
BlockACK::doIsAccepting(const wns::ldk::CompoundPtr& compound) const
{
    return (((currentTxQueue == NULL) or
             (currentTxQueue->getReceiver() == friends.manager->getReceiverAddress(compound->getCommandPool())))
            and hasCapacity());
}


bool
BlockACK::hasCapacity() const
{
    return((this->storageSize() < this->capacity) and 
           ((currentTxQueue == NULL) or
            (not currentTxQueue->waitsForACK())) and
           ((currentTxQueue == NULL) or
            (currentTxQueue->storageSize() < this->maxOnAir)));
}

void
BlockACK::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(this->hasCapacity(), "processOutgoing although no capacity");

    if(friends.manager->lifetimeExpired(compound->getCommandPool()))
    {
        MESSAGE_SINGLE(NORMAL, logger, "outgoing compound has expired lifetime -> drop");
        return;
    }
    wns::service::dll::UnicastAddress receiver = friends.manager->getReceiverAddress(compound->getCommandPool());
    if (currentTxQueue != NULL and receiver == currentTxQueue->getReceiver())
    {
        // processed compound has the current receiver as destination, added to
        // compound block for next round
        MESSAGE_SINGLE(NORMAL, this->logger,"Next frame in a row  processed for receiver: " << currentTxQueue->getReceiver() << " with SN: " << currentTxQueue->getNextSN());
        currentTxQueue->processOutgoing(compound);
    }
    if (currentTxQueue == NULL)
    {
        if (not nextTransmissionSN.knows(receiver))
        {
            nextTransmissionSN.insert(receiver,0);
        }
        currentTxQueue = new TransmissionQueue(this,
                                               maxOnAir,
                                               0.0,
                                               receiver,
                                               nextTransmissionSN.find(receiver),
                                               perMIB,
                                               &sizeCalculator);

        MESSAGE_BEGIN(NORMAL, this->logger, m, "First frame in a row");
        m << " processed for receiver: " << currentTxQueue->getReceiver();
        m << " with SN: " << nextTransmissionSN.find(receiver);
        MESSAGE_END();
        currentTxQueue->processOutgoing(compound);
    }
    MESSAGE_SINGLE(NORMAL, this->logger, "Stored outgoing frame, remaining capacity " << this->capacity - this->storageSize());
}

void
BlockACK::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    wns::service::dll::UnicastAddress transmitter = friends.manager->getTransmitterAddress(compound->getCommandPool());

    if(getCommand(compound->getCommandPool())->isACK())
    {
        assure(currentTxQueue != NULL, "got ACK though no transmission queue");
        assure(transmitter == currentTxQueue->getReceiver(), "got ACK from wrong Station");
        assure(this->baState == receptionFinished or this->baState == receiving,
               "Received ACK but not waiting for one");


        //perMIB->onSuccessfullTransmission(currentTxQueue->getReceiver());
        this->processIncomingACKSNs(getCommand(compound->getCommandPool())->peer.ackSNs);
        return;

    } // if compound is ACK
    else
    {
        if(getCommand(compound->getCommandPool())->isACKreq())
        {
            if(not rxQueues.knows(transmitter))
            {
                MESSAGE_BEGIN(NORMAL, this->logger, m, "Received BA-REQ from unknown transmitter ");
                m << transmitter << " (this would not happen with correct BA-init)";
                m << " -> new rxQueue";
                MESSAGE_END();

                assure(not hasACKfor.isValid(), "Received BA-REQ from " << transmitter << ", but already existing BA for " << hasACKfor);

                rxQueues.insert(transmitter, new ReceptionQueue(this, getCommand(compound->getCommandPool())->peer.sn, transmitter));
            }
            rxQueues.find(transmitter)->processIncomingACKreq(compound);
            hasACKfor = transmitter;
            return;
        } // compound is ACK req
        else
        {
            if(not rxQueues.knows(transmitter))
            {
                MESSAGE_SINGLE(NORMAL, this->logger, "First frame from " << transmitter << " -> new rxQueue");
                rxQueues.insert(transmitter, new ReceptionQueue(this, getCommand(compound->getCommandPool())->peer.sn, transmitter));
            }
            rxQueues.find(transmitter)->processIncomingData(compound, (*sizeCalculator)(compound));
        } // is not ACK req
    } // is not ACK
} // BlockACK::processIncoming

void
BlockACK::processIncomingACKSNs(std::set<BlockACKCommand::SequenceNumber> ackSNs)
{
    if(hasTimeoutSet())
    {
        cancelTimeout();
    }

    // Reset the baState: Noting is send, not waiting for anything -> idle!
    this->baState = idle;

    // forward SNs to currentTxQueue
    currentTxQueue->processIncomingACK(ackSNs);

    if ((currentTxQueue->getNumWaitingPDUs()+currentTxQueue->getNumOnAirPDUs()) != 0)
    {
        // either not all compounds of the current send block have been
        // transmitted successfully or compounds for the same receiver
        // arrived while the current send block retransmitted unsuccessfully
        // sent compounds
        return;
    }

    // currentTxQueue is empty -> store SN and delete it
    nextTransmissionSN.update(currentTxQueue->getReceiver(),currentTxQueue->getNextSN());
    MESSAGE_SINGLE(NORMAL, this->logger, "No more waiting frames -> deleted currentTxQueue");
    delete currentTxQueue;
    currentTxQueue = NULL;
    return;
}


const wns::ldk::CompoundPtr
BlockACK::hasACK() const
{
    if(hasACKfor.isValid())
    {
        return(rxQueues.find(hasACKfor)->hasACK());
    }
    else
    {
        return wns::ldk::CompoundPtr();
    }
} // hasACK

wns::ldk::CompoundPtr
BlockACK::getACK()
{
    wns::service::dll::UnicastAddress adr = hasACKfor;
    hasACKfor = wns::service::dll::UnicastAddress();
    return(rxQueues.find(adr)->getACK());
} // getACK

const wns::ldk::CompoundPtr
BlockACK::hasData() const
{
    wns::ldk::CompoundPtr compound = friends.sendBuffer->hasSomethingToSend();
    if((hasCapacity()) and
       (compound != wns::ldk::CompoundPtr()) and
       (currentTxQueue != NULL) and
       (friends.manager->getReceiverAddress(compound->getCommandPool()) == currentTxQueue->getReceiver()))
    {
        // There are more compounds in the buffer and we still have capacity -> delay transmission
        return wns::ldk::CompoundPtr();
    }

    if (currentTxQueue != NULL)
    {
        return(currentTxQueue->hasData());
    }
    else
    {
        return wns::ldk::CompoundPtr();
    }
} // hasData

wns::ldk::CompoundPtr
BlockACK::getData()
{
    assure(hasData(), "trying to get data though there aren't any waiting");

    // get the next waiting compound
    wns::ldk::CompoundPtr it = currentTxQueue->getData();

    return(it);
} // getData


void
BlockACK::onTxStart(const wns::ldk::CompoundPtr& /*compound*/)
{
    baState = idle;
}

void
BlockACK::onTxEnd(const wns::ldk::CompoundPtr& compound)
{
    // we await the tx end of the blockACKreq
    if((getFUN()->getProxy()->commandIsActivated(compound->getCommandPool(), this)) and
       (getCommand(compound->getCommandPool())->isACKreq()) and 
       (currentTxQueue->getReceiver() == friends.manager->getReceiverAddress(compound->getCommandPool())))
    {
        assure(currentTxQueue != NULL, "TxEnd from BA-REQ, but no transmission queue");
        assure(currentTxQueue->waitsForACK(),
               "TxEnd from BA-REQ for current receiver " << currentTxQueue->getReceiver() << ", but queue is not waiting for ACK");

        // we give ackTimeout until the baState must be "receiving"
        setNewTimeout(ackTimeout);
        baState = waitForACK;
        MESSAGE_SINGLE(NORMAL, this->logger, "onTxEnd() of BAreq, wait on BA for " << ackTimeout);
    }
}

void
BlockACK::onTimeout()
{
    if(this->baState == receiving)
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Started reception during wait for BA -> wait for delivery");
        return;
    }

    // we did not receive anything after the blockACKreq transmission
    assure(currentTxQueue != NULL, "Timeout, but no transmission queue");
    assure(currentTxQueue->waitsForACK(), "Timeout, but currentTxQueue is not waiting for ACK");

    MESSAGE_SINGLE(NORMAL, this->logger, "Timeout -> failed transmission to " << currentTxQueue->getReceiver());
    //perMIB->onFailedTransmission(currentTxQueue->getReceiver());

    // no ACK'ed SNs have arrived, use pseudo-vector
    std::set<BlockACKCommand::SequenceNumber> none;
    none.clear();
    this->processIncomingACKSNs(none);

    this->tryToSend();
}

void
BlockACK::onTransmissionHasFailed(const wns::ldk::CompoundPtr& compound)
{
    assure(currentTxQueue != NULL, "transsmissionHasFailed, but there is no transmission queue");
    assure(currentTxQueue->waitsForACK(), "transmissionHasFailed, but currentTxQueue is not waiting for ACK");
    assure(friends.manager->getReceiverAddress(compound->getCommandPool()) == currentTxQueue->getReceiver(),
           "transmissionHasFailed has different rx address than current receiver");

    MESSAGE_SINGLE(NORMAL, this->logger, "Indication of failed transmission to " << friends.manager->getReceiverAddress(compound->getCommandPool()));
    // RTS/CTS -> no signalling to perMIB
    //this->printTxQueueStatus();


    // no ACK'ed SNs have arrived, use pseudo-vector
    std::set<BlockACKCommand::SequenceNumber> none;
    none.clear();
    this->processIncomingACKSNs(none);

    this->tryToSend();
}

void
BlockACK::onRxStart(wns::simulator::Time /*expRxTime*/)
{
    if(this->baState == waitForACK)
    {
        assure(hasTimeoutSet(), "ackState is waiting but no timeout set?");
        this->baState = receiving;
        MESSAGE_SINGLE(NORMAL, this->logger, "onRxStart() during wait for BA");
    }
}

void
BlockACK::onRxEnd()
{
    if(this->baState == receiving)
    {
        this->baState = receptionFinished;
        if(not hasTimeoutSet())
        {
            setTimeout(10e-9);
            MESSAGE_SINGLE(NORMAL, this->logger, "onRxEnd() during wait for BA -> short wait for delivery of BA");
        }
    }
}

void
BlockACK::onRxError()
{
    if(this->baState == receiving)
    {
        MESSAGE_SINGLE(NORMAL, logger, "onRxError and waiting for ACK -> failure");
        this->baState = waitForACK;

        if(not hasTimeoutSet())
        {
            this->onTimeout();
        }
    }
}

void
BlockACK::calculateSizes(const wns::ldk::CommandPool* commandPool, Bit& commandPoolSize, Bit& dataSize) const
{
    if(getCommand(commandPool)->isACK())
    {
        commandPoolSize = this->baBits;
        dataSize = 0;
        return;
    }

    if(getCommand(commandPool)->isACKreq())
    {
        commandPoolSize = this->baReqBits;
        dataSize = 0;
        return;
    }

    getFUN()->getProxy()->calculateSizes(commandPool, commandPoolSize, dataSize, this);
}

void BlockACK::printTxQueueStatus() const
{
    if (currentTxQueue != NULL)
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "TxQueue to " << currentTxQueue->getReceiver() << " waits for ACK: " << currentTxQueue->waitsForACK());
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "no transmission queue allocated yet");
    }
}

unsigned int
BlockACK::getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const
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
BlockACK::copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst)
{
    this->activateCommand(dst->getCommandPool())->localTransmissionCounter = this->getCommand(src)->localTransmissionCounter;
}

