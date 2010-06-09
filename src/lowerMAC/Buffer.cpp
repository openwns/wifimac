/*******************************************************************************
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 5, D-52074 Aachen, Germany
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

#include <WIFIMAC/lowerMAC/Buffer.hpp>

using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    Buffer,
    wns::ldk::buffer::Buffer,
    "wifimac.lowerMAC.Buffer",
    wns::ldk::FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    Buffer,
    wns::ldk::FunctionalUnit,
    "wifimac.lowerMAC.Buffer",
    wns::ldk::FUNConfigCreator);

Buffer::Buffer(wns::ldk::fun::FUN* fuNet, const wns::pyconfig::View& config) :
    wns::ldk::buffer::Buffer(fuNet, config),

    wns::ldk::fu::Plain<Buffer>(fuNet),
    wns::ldk::Delayed<Buffer>(),
    raName(config.get<std::string>("raName")),
    managerName(config.get<std::string>("managerName")),
    protocolCalculatorName(config.get<std::string>("protocolCalculatorName")),
    buffer(wns::ldk::buffer::dropping::ContainerType()),
    maxSize(config.get<int>("size")),
    currentSize(0),
    sizeCalculator(),
    dropper(),
    totalPDUs(),
    droppedPDUs(),
    logger("WNS", config.get<std::string>("name"))
{
    {
        std::string pluginName = config.get<std::string>("sizeUnit");
        sizeCalculator = std::auto_ptr<wns::ldk::buffer::SizeCalculator>(wns::ldk::buffer::SizeCalculator::Factory::creator(pluginName)->create());
    }
    friends.ra = NULL;
    friends.manager = NULL;
    protocolCalculator = NULL;
    {
        std::string pluginName = config.get<std::string>("drop");
        dropper = std::auto_ptr<wns::ldk::buffer::dropping::Drop>(wns::ldk::buffer::dropping::Drop::Factory::creator(pluginName)->create());
    }
} // Buffer

Buffer::Buffer(const Buffer& other) :
    //wns::ldk::CompoundHandlerInterface<Buffer>(other),
	wns::ldk::CommandTypeSpecifierInterface(other),
	wns::ldk::HasReceptorInterface(other),
	wns::ldk::HasConnectorInterface(other),
	wns::ldk::HasDelivererInterface(other),
	wns::CloneableInterface(other),
    wns::IOutputStreamable(other),
    wns::PythonicOutput(other),
	wns::ldk::FunctionalUnit(other),
	wns::ldk::DelayedInterface(other),
    wns::ldk::buffer::Buffer(other),
	wns::ldk::fu::Plain<Buffer>(other),
	wns::ldk::Delayed<Buffer>(other),
	buffer(other.buffer),
	maxSize(other.maxSize),
	currentSize(other.currentSize),
	sizeCalculator(wns::clone(other.sizeCalculator)),
	dropper(wns::clone(other.dropper)),
	totalPDUs(other.totalPDUs),
	droppedPDUs(other.droppedPDUs),
	logger(other.logger)
{
	friends.ra = other.friends.ra;
    friends.manager = other.friends.manager;
	protocolCalculator = other.protocolCalculator;
}

void Buffer::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");
    friends.ra = getFUN()->findFriend<wifimac::lowerMAC::RateAdaptation*>(raName);
    friends.manager = getFUN()->findFriend<wifimac::lowerMAC::Manager*>(managerName);
    protocolCalculator = getFUN()->getLayer<dll::ILayer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
}


Buffer::~Buffer()
{
} // ~Buffer


//
// Delayed interface
//
void
Buffer::processIncoming(const wns::ldk::CompoundPtr& compound)
{
	getDeliverer()->getAcceptor(compound)->onData(compound);
} // processIncoming


bool
Buffer::hasCapacity() const
{
	return true;
} // hasCapacity


void
Buffer::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
    checkLifetime();

	buffer.push_back(compound);
	currentSize += (*sizeCalculator)(compound);

	while(currentSize > maxSize)
    {
        MESSAGE_BEGIN(NORMAL, logger, m, getFUN()->getName());
        m << " dropping a PDU! maxSize reached : " << maxSize;
        m << " current size is " << currentSize;
        MESSAGE_END();

        wns::ldk::CompoundPtr toDrop = (*dropper)(buffer);
        int pduSize = (*sizeCalculator)(toDrop);
        currentSize -= pduSize;
        increaseDroppedPDUs(pduSize);
    }
    MESSAGE_SINGLE(NORMAL, this->logger, "Store outgoing compound, size is now " << currentSize);
    increaseTotalPDUs();
    probe();
} // processOutgoing


const wns::ldk::CompoundPtr
Buffer::hasSomethingToSend() const
{
    if(buffer.empty())
    {
        return wns::ldk::CompoundPtr();
    }
    return buffer.front();
} // somethingToSend


wns::ldk::CompoundPtr
Buffer::getSomethingToSend()
{
	wns::ldk::CompoundPtr compound = buffer.front();
	buffer.pop_front();

	currentSize -= (*sizeCalculator)(compound);
	probe();

    checkLifetime();


    MESSAGE_SINGLE(NORMAL, this->logger, "Send compound, size is now " << currentSize);
	return compound;
} // getSomethingToSend


void
Buffer::checkLifetime()
{
    for (wns::ldk::buffer::dropping::ContainerType::iterator it = buffer.begin();
         it != buffer.end();
        )
    {
        wns::ldk::buffer::dropping::ContainerType::iterator next = it;
        ++next;

        if(friends.manager->lifetimeExpired((*it)->getCommandPool()))
        {
            int pduSize = (*sizeCalculator)(*it);
            currentSize -= pduSize;
            increaseDroppedPDUs(pduSize);
            probe();

            buffer.erase(it);

            MESSAGE_BEGIN(NORMAL, logger, m, getFUN()->getName());
            m << "PDU in queue has reached lifetime -> drop!";
            m << "New size is " << currentSize;
            MESSAGE_END();
        }
        it = next;
    }
}

//
// Buffer interface
//

unsigned long int
Buffer::getSize()
{
	return currentSize;
} // getSize


unsigned long int
Buffer::getMaxSize()
{
	return maxSize;
} // getMaxSize


wns::simulator::Time 
Buffer::getNextTransmissionDuration()
{
    if (buffer.empty())
    {
        return 0;
    }
    wifimac::convergence::PhyMode phyMode = friends.ra->getPhyMode(buffer.front());
    return(protocolCalculator->getDuration()->MPDU_PPDU(buffer.front()->getLengthInBits(),
                                                       phyMode));
}

wns::service::dll::UnicastAddress
Buffer::getNextReceiver() const
{
    if (buffer.empty())
    {
        return wns::service::dll::UnicastAddress();
    }
    return friends.manager->getReceiverAddress(buffer.front()->getCommandPool());
}
