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
    preambleProcessingDelay(config_.get<wns::simulator::Time>("myConfig.preambleProcessingDelay")),
    blockACKPhyMode(config_.getView("myConfig.blockACKPhyMode")),
    capacity(config_.get<Bit>("myConfig.capacity")),
    maxOnAir(config_.get<size_t>("myConfig.maxOnAir")),
    baBits(config_.get<Bit>("myConfig.blockACKBits")),
    baReqBits(config_.get<Bit>("myConfig.blockACKRequestBits")),
    maximumTransmissions(config_.get<size_t>("myConfig.maximumTransmissions")),
    impatientBAreqTransmission(config_.get<bool>("myConfig.impatient")),
    currentReceiver(),
    txQueue(NULL),
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
    sendBuffer(other.sendBuffer),
    sifsDuration(other.sifsDuration),
    maximumACKDuration(other.maximumACKDuration),
    preambleProcessingDelay(other.preambleProcessingDelay),
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
    if (txQueue != NULL)
    {
       delete txQueue;
       txQueue = NULL;
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
    sendBuffer = getFUN()->findFriend<wns::ldk::DelayedInterface*>(sendBufferName);
} // BlockACK::onFUNCreated

unsigned int
BlockACK::storageSize() const
{
    unsigned int size = 0;

    if (txQueue != NULL)
    {
        size = txQueue->storageSize();
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
	return (((currentReceiver ==wns::service::dll::UnicastAddress()) 
		  or (currentReceiver == friends.manager->getReceiverAddress(compound->getCommandPool()))) 
		and hasCapacity());
}


bool
BlockACK::hasCapacity() const
{
    return((this->storageSize() < this->capacity) and 
           ((txQueue == NULL) or
            (not txQueue->waitsForACK())) and
           ((txQueue == NULL) or
            (txQueue->storageSize() < this->maxOnAir)));
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
    if (receiver == currentReceiver)
    {
        // processed compound has the current receiver as destination, added to
        // compound block for next round
        MESSAGE_SINGLE(NORMAL, this->logger,"Next frame in a row  processed for receiver: " << currentReceiver << " with SN: " << txQueue->getNextSN());
        txQueue->processOutgoing(compound, (*sizeCalculator)(compound));
    }
    if (currentReceiver == wns::service::dll::UnicastAddress())
    {
        // treatment of the very first compound to be proccessed by this transceiver
        if (txQueue == NULL)
        {
            if (not nextTransmissionSN.knows(receiver))
            {
                nextTransmissionSN.insert(receiver,0);
            }
            txQueue = new TransmissionQueue(this, this->maxOnAir, receiver, nextTransmissionSN.find(receiver), perMIB);
        }
        currentReceiver = receiver;
        MESSAGE_BEGIN(NORMAL, this->logger, m, "First frame in a row");
        m << " processed for receiver: " << currentReceiver;
        m << " with SN: " << nextTransmissionSN.find(receiver);
        MESSAGE_END();
        txQueue->processOutgoing(compound, (*sizeCalculator)(compound));
    }
    MESSAGE_SINGLE(NORMAL, this->logger, "Stored outgoing frame, remaining capacity " << this->capacity - this->storageSize());
}

void
BlockACK::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    wns::service::dll::UnicastAddress transmitter = friends.manager->getTransmitterAddress(compound->getCommandPool());

    if(getCommand(compound->getCommandPool())->isACK())
    {
        assure(currentReceiver != wns::service::dll::UnicastAddress(),"got ACK though nothing has been transmitted");
        assure(txQueue != NULL,"got ACK though no transmission queue");
        assure(transmitter == currentReceiver,"got ACK from wrong Station");
        assure(this->baState == waitForACK or this->baState == receiving,
               "Received ACK but not waiting for one");

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
    this->baState = idle;

    // forward SNs to txQueue
    txQueue->processIncomingACK(ackSNs);

    if ((txQueue->getNumWaitingPDUs()+txQueue->getNumOnAirPDUs()) != 0)
    {
        // either not all compounds of the current send block have been
        // transmitted successfully or compounds for the same receiver
        // arrived while the current send block retransmitted unsuccessfully
        // sent compounds
        return;
    }

    // txQueue is empty -> store SN and delete it
    nextTransmissionSN.update(currentReceiver,txQueue->getNextSN());
    MESSAGE_SINGLE(NORMAL, this->logger, "No more waiting frames -> deleted txQueue");
    delete txQueue;
    txQueue = NULL;
    currentReceiver = wns::service::dll::UnicastAddress();
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
    wns::ldk::CompoundPtr compound = sendBuffer->hasSomethingToSend();
    if(hasCapacity() and
       (compound != wns::ldk::CompoundPtr())
	and (friends.manager->getReceiverAddress(compound->getCommandPool()) == currentReceiver))
    {
        // There are more compounds in the buffer and we still have capacity -> delay transmission
        return wns::ldk::CompoundPtr();
    }

    if (txQueue != NULL)
    {
        return(txQueue->hasData());
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
    wns::ldk::CompoundPtr it = txQueue->getData();

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
       (currentReceiver == friends.manager->getReceiverAddress(compound->getCommandPool())))
    {
        assure(txQueue != NULL, "TxEnd from BA-REQ, but no transmission queue");
        assure(txQueue->waitsForACK(),
               "TxEnd from BA-REQ for current receiver " << currentReceiver << ", but queue is not waiting for ACK");
        setNewTimeout(sifsDuration + preambleProcessingDelay);
        baState = waitForACK;
        MESSAGE_SINGLE(NORMAL, this->logger, "onTxEnd() of BAreq, wait on BA for " << sifsDuration + preambleProcessingDelay);
    }
}

void
BlockACK::onTimeout()
{
    // we did not receive anything after the blockACKreq transmission
    assure(txQueue != NULL, "Timeout, but no transmission queue");
    assure(txQueue->waitsForACK(), "Timeout, but txQueue is not waiting for ACK");

    MESSAGE_SINGLE(NORMAL, this->logger, "Timeout -> failed transmission to " << currentReceiver)

    // no ACK'ed SNs have arrived, use pseudo-vector
    std::set<BlockACKCommand::SequenceNumber> none;
    none.clear();
    this->processIncomingACKSNs(none);

    this->tryToSend();
}

void
BlockACK::onTransmissionHasFailed(const wns::ldk::CompoundPtr& compound)
{
    assure(friends.manager->getReceiverAddress(compound->getCommandPool()) == this->currentReceiver,
           "transmissionHasFailed has different rx address than current receiver");
    assure(txQueue != NULL, "transsmissionHasFailed, but there is no transmission queue");
    assure(txQueue->waitsForACK(), "transmissionHasFailed, but txQueue is not waiting for ACK");

    MESSAGE_SINGLE(NORMAL, this->logger, "Indication of failed transmission to " << friends.manager->getReceiverAddress(compound->getCommandPool()));
    this->printTxQueueStatus();


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
        cancelTimeout();
        this->baState = receiving;
        MESSAGE_SINGLE(NORMAL, this->logger, "onRxStart() during wait for BA -> stop timeout");
    }
}

