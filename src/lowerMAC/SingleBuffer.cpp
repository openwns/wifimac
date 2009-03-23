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

#include <WIFIMAC/lowerMAC/SingleBuffer.hpp>

using namespace wns::ldk;
using namespace wns::ldk::buffer;
using namespace wns::ldk::buffer::dropping;
using namespace wifimac::lowerMAC;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	SingleBuffer,
	Buffer,
	"wifimac.lowerMAC.SingleBuffer",
	FUNConfigCreator);

STATIC_FACTORY_REGISTER_WITH_CREATOR(
	SingleBuffer,
	FunctionalUnit,
	"wifimac.lowerMAC.SingleBuffer",
	FUNConfigCreator);

SingleBuffer::SingleBuffer(fun::FUN* fuNet, const wns::pyconfig::View& config) :
		Buffer(fuNet, config),

		fu::Plain<SingleBuffer>(fuNet),
		Delayed<SingleBuffer>(),
	        raName(config.get<std::string>("raName")),
    		protocolCalculatorName(config.get<std::string>("protocolCalculatorName")),
		buffer(ContainerType()),
		maxSize(config.get<int>("size")),
		currentSize(0),
		sizeCalculator(),
		dropper(),
		totalPDUs(),
		droppedPDUs(),
		maxDuration(0),
		isActive(false),
		logger("WNS", config.get<std::string>("name"))
{
	{
		std::string pluginName = config.get<std::string>("sizeUnit");
		sizeCalculator = std::auto_ptr<SizeCalculator>(SizeCalculator::Factory::creator(pluginName)->create());
		friends.ra = NULL;
		protocolCalculator = NULL;
	}

	{
		std::string pluginName = config.get<std::string>("drop");
		dropper = std::auto_ptr<Drop>(Drop::Factory::creator(pluginName)->create());
	}
} // SingleBuffer

SingleBuffer::SingleBuffer(const SingleBuffer& other) :
	CompoundHandlerInterface(other),
	CommandTypeSpecifierInterface(other),
	HasReceptorInterface(other),
	HasConnectorInterface(other),
	HasDelivererInterface(other),
	CloneableInterface(other),
	IOutputStreamable(other),
	PythonicOutput(other),
	FunctionalUnit(other),
	DelayedInterface(other),
	Buffer(other),
	fu::Plain<SingleBuffer>(other),
	Delayed<SingleBuffer>(other),
	buffer(other.buffer),
	maxSize(other.maxSize),
	currentSize(other.currentSize),
	sizeCalculator(wns::clone(other.sizeCalculator)),
	dropper(wns::clone(other.dropper)),
	totalPDUs(other.totalPDUs),
	droppedPDUs(other.droppedPDUs),
	maxDuration(other.maxDuration),
	logger(other.logger)
{
	friends.ra = other.friends.ra;
	protocolCalculator = other.protocolCalculator;
}

void SingleBuffer::onFUNCreated()
{
    MESSAGE_SINGLE(NORMAL, this->logger, "onFUNCreated() started");
    friends.ra = getFUN()->findFriend<wifimac::lowerMAC::RateAdaptation*>(raName);
    protocolCalculator = getFUN()->getLayer<dll::Layer2*>()->getManagementService<wifimac::management::ProtocolCalculator>(protocolCalculatorName);
}


SingleBuffer::~SingleBuffer()
{
} // ~SingleBuffer


//
// Delayed interface
//
void
SingleBuffer::processIncoming(const CompoundPtr& compound)
{
	getDeliverer()->getAcceptor(compound)->onData(compound);
} // processIncoming


bool
SingleBuffer::hasCapacity() const
{
	return true;
} // hasCapacity


void
SingleBuffer::processOutgoing(const CompoundPtr& compound)
{
	buffer.push_back(compound);
	currentSize += (*sizeCalculator)(compound);

	while(currentSize > maxSize) {

		MESSAGE_BEGIN(NORMAL, logger, m, getFUN()->getName());
		m << " dropping a PDU! maxSize reached : " << maxSize;
		m << " current size is " << currentSize;
		MESSAGE_END();

		CompoundPtr toDrop = (*dropper)(buffer);
		int pduSize = (*sizeCalculator)(toDrop);
		currentSize -= pduSize;
		increaseDroppedPDUs(pduSize);
	}

	increaseTotalPDUs();
	probe();
} // processOutgoing


const CompoundPtr
SingleBuffer::hasSomethingToSend() const
{
	if(buffer.empty() or (isActive == false))
	{
		return CompoundPtr();
	}
	if ((maxDuration > 0) and (firstCompoundDuration() > maxDuration))
	{
		return CompoundPtr();
	}
	
	return buffer.front();
} // somethingToSend


CompoundPtr
SingleBuffer::getSomethingToSend()
{
	CompoundPtr compound = buffer.front();
	buffer.pop_front();

	currentSize -= (*sizeCalculator)(compound);
	probe();
	isActive = false;
	return compound;
} // getSomethingToSend


//
// Buffer interface
//

uint32_t
SingleBuffer::getSize()
{
	return currentSize;
} // getSize


uint32_t
SingleBuffer::getMaxSize()
{
	return maxSize;
} // getMaxSize


void SingleBuffer::setDuration(wns::simulator::Time duration) 
{
	maxDuration = duration;
	isActive = true;
}

wns::simulator::Time SingleBuffer::getActualDuration(wns::simulator::Time duration) 
{
	if (firstCompoundDuration() > duration)
	{
		return 0;
	}
	return firstCompoundDuration();
}


wns::simulator::Time SingleBuffer::firstCompoundDuration() const
{
	if (buffer.empty())
	{
		return 0;
	}
	wifimac::convergence::PhyMode phyMode = friends.ra->getPhyMode(buffer.front());
	return(protocolCalculator->getDuration()->getMPDU_PPDU(buffer.front()->getLengthInBits(),phyMode.getDataBitsPerSymbol(), phyMode.getNumberOfSpatialStreams(), 20, std::string("Basic")));
}
