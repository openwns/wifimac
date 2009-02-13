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

#include <WIFIMAC/lowerMAC/BlockACK.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/probe/bus/utils.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::BlockACK,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.BlockACK",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::lowerMAC::BlockACK,
    wns::ldk::probe::Probe,
    "wifimac.lowerMAC.BlockACK",
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
    expectedACKDuration(config_.get<wns::simulator::Time>("myConfig.expectedACKDuration")),
    preambleProcessingDelay(config_.get<wns::simulator::Time>("myConfig.preambleProcessingDelay")),
    capacity(config_.get<Bit>("myConfig.capacity")),
    maxOnAir(config_.get<size_t>("myConfig.maxOnAir")),
    baBits(config_.get<Bit>("myConfig.blockACKBits")),
    baReqBits(config_.get<Bit>("myConfig.blockACKRequestBits")),
    maximumTransmissions(config_.get<size_t>("myConfig.maximumTransmissions")),
    impatientBAreqTransmission(config_.get<bool>("myConfig.impatientBAreqTransmission")),
    currentReceiver(),
    nextReceiver(),
    txQueue(NULL),
    rxQueues(),
    hasACKfor(),
    nextFirstCompound(),
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
        uint32_t value  = config_.get<uint32_t>("localIDs.values()",ii);
        localContext.addProvider(wns::probe::bus::contextprovider::Constant(key, value));
        MESSAGE_SINGLE(VERBOSE, logger, "Using Local IDName '"<<key<<"' with value: "<<value);
    }
    numTxAttemptsProbe = wns::probe::bus::collector(localContext, config_, "numTxAttemptsProbeName");
    friends.manager = NULL;
}


BlockACK::~BlockACK()
{
    // clear queues
    rxQueues.clear();
    if (txQueue != NULL)
	delete txQueue;
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
}

Bit BlockACK::sizeInBit() const
{
    Bit size = 0;

    if (txQueue != NULL)
	    size = txQueue->sizeInBit();

    for(wns::container::Registry<wns::service::dll::UnicastAddress, ReceptionQueue*>::const_iterator it = rxQueues.begin();
        it != rxQueues.end();
        it++)
    {
        size += (*it).second->sizeInBit();
    }

    return(size);
}

bool
BlockACK::hasCapacity() const
{ // if there last compound to be processed has a different receiver as the current one, don't accept further compounds
  // till current send block is transmitted successfully
  // OR 
  // buffer capacity has been reached
    return((this->sizeInBit() < this->capacity) and (currentReceiver == nextReceiver));
}

void
BlockACK::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    assure(this->hasCapacity(), "processOutgoing although no capacity");

    wns::service::dll::UnicastAddress receiver = friends.manager->getReceiverAddress(compound->getCommandPool());
    if (receiver == currentReceiver) 
    { // processed compound has the current receiver as destination, added to compound block for next round
	MESSAGE_SINGLE(NORMAL, this->logger,"Next frame in a row  processed for receiver: " << currentReceiver << " with SN: " << txQueue->getNextSN());
	txQueue->processOutgoing(compound);
    }
    else
    {
      // processed compound will be sent to a different receiver thus it is not added to current compound block but temporarily stored
      // the compound will become the first compound of the next transmission block
      MESSAGE_SINGLE(NORMAL, this->logger, "First frame to " << receiver << " -> current queue finished, temporarily store compound");
      nextFirstCompound = compound;
    }
    if (currentReceiver == wns::service::dll::UnicastAddress()) 
    { // treatment of the very first compound to be proccessed by this transceiver
	if (txQueue == NULL) {
		if (!nextTransmissionSN.knows(receiver))
			nextTransmissionSN.insert(receiver,0);
		txQueue = new TransmissionQueue(this,this->maxOnAir, receiver, nextTransmissionSN.find(receiver),perMIB);
	}
	currentReceiver = receiver;
	MESSAGE_SINGLE(NORMAL, this->logger,"First frame in a (possible) row  processed for receiver: " << currentReceiver << " with SN: 0");
	txQueue->processOutgoing(nextFirstCompound);
	nextFirstCompound = wns::ldk::CompoundPtr();
    }
    nextReceiver = receiver;   
    MESSAGE_SINGLE(NORMAL, this->logger, "Stored outgoing frame, remaining capacity " << this->capacity - this->sizeInBit());
}