void
BlockACK::onRxEnd()
{
    if(this->baState == receiving)
    {
        setTimeout(10e-9);
        MESSAGE_SINGLE(NORMAL, this->logger, "onRxEnd() during wait for BA -> short wait for delivery of BA");
    }
}

void
BlockACK::onRxError()
{
    if(this->baState == receiving)
    {
        MESSAGE_SINGLE(NORMAL, logger, "onRxError and waiting for ACK -> failure");

        if(hasTimeoutSet())

        {
            cancelTimeout();
        }
        this->onTimeout();
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
    if (txQueue != NULL)
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "TxQueue to " << currentReceiver << " waits for ACK: " << txQueue->waitsForACK());
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



TransmissionQueue::TransmissionQueue(BlockACK* parent_,
                                     size_t maxOnAir_,
                                     wns::service::dll::UnicastAddress adr_,
                                     BlockACKCommand::SequenceNumber sn_,
                                     wifimac::management::PERInformationBase* perMIB_) :
    parent(parent_),
    maxOnAir(maxOnAir_),
    adr(adr_),
    txQueue(),
    onAirQueue(),
    nextSN(sn_),
    baREQ(),
    waitForACK(false),
    baReqRequired(false),
    perMIB(perMIB_)
{
    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << " created");

    // prepare Block ACK request
    this->baREQ = parent->friends.manager->createCompound(parent->friends.manager->getMACAddress(),
                                                          adr,
                                                          DATA,
                                                          parent->sifsDuration + parent->maximumACKDuration,
                                                          true);
    // block ack settings
    BlockACKCommand* baReqCommand = parent->activateCommand(this->baREQ->getCommandPool());
    baReqCommand->peer.type = BAREQ;
    baReqCommand->localTransmissionCounter = 1;

} // TransmissionQueue

