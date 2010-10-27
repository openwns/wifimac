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
#include <WIFIMAC/draftn/ReceptionQueue.hpp>

using namespace wifimac::draftn;

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

const unsigned int
ReceptionQueue::storageSize() const
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

void
ReceptionQueue::processIncomingData(const wns::ldk::CompoundPtr& compound, const unsigned int size)
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

void
ReceptionQueue::purgeRxStorage()
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

void
ReceptionQueue::processIncomingACKreq(const wns::ldk::CompoundPtr& compound)
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

const wns::ldk::CompoundPtr
ReceptionQueue::hasACK() const
{
    return(this->blockACK);
} // ReceptionQueue::hasACK

wns::ldk::CompoundPtr
ReceptionQueue::getACK()
{
    assure(hasACK(), "Called getACK although no ack prepared?");
    assure(this->blockACK != wns::ldk::CompoundPtr(), "Called getACK but blockACK is empty?");

    wns::ldk::CompoundPtr it = this->blockACK;
    this->blockACK = wns::ldk::CompoundPtr();

    MESSAGE_SINGLE(NORMAL, parent->logger, "RxQ" << adr << ": Transmit BA");

    return(it);
} // ReceptionQueue::getACK