void
BlockACK::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    uint32_t i;

    wns::service::dll::UnicastAddress transmitter = friends.manager->getTransmitterAddress(compound->getCommandPool());
    if(getCommand(compound->getCommandPool())->isACK())
    {
	assure(currentReceiver != wns::service::dll::UnicastAddress(),"got ACK though nothing has been transmitted");
	assure(txQueue != NULL,"got ACK though no transmission queue");	
	assure(transmitter == currentReceiver,"got ACK from wrong Station");
        assure(this->baState == waitForACK or this->baState == receiving,
               "Received ACK but not waiting for one");

        if(hasTimeoutSet())
        {
            cancelTimeout();
        }
        this->baState = idle;
        txQueue->processIncomingACK(getCommand(compound->getCommandPool())->peer.ackSNs);
	if (txQueue->waitingSize() != 0)
	{ 
		// either not all compounds of current send block have been transmitted successfully or
		// compounds for the same receiver arrived while the current send block retransmitted 
		// unsuccessfully sent compounds
		return;
	}
	if (nextReceiver != currentReceiver)
	{
		// txQueue is empty and there is a compound for a different receiver waiting, temporarily stored
	 	// store the current SN of the txQueue for current receiver and prepare transmission queue for next send block
	 	// using the temporarily stored compound as head and the next SN to be used for this link
		nextTransmissionSN.update(currentReceiver,txQueue->getNextSN());
		delete txQueue;
		if (!nextTransmissionSN.knows(nextReceiver))
			nextTransmissionSN.insert(nextReceiver,0);
		txQueue = new TransmissionQueue(this,this->maxOnAir, nextReceiver, nextTransmissionSN.find(nextReceiver),perMIB);
		MESSAGE_SINGLE(NORMAL, this->logger,"First frame in a (possible) row  processed for receiver: " << nextReceiver << " with StartSN: " << nextTransmissionSN.find(nextReceiver));
		txQueue->processOutgoing(nextFirstCompound);
		nextFirstCompound = wns::ldk::CompoundPtr();
		currentReceiver = nextReceiver;
	}	
        return;
    }

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
    }

    if(not rxQueues.knows(transmitter))
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "First frame from " << transmitter << " -> new rxQueue");
        rxQueues.insert(transmitter, new ReceptionQueue(this, getCommand(compound->getCommandPool())->peer.sn, transmitter));
    }
    rxQueues.find(transmitter)->processIncomingData(compound);
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
        if (txQueue != NULL)
                return(txQueue->hasData());
        else
                return wns::ldk::CompoundPtr();
} // hasData

wns::ldk::CompoundPtr
BlockACK::getData()
{
    assure(hasData(), "trying to get data though there aren't any waiting");

    // get the next waiting compound
    wns::ldk::CompoundPtr it = txQueue->getData();

    // if there are more compounds in the send buffer FU above the BlockACK for the same receiver, which haven't been processed yet
    // and the BlockACK FU accepts them, "manually" store the next compound waiting in the send buffer FU
    if ((txQueue->waitingSize() == 0) and (hasCapacity()) and (sendBuffer->hasSomethingToSend() != wns::ldk::CompoundPtr()))
    {
	MESSAGE_SINGLE(NORMAL, this->logger, "getData(): one look ahead insert of next compound from send buffer")
	processOutgoing(sendBuffer->getSomethingToSend());
    }
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

    txQueue->missingACK();
    this->baState = idle;
    this->tryToSend();
}

void
BlockACK::transmissionHasFailed(const wns::ldk::CompoundPtr& compound)
{
    assure(friends.manager->getReceiverAddress(compound->getCommandPool()) == this->currentReceiver,
           "transmissionHasFailed has different rx address than current receiver");
    assure(txQueue != NULL, "transsmissionHasFailed, but there is no transmission queue");
    assure(txQueue->waitsForACK(), "transmissionHasFailed, but txQueue is not waiting for ACK");

    MESSAGE_SINGLE(NORMAL, this->logger, "Indication of failed transmission to " << friends.manager->getReceiverAddress(compound->getCommandPool()));
    this->printTxQueueStatus();

    if(hasTimeoutSet())
    {
        cancelTimeout();
    }

    txQueue->missingACK();
    this->baState = idle;
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
    // we do nothing and let the timeout pass
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

size_t
BlockACK::getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const
{
    return(this->getCommand(compound)->localTransmissionCounter);
}

void
BlockACK::copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst)
{
    this->activateCommand(dst->getCommandPool())->localTransmissionCounter = this->getCommand(src)->localTransmissionCounter;
}