TransmissionQueue::~TransmissionQueue()
{
    onAirQueue.clear();
    this->baREQ = wns::ldk::CompoundPtr();
} // TransmissionQueue::~TransmissionQueue


const unsigned int TransmissionQueue::onAirQueueSize() const
{
    unsigned int size = 0;
    for(std::deque<CompoundPtrWithSize>::const_iterator it = onAirQueue.begin();
        it != onAirQueue.end();
        it++)
    {
        size += it->second;
    }
    return(size);
}

const unsigned int TransmissionQueue::txQueueSize() const
{
    unsigned int size = 0;
    for(std::deque<CompoundPtrWithSize>::const_iterator it = txQueue.begin();
        it != txQueue.end();
        it++)
    {
        size += it->second;
    }
    return(size);
}


const unsigned int TransmissionQueue::storageSize() const
{
    return(this->onAirQueueSize() + this->txQueueSize());
}

void TransmissionQueue::processOutgoing(const wns::ldk::CompoundPtr& compound, const unsigned int size)
{
    BlockACKCommand* baCommand = parent->activateCommand(compound->getCommandPool());
    baCommand->peer.type = I;
    baCommand->peer.sn = this->nextSN++;
    baCommand->localTransmissionCounter = 1;

    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Queue outgoing compound with sn " << baCommand->peer.sn);
    txQueue.push_back(CompoundPtrWithSize(compound, size));
} // TransmissionQueue::processOutgoing

const wns::ldk::CompoundPtr TransmissionQueue::hasData() const
{
    if(waitForACK)
    {
        MESSAGE_SINGLE(VERBOSE, parent->logger, "TxQ" << adr << ": hasData: Waiting for ACK -> no other transmissions");
        // no other transmissions during waiting for ACK
        return(wns::ldk::CompoundPtr());
    }
    if((not txQueue.empty()) and (onAirQueueSize()+txQueue.front().second <= this->maxOnAir))
    {
        // regular frame pending
        MESSAGE_SINGLE(VERBOSE, parent->logger, "TxQ" << adr << ": hasData: Regular frame is pending");
        return(txQueue.front().first);
    }

    // baReq pending
    if(this->baReqRequired)
    {
        // impatient BAreq -> send as soon as required
        if(parent->impatientBAreqTransmission)
        {
            return(this->baREQ);
        }

        // next compound would exceed on air limit -> send
        if((not txQueue.empty()) and
           (onAirQueueSize()+txQueue.front().second > this->maxOnAir))
        {
            return(this->baREQ);
        }

        // no next compound, but nothing would match anyway -> send
        if((txQueue.empty()) and
           (onAirQueueSize() == this->maxOnAir))
        {
            return(this->baREQ);
        }
    }

    return(wns::ldk::CompoundPtr());

} // TransmissionQueue::hasData

wns::ldk::CompoundPtr TransmissionQueue::getData()
{
    assure(this->hasData(), "Called getData although hasData is false");

    // compound pending?
    if((not txQueue.empty()) and
       (onAirQueue.size()+txQueue.front().second <= this->maxOnAir))
    {
        // transmit another frame
        onAirQueue.push_back(txQueue.front());
        wns::ldk::CompoundPtr it = txQueue.front().first;
        txQueue.pop_front();

        MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Transmit pending frame with sn " << parent->getCommand(it->getCommandPool())->peer.sn);
        this->baReqRequired = true;

        return(it->copy());
    }

    // no compound pending --> BAreq must be pending
    assure(this->baReqRequired,
           "No compound pending and no baReqRequired, but hasData is true");
    if(txQueue.empty())
    {
        MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": No more frames to tx, send BAreq with start-sn " << parent->getCommand(onAirQueue.front().first)->peer.sn);
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Reached tx window, send BAreq with start-sn " << parent->getCommand(onAirQueue.front().first)->peer.sn);
    }

    wns::ldk::CompoundPtr it = this->baREQ->copy();
    // no more BAreq required until next regular frame transmission
    this->baReqRequired = false;
    // now set start-sn of BA-req to sn of first compound in transmission queue
    parent->getCommand(it->getCommandPool())->peer.sn = parent->getCommand(onAirQueue.front().first)->peer.sn;
    // stop future transmissions until ACK (or timeout of it) arrives
    this->waitForACK = true;
    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Set waitForACK to true");
    return(it);

} // TransmissionQueue::getData

