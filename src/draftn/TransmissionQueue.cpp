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

#include <WIFIMAC/draftn/TransmissionQueue.hpp>
#include <WIFIMAC/draftn/BlockACK.hpp>

using namespace wifimac::draftn;

TransmissionQueue::TransmissionQueue(BlockACK* parent_,
                                     size_t maxOnAir_,
                                     wns::simulator::Time maxDelay_,
                                     wns::service::dll::UnicastAddress adr_,
                                     BlockACKCommand::SequenceNumber sn_,
                                     wifimac::management::PERInformationBase* perMIB_,
                                     std::auto_ptr<wns::ldk::buffer::SizeCalculator> *sizeCalculator_) :
    parent(parent_),
    maxOnAir(maxOnAir_),
    maxDelay(maxDelay_),
    adr(adr_),
    txQueue(),
    onAirQueue(),
    nextSN(sn_),
    baREQ(),
    waitForACK(false),
    baReqRequired(false),
    perMIB(perMIB_),
    sizeCalculator(sizeCalculator_),
    oldestTimestamp()
{
    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << " created");

    // prepare Block ACK request
    this->baREQ = parent->friends.manager->createCompound(parent->friends.manager->getMACAddress(),
                                                          adr,
                                                          DATA,
                                                          parent->sifsDuration + parent->maximumACKDuration,
                                                          parent->ackTimeout);
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


const unsigned int
TransmissionQueue::onAirQueueSize() const
{
    unsigned int size = 0;
    for(std::deque<CompoundPtrWithTime>::const_iterator it = onAirQueue.begin();
        it != onAirQueue.end();
        it++)
    {
        size += (*(*sizeCalculator))(it->first);
    }
    return(size);
}

const unsigned int
TransmissionQueue::txQueueSize() const
{
    unsigned int size = 0;
    for(std::deque<CompoundPtrWithTime>::const_iterator it = txQueue.begin();
        it != txQueue.end();
        it++)
    {
       size += (*(*sizeCalculator))(it->first);
    }
    return(size);
}


const unsigned int
TransmissionQueue::storageSize() const
{
    return(this->onAirQueueSize() + this->txQueueSize());
}

void
TransmissionQueue::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    BlockACKCommand* baCommand = parent->activateCommand(compound->getCommandPool());
    baCommand->peer.type = I;
    baCommand->peer.sn = this->nextSN++;
    baCommand->localTransmissionCounter = 1;

    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Queue outgoing compound with sn " << baCommand->peer.sn);
    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": send time for compound is " << wns::simulator::getEventScheduler()->getTime()+this->maxDelay << " maxDelay is " << this->maxDelay << " oldest is " << oldestTimestamp);
    txQueue.push_back(CompoundPtrWithTime(compound, wns::simulator::getEventScheduler()->getTime()));

    if ((oldestTimestamp == wns::simulator::Time()) or
        (wns::simulator::getEventScheduler()->getTime() < oldestTimestamp))
    {
        oldestTimestamp = wns::simulator::getEventScheduler()->getTime();
    }
} // TransmissionQueue::processOutgoing

const
wns::ldk::CompoundPtr TransmissionQueue::hasData() const
{
    if(waitForACK)
    {
        MESSAGE_SINGLE(VERBOSE, parent->logger, "TxQ" << adr << ": hasData: Waiting for ACK -> no other transmissions");
        // no other transmissions during waiting for ACK
        return(wns::ldk::CompoundPtr());
    }
    if((not txQueue.empty()) and (onAirQueueSize()+(*(*sizeCalculator))(txQueue.front().first) <= this->maxOnAir))
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
           (onAirQueueSize()+(*(*sizeCalculator))(txQueue.front().first) > this->maxOnAir))
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