TransmissionQueue::TransmissionQueue(BlockACK* parent_, size_t maxOnAir_, wns::service::dll::UnicastAddress adr_, 
					BlockACKCommand::SequenceNumber sn_,  wifimac::management::PERInformationBase* perMIB_) :
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
                                                          parent->sifsDuration + parent->expectedACKDuration,
                                                          true);
    // block ack settings
    BlockACKCommand* baReqCommand = parent->activateCommand(this->baREQ->getCommandPool());
    baReqCommand->peer.type = BAREQ;
    baReqCommand->localTransmissionCounter = 1;
    // do not yet set the start sn
    //baReqCommand->peer.sn = 0

} // TransmissionQueue

TransmissionQueue::~TransmissionQueue()
{
    onAirQueue.clear();
    this->baREQ = wns::ldk::CompoundPtr();
} // TransmissionQueue::~TransmissionQueue


const Bit TransmissionQueue::sizeInBit() const
{
    Bit size = 0;

    for(std::deque<CompoundPtrWithSize>::const_iterator it = onAirQueue.begin();
        it != onAirQueue.end();
        it++)
    {
        size += it->second;
    }

    for(std::deque<CompoundPtrWithSize>::const_iterator it = txQueue.begin();
        it != txQueue.end();
        it++)
    {
        size += it->second;
    }
    return(size);
}

void TransmissionQueue::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    BlockACKCommand* baCommand = parent->activateCommand(compound->getCommandPool());
    baCommand->peer.type = I;
    baCommand->peer.sn = this->nextSN++;
    baCommand->localTransmissionCounter = 1;

    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Queue outgoing compound with sn " << baCommand->peer.sn);
    txQueue.push_back(CompoundPtrWithSize(compound, compound->getLengthInBits()));
} // TransmissionQueue::processOutgoing

const wns::ldk::CompoundPtr TransmissionQueue::hasData() const
{
    if(waitForACK)
    {
        MESSAGE_SINGLE(VERBOSE, parent->logger, "TxQ" << adr << ": hasData: Waiting for ACK -> no other transmissions");
        // no other transmissions during waiting for ACK
        return(wns::ldk::CompoundPtr());
    }
    if((not txQueue.empty()) and (onAirQueue.size() < this->maxOnAir))
    {
        // regular frame pending
        MESSAGE_SINGLE(VERBOSE, parent->logger, "TxQ" << adr << ": hasData: Regular frame is pending");
        return(txQueue.front().first);
    }

    if(this->baReqRequired)
    {
        if((parent->impatientBAreqTransmission) or
           (onAirQueue.size() == this->maxOnAir))
        {
            return(this->baREQ);
        }
    }

    return(wns::ldk::CompoundPtr());

} // TransmissionQueue::hasData