void TransmissionQueue::processIncomingACK(std::set<BlockACKCommand::SequenceNumber> ackSNs)
{
    assure(not onAirQueue.empty(), "Received ACK but onAirQueue is empy");
    assure(this->waitForACK, "Received ACK but not waiting for one");
    this->waitForACK = false;
    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Received ACK, iterate through compounds on air");

    bool insertBack = false;
    bool blockACKsuccess = true;
    std::deque<CompoundPtrWithSize>::iterator txQueueFirst;
    if(txQueue.empty())
    {
        insertBack = true;
    }
    else
    {
        txQueueFirst = txQueue.begin();
    }

    std::set<BlockACKCommand::SequenceNumber>::iterator snIt = ackSNs.begin();
    assure(isSortedBySN(onAirQueue),
           "onAirQueue is not sorted by SN!");

    for(std::deque<CompoundPtrWithSize>::iterator onAirIt = onAirQueue.begin();
        onAirIt != onAirQueue.end();
        onAirIt++)
    {
        BlockACKCommand::SequenceNumber onAirSN = parent->getCommand((onAirIt->first)->getCommandPool())->peer.sn;

        if((snIt == ackSNs.end()) or (*snIt != onAirSN))
        {
             // retransmission
            int txCounter = ++(parent->getCommand((onAirIt->first)->getCommandPool())->localTransmissionCounter);
            perMIB->onFailedTransmission(adr);
	    blockACKsuccess = false;
            if(parent->getManager()->lifetimeExpired((onAirIt->first)->getCommandPool()))
            {
                MESSAGE_BEGIN(NORMAL, parent->logger, m, "TxQ" << adr << ":   Compound " << onAirSN);
                m << ", ackSN " << ((snIt == ackSNs.end()) ? -1 : (*snIt));
                m << " -> " << txCounter;
                m << " transmissions, lifetime expired --> drop!";
                MESSAGE_END();

                parent->numTxAttemptsProbe->put(onAirIt->first, txCounter);
            } // lifetime expired
            else
            {
                // BlockACK does not drop frames due to their number of
                // retransmissions, see IEEE 802.11-2007, 9.10.3
                MESSAGE_BEGIN(NORMAL, parent->logger, m, "TxQ" << adr << ":   Compound " << onAirSN);
                m << ", ackSN " << ((snIt == ackSNs.end()) ? -1 : (*snIt));
                m << " -> " << txCounter;
                m << " transmissions, retransmit";
                MESSAGE_END();

                if(insertBack)
                {
                    txQueue.push_back(*onAirIt);
                }
                else
                {
                    txQueue.insert(txQueueFirst, *onAirIt);
                }
            } // lifetime not expired
        } // SN does not match
        else
        {
            // *snIt is equal to sn from *onAirIt --> success, go to next sn
            MESSAGE_BEGIN(NORMAL, parent->logger, m, "TxQ" << adr << ":   Compound " << onAirSN);
            m << ", ackSN " << (*snIt) << " -> success";
            MESSAGE_END();
            snIt++;

            perMIB->onSuccessfullTransmission(adr);
            parent->numTxAttemptsProbe->put(onAirIt->first, parent->getCommand((onAirIt->first)->getCommandPool())->localTransmissionCounter);
        } // SN matches
    } // for-loop over onAirQueue

    // nothing is onAir now
    for (int i=0; i < parent->observers.size(); i++)
    {
	parent->observers[i]->onBlockACKReception(blockACKsuccess);
    }
    onAirQueue.clear();

} // TransmissionQueue::processACK
  