wns::ldk::CompoundPtr
TransmissionQueue::getData()
{
    assure(this->hasData(), "Called getData although hasData is false");

    // compound pending?
    if((not txQueue.empty()) and
       (onAirQueueSize()+(*(*sizeCalculator))(txQueue.front().first) <= this->maxOnAir))
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
        oldestTimestamp = wns::simulator::Time();
    }
    else
    {
        MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Reached tx window, send BAreq with start-sn " << parent->getCommand(onAirQueue.front().first)->peer.sn);
        oldestTimestamp = txQueue.front().second;
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

void
TransmissionQueue::processIncomingACK(std::set<BlockACKCommand::SequenceNumber> ackSNs)
{
    assure(not onAirQueue.empty(), "Received ACK but onAirQueue is empy");
    assure(this->waitForACK, "Received ACK but not waiting for one");
    this->waitForACK = false;
    MESSAGE_SINGLE(NORMAL, parent->logger, "TxQ" << adr << ": Received ACK, iterate through compounds on air");

    bool insertBack = false;
    bool blockACKsuccess = true;
    std::deque<CompoundPtrWithTime>::iterator txQueueFirst;
    if(txQueue.empty())
    {
        insertBack = true;
    }
    else
    {
        txQueueFirst = txQueue.begin();
    }

    std::set<wifimac::draftn::BlockACKCommand::SequenceNumber>::iterator snIt = ackSNs.begin();
    assure(isSortedBySN(onAirQueue),
           "onAirQueue is not sorted by SN!");

    for(std::deque<CompoundPtrWithTime>::iterator onAirIt = onAirQueue.begin();
        onAirIt != onAirQueue.end();
        onAirIt++)
    {
        wifimac::draftn::BlockACKCommand::SequenceNumber onAirSN = parent->getCommand((onAirIt->first)->getCommandPool())->peer.sn;

        if((snIt == ackSNs.end()) or (*snIt != onAirSN))
        {
             // retransmission
            int txCounter = ++(parent->getCommand((onAirIt->first)->getCommandPool())->localTransmissionCounter);
            //perMIB->onFailedTransmission(adr);
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
                    txQueueFirst = txQueue.insert(txQueueFirst, *onAirIt);
                    txQueueFirst++;
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

            //perMIB->onSuccessfullTransmission(adr);
            parent->numTxAttemptsProbe->put(onAirIt->first, parent->getCommand((onAirIt->first)->getCommandPool())->localTransmissionCounter);
        } // SN matches
    } // for-loop over onAirQueue

    // nothing is onAir now
    for (int i=0; i < parent->observers.size(); i++)
    {
        parent->observers[i]->onBlockACKReception(blockACKsuccess);
    }
    onAirQueue.clear();

   // find oldest timestamp
   oldestTimestamp = wns::simulator::Time();
   if (txQueueSize() > 0)
   {
       oldestTimestamp = txQueue.begin()->second;
       for (std::deque<CompoundPtrWithTime>::iterator itr = txQueue.begin(); itr != txQueue.end(); itr++)
       {
           if (itr->second < oldestTimestamp)
           {
               oldestTimestamp = itr->second;
           }
       }
   }
} // TransmissionQueue::processACK

bool
TransmissionQueue::isSortedBySN(const std::deque<CompoundPtrWithTime> q) const
{
    if(q.empty())
    {
        return true;
    }
    std::deque<CompoundPtrWithTime>::const_iterator it = q.begin();
    wifimac::draftn::BlockACKCommand::SequenceNumber lastSN = parent->getCommand((it->first)->getCommandPool())->peer.sn;
    ++it;
    MESSAGE_SINGLE(NORMAL,parent->logger,"SN: (+)" << lastSN);

    for(;
        it != q.end();
        it++)
    {
        wifimac::draftn::BlockACKCommand::SequenceNumber curSN = parent->getCommand((it->first)->getCommandPool())->peer.sn;
        MESSAGE_SINGLE(NORMAL,parent->logger,"SN: " << curSN);
        if(curSN < lastSN)
        {
            return false;
        }
        lastSN = curSN;
    }
    return true;
}

const bool
TransmissionQueue::waitsForACK() const
{
    return(this->waitForACK);
}

wns::ldk::CompoundPtr 
TransmissionQueue::getTxFront()
{
	if (txQueue.empty())
	{
		return wns::ldk::CompoundPtr();
	}
	return txQueue.front().first;
}


std::list<wns::ldk::CompoundPtr> 
TransmissionQueue::getAirableCompounds() 
{
    std::list<wns::ldk::CompoundPtr> ret;
    size_t retSize = 0;
    for (std::deque<CompoundPtrWithTime>::iterator itr = txQueue.begin(); itr != txQueue.end(); itr++)
    {
        if (retSize + (*(*sizeCalculator))(itr->first) > maxOnAir)
        {
            break;
        }
        ret.push_back(itr->first);
        retSize += (*(*sizeCalculator))(itr->first);
    }
    return ret;
}