wns::ldk::CompoundPtr TransmissionQueue::getData()
{
    assure(this->hasData(), "Called getData although hasData is false");

    if((not txQueue.empty()) and (onAirQueue.size() < this->maxOnAir))
    {
        // transmit another frame
        onAirQueue.push_back(txQueue.front());
        wns::ldk::CompoundPtr it = txQueue.front().first;
        txQueue.pop_front();

        MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Transmit pending frame with sn " << parent->getCommand(it->getCommandPool())->peer.sn);
        this->baReqRequired = true;

        return(it->copy());
    }

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

    for(std::deque<CompoundPtrWithSize>::iterator onAirIt = onAirQueue.begin();
        onAirIt != onAirQueue.end();
        onAirIt++)
    {
        BlockACKCommand::SequenceNumber onAirSN = parent->getCommand((onAirIt->first)->getCommandPool())->peer.sn;

        if((snIt == ackSNs.end()) or (*snIt != onAirSN))
        {
             // retransmission
            if(++(parent->getCommand((onAirIt->first)->getCommandPool())->localTransmissionCounter) > parent->maximumTransmissions)
            {
                MESSAGE_BEGIN(NORMAL, parent->logger, m, "TxQ" << adr << ":   Compound " << onAirSN);
                m << ", ackSN " << ((snIt == ackSNs.end()) ? -1 : (*snIt));
                m << " -> reached max transmissions, drop";
                MESSAGE_END();
                parent->numTxAttemptsProbe->put(onAirIt->first, parent->getCommand((onAirIt->first)->getCommandPool())->localTransmissionCounter);
		perMIB->onFailedTransmission(adr);
            }
            else
            {
                 MESSAGE_BEGIN(NORMAL, parent->logger, m, "TxQ" << adr << ":   Compound " << onAirSN);
                 m << ", ackSN " << ((snIt == ackSNs.end()) ? -1 : (*snIt));
                 m << " -> retransmit";
                 MESSAGE_END();
		perMIB->onFailedTransmission(adr);

                if(insertBack)
                {
                    txQueue.push_back(*onAirIt);
                }
                else
                {
                    txQueue.insert(txQueueFirst, *onAirIt);
                }
            }
        }
        else
        {
            // *snIt is equal to sn from *onAirIt --> success, go to next sn
            MESSAGE_BEGIN(NORMAL, parent->logger, m, "TxQ" << adr << ":   Compound " << onAirSN);
            m << ", ackSN " << (*snIt) << " -> success";
            MESSAGE_END();
            snIt++;

            perMIB->onSuccessfullTransmission(adr);
            parent->numTxAttemptsProbe->put(onAirIt->first, parent->getCommand((onAirIt->first)->getCommandPool())->localTransmissionCounter);
        }
    }

    // nothing is onAir now
    onAirQueue.clear();

} // TransmissionQueue::processACK

void TransmissionQueue::missingACK()
{
    std::set<BlockACKCommand::SequenceNumber> none;
    none.clear();
    this->processIncomingACK(none);
} // TransmissionQueue::missingACK

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

const size_t ReceptionQueue::size() const
{
    return(rxStorage.size());
} // ReceptionQueue::size

const Bit ReceptionQueue::sizeInBit() const
{
    Bit size = 0;

    for(std::map<BlockACKCommand::SequenceNumber, CompoundPtrWithSize>::const_iterator it = rxStorage.begin();
        it != rxStorage.end();
        it++)
    {
        size += it->second.second;
    }
    return(size);
}

void ReceptionQueue::processIncomingData(const wns::ldk::CompoundPtr& compound)
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
            rxStorage[baCommand->peer.sn] = CompoundPtrWithSize(compound, compound->getLengthInBits());
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
    // frames before that sn are discarded due to maximum lifetime or retransmissions
    BlockACKCommand::SequenceNumber minSN = parent->getCommand(compound->getCommandPool())->peer.sn;
    // stop waiting for everything below the minSN
    while((not rxStorage.empty()) and (rxStorage.begin()->first < minSN))
    {
        MESSAGE_SINGLE(NORMAL, parent->logger, "RxQ" << adr << ": Received BAreq with SN " << minSN << " -> deliver waiting SN " << rxStorage.begin()->first);
        parent->getDeliverer()->getAcceptor(rxStorage.begin()->second.first)->onData(rxStorage.begin()->second.first);
        rxStorage.erase(rxStorage.begin());
    }
    // waitingForSN is never reduced
    this->waitingForSN = minSN > this->waitingForSN ? minSN : this->waitingForSN;
    // shift of waitingForSN might free already received frames
    this->purgeRxStorage();

    // do not acknowledge old frames
    while((not rxSNs.empty()) and (*(rxSNs.begin()) < minSN))
    {
        rxSNs.erase(rxSNs.begin());
    }

    // create BlockACK
    this->blockACK = parent->friends.manager->createCompound(parent->friends.manager->getMACAddress(),
                                                             adr,
                                                             ACK,
                                                             parent->friends.manager->getFrameExchangeDuration(compound->getCommandPool())
                                                             - parent->sifsDuration - parent->expectedACKDuration,
                                                             false);
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