bool TransmissionQueue::isSortedBySN(const std::deque<CompoundPtrWithSize> q) const
{
    if(q.empty())
    {
        return true;
    }
    std::deque<CompoundPtrWithSize>::const_iterator it = q.begin();
    BlockACKCommand::SequenceNumber lastSN = parent->getCommand((it->first)->getCommandPool())->peer.sn;
    ++it;

    for(;
        it != q.end();
        it++)
    {
        BlockACKCommand::SequenceNumber curSN = parent->getCommand((it->first)->getCommandPool())->peer.sn;
        if(curSN < lastSN)
        {
            return false;
        }
        lastSN = curSN;
    }
    return true;

}

const bool TransmissionQueue::waitsForACK() const
{
    return(this->waitForACK);
}

ReceptionQueue::ReceptionQueue(BlockACK* parent_, BlockACKCommand::SequenceNumber firstSN_, wns::service::dll::UnicastAddress adr_):
    parent(parent_),
    adr(adr_),
    waitingForSN(firstSN_),
    rxStorage(),
    rxSNs(),
    blockACK()
{
    MESSAGE_SINGLE(NORMAL, parent->logger, "RxQ" << adr << " created");
} // ReceptionQueue

const unsigned int ReceptionQueue::storageSize() const
{
    unsigned int size = 0;

    for(std::map<BlockACKCommand::SequenceNumber, CompoundPtrWithSize>::const_iterator it = rxStorage.begin();
        it != rxStorage.end();
        it++)
    {
        size += it->second.second;
    }
    return(size);
} // ReceptionQueue::size

void ReceptionQueue::processIncomingData(const wns::ldk::CompoundPtr& compound, const unsigned int size)
{
    assure(this->blockACK == wns::ldk::CompoundPtr(), "Received blockACKreq - cannot process more incoming data");

    BlockACKCommand* baCommand = parent->getCommand(compound->getCommandPool());
    assure(baCommand->peer.type == I, "can only process incoming data");

    // store sn for ack
    rxSNs.insert(baCommand->peer.sn);

    // process data compound
    if(baCommand->peer.sn == this->waitingForSN)
    {
        MESSAGE_BEGIN(NORMAL, parent->logger, m, "RxQ" << adr << ": Received SN ");
        m << baCommand->peer.sn;
        m << ", waiting for " << this->waitingForSN;
        m << " --> deliver ";
        MESSAGE_END();

        parent->getDeliverer()->getAcceptor(compound)->onData(compound);
        ++this->waitingForSN;
        // check if other compounds from the rxQueue can be delivered now
        this->purgeRxStorage();
    }
    else
    {
        // received compound does not match waitingForSN

        if((baCommand->peer.sn < this->waitingForSN) or (rxStorage.find(baCommand->peer.sn) != rxStorage.end()))
        {
            // received old compound
            MESSAGE_BEGIN(NORMAL, parent->logger, m, "RxQ" << adr << ": Received known SN ");
            m << baCommand->peer.sn;
            m << ", waiting for " << this->waitingForSN;
            m << " --> drop";
            MESSAGE_END();
        }
        else
        {
            // received new compound
            MESSAGE_BEGIN(NORMAL, parent->logger, m, "RxQ" << adr << ": Received new SN ");
            m << baCommand->peer.sn;
            m << ", waiting for " << this->waitingForSN;
            m << " --> store";
            MESSAGE_END();
            rxStorage[baCommand->peer.sn] = CompoundPtrWithSize(compound, size);
        }
    }

} // ReceptionQueue::processIncomingData

void ReceptionQueue::purgeRxStorage()
{
   while(not rxStorage.empty())
   {
       if(rxStorage.begin()->first == this->waitingForSN)
       {
           MESSAGE_BEGIN(NORMAL, parent->logger, m, "RxQ" << adr << ": New SN allows in-order delivery SN ");
           m << this->waitingForSN;
           m << " --> deliver";
           MESSAGE_END();

           parent->getDeliverer()->getAcceptor(rxStorage.begin()->second.first)->onData(rxStorage.begin()->second.first);
           rxStorage.erase(rxStorage.begin());
           ++this->waitingForSN;
       }
       else
       {
           // gap still exists
           return;
       }
   }
} // ReceptionQueue::purgeRxStorage

