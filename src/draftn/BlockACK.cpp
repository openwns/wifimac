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
        assure(currentReceiver != wns::service::dll::UnicastAddress(), "got ACK though nothing has been transmitted");
        assure(txQueue != NULL, "got ACK though no transmission queue");
        assure(transmitter == currentReceiver, "got ACK from wrong Station");
        assure(this->baState == receptionFinished, "Received ACK but not waiting for one");


        perMIB->onSuccessfullTransmission(currentReceiver);
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
    assure(txQueue != NULL, "Timeout, but no transmission queue");
    assure(txQueue->waitsForACK(), "Timeout, but txQueue is not waiting for ACK");

    MESSAGE_SINGLE(NORMAL, this->logger, "Timeout -> failed transmission to " << currentReceiver);
    perMIB->onFailedTransmission(currentReceiver);

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