void ReceptionQueue::processIncomingACKreq(const wns::ldk::CompoundPtr& compound)
{
    assure(this->blockACK == wns::ldk::CompoundPtr(), "Already prepared blockACK");

    // the sn in the BAreq gives the minimum SN that shall be acknowledged, all
    // frames before that sn are discarded due to maximum lifetime or
    // retransmissions
    BlockACKCommand::SequenceNumber minSN = parent->getCommand(compound->getCommandPool())->peer.sn;
    // stop waiting for everything below the minSN
    while((not rxStorage.empty()) and (rxStorage.begin()->first < minSN))
    {
        MESSAGE_SINGLE(NORMAL, parent->logger, "RxQ" << adr << ": Received BAreq with SN " << minSN << " -> deliver waiting SN " << rxStorage.begin()->first);
        parent->getDeliverer()->getAcceptor(rxStorage.begin()->second.first)->onData(rxStorage.begin()->second.first);
        rxStorage.erase(rxStorage.begin());
    }
    // waitingForSN is never reduced
    this->waitingForSN = (minSN > this->waitingForSN) ? (minSN) : (this->waitingForSN);
    // shift of waitingForSN might free already received frames
    this->purgeRxStorage();

    // do not acknowledge old frames
    while((not rxSNs.empty()) and (*(rxSNs.begin()) < minSN))
    {
        rxSNs.erase(rxSNs.begin());
    }

    // create BlockACK
   wns::simulator::Time fxDur = parent->friends.manager->getFrameExchangeDuration(compound->getCommandPool()) - parent->sifsDuration - parent->maximumACKDuration;
   if (fxDur < parent->sifsDuration)
   {
	fxDur = 0;
    }

MESSAGE_SINGLE(NORMAL,parent->logger,"create BA with exchange duration : " << fxDur);
    this->blockACK = parent->friends.manager->createCompound(parent->friends.manager->getMACAddress(),
                                                             adr,
                                                             ACK,
							     fxDur,
                                                             false);
    parent->friends.manager->setPhyMode(this->blockACK->getCommandPool(),
                                        parent->blockACKPhyMode);

    // set baCommand information
    BlockACKCommand* baCommand = parent->activateCommand(this->blockACK->getCommandPool());
    baCommand->peer.type = BA;
    baCommand->peer.ackSNs = rxSNs;
    if(rxSNs.empty())
    {
        baCommand->peer.sn = this->waitingForSN;
    }
    else
    {
        // take the minimum of waiting for and rxSNs
        baCommand->peer.sn = (*(rxSNs.begin())) < this->waitingForSN ? (*(rxSNs.begin())) : this->waitingForSN;
    }

    MESSAGE_BEGIN(NORMAL, parent->logger, m, "RxQ" << adr << ": Received BAreq with SN ");
    m << minSN;
    m << ", prepare BA with ";
    m << " fExDur " << parent->friends.manager->getFrameExchangeDuration(this->blockACK->getCommandPool());
    m << " startSN: " << baCommand->peer.sn;
    m << " ackSNs:";
#ifndef WNS_NO_LOGGING
    for(std::set<BlockACKCommand::SequenceNumber>::iterator snIt = rxSNs.begin();
        snIt != rxSNs.end();
        snIt++)
    {
        m << " " << (*snIt);
    }
#endif
    MESSAGE_END();

    rxSNs.clear();

} // ReceptionQueue::processIncomingACKreq

const wns::ldk::CompoundPtr ReceptionQueue::hasACK() const
{
    return(this->blockACK);
} // ReceptionQueue::hasACK

wns::ldk::CompoundPtr ReceptionQueue::getACK()
{
    assure(hasACK(), "Called getACK although no ack prepared?");
    assure(this->blockACK != wns::ldk::CompoundPtr(), "Called getACK but blockACK is empty?");

    wns::ldk::CompoundPtr it = this->blockACK;
    this->blockACK = wns::ldk::CompoundPtr();

    MESSAGE_SINGLE(NORMAL, parent->logger, "RxQ" << adr << ": Transmit BA");

    return(it);
} // ReceptionQueue::getACK
