/******************************************************************************
 * WiFiMac                                                                    *
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2009
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

#include <WIFIMAC/lowerMAC/MultiBuffer.hpp>
#include <WIFIMAC/pathselection/ForwardingCommand.hpp>
#include <WNS/ldk/CommandReaderInterface.hpp>
#include <DLL/UpperConvergence.hpp>

using namespace wifimac::lowerMAC;

//
// queue selection strategies
//

// return id (= MAC ID) for the next buffer to be sent or -1 
// if no buffer has reached the send limit, while the multi buffer still has storage capcity
int32_t ConstantRRSelector::getSendBuffer(const DestinationBuffers &buffers, bool strict) 
{

   ContainerType *buffer;   
   DestinationBuffers::const_iterator itr;
   DestinationBuffers::const_iterator travel;
   uint32_t bufferSendSize = multiBuffer->getBufferSendSize();
   int32_t newCurrentBuffer = -1;
   int32_t count = lastBuffer;

// Round Robin approach to check for the next send buffer, based on MAC Id and the last used buffer (if any)

   // get to the current buffer (if any, else we will start with the first buffer)
   for (itr = buffers.begin(); (itr != buffers.end()) and (count > 0); itr++)
   {
	count--;
   }

   // if there was a current buffer, iterate to successor
   if (lastBuffer >= 0)
   {
	if (itr != buffers.end())
        {
		itr++;
        }
	else
        {
		itr = buffers.begin();
        }
   }
   // search for buffer following the last used to find next buffer which size meets size criteria
   for (itr; itr != buffers.end(); itr++)
	if ((itr->second->size() >= bufferSendSize))
        {
        	newCurrentBuffer = itr->first;
                break;
        }

    // reached the last buffer without finding a potential buffer to be send next
    // check now buffers from first one to the last used one
    count = lastBuffer;
    if ((newCurrentBuffer == -1) and (lastBuffer >= 0))
    {
	   for (itr=buffers.begin(); (itr != buffers.end()) and (count >= 0); itr++) 
	   {
		count--;
		if ((itr->second->size() >= bufferSendSize))
	        {
	        	newCurrentBuffer = itr->first;
	                break;
	        }
	    }
   }
   if (newCurrentBuffer == -1) // no send buffer currently has met the size criteria
   {
	if (strict == false) // return buffer with most compounds
        {
		newCurrentBuffer = buffers.begin()->first;
               	for (itr = buffers.begin(); itr != buffers.end(); itr++)
                       	if (itr->second->size() > buffers.find(newCurrentBuffer)->size())
                                       newCurrentBuffer = itr->first;
		if (buffers.find(newCurrentBuffer)->size() == 0)
			newCurrentBuffer = -1;
        }
   }
   lastBuffer = newCurrentBuffer;
   return newCurrentBuffer;
}

// "register" multi buffer this strategy is attached to
void
ConstantRRSelector::setMultiBuffer(MultiBuffer *buffer,wns::logger::Logger *l) {
	multiBuffer = buffer;
	lastBuffer = -1;
	logger = l;
}

STATIC_FACTORY_REGISTER(ConstantRRSelector, Queuing, "ConstantRRSelector");


//
// FunctionalUnit implementation
//


STATIC_FACTORY_REGISTER_WITH_CREATOR(
	wifimac::lowerMAC::MultiBuffer,
	wns::ldk::FunctionalUnit,
	"wifimac.lowerMAC.MultiBuffer",
	wns::ldk::FUNConfigCreator);


MultiBuffer::MultiBuffer(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config) :
	    wns::ldk::fu::Plain<MultiBuffer, wns::ldk::EmptyCommand>(fun),
	    CanTimeout(),
            sizeCalculator(),
	    maxSize(config.get<int>("size")),
	    logger(config.get("logger")),
	    bufferSendSize(config.get<int>("myConfig.sendSize")),
	    incomingTimeout(config.get<float>("myConfig.timeout")),
	    impatient(config.get<bool>("myConfig.impatient")),
            stilltoBeSent(0),
	    currentSize(0),
	    currentBuffer(-1),
	    queueSelector()
{
	std::string pluginName = config.get<std::string>("sizeUnit");
	sizeCalculator = std::auto_ptr<wns::ldk::buffer::SizeCalculator>(wns::ldk::buffer::SizeCalculator::Factory::creator(pluginName)->create());
	pluginName = config.get<std::string>("myConfig.queueSelector");
        queueSelector = std::auto_ptr<Queuing>(Queuing::Factory::creator(pluginName)->create());
	queueSelector->setMultiBuffer(this, &(this->logger));
}


MultiBuffer::MultiBuffer(const MultiBuffer& other) :
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
        wns::ldk::fu::Plain<MultiBuffer>(other),
        wns::ldk::Delayed<MultiBuffer>(other),
        maxSize(other.maxSize),
	sendBuffers(other.sendBuffers),
	currentSize(other.currentSize),
	currentBuffer(other.currentBuffer),
	stilltoBeSent(other.stilltoBeSent),
	bufferSendSize(other.bufferSendSize),
	queueSelector(wns::clone(other.queueSelector)),
        sizeCalculator(wns::clone(other.sizeCalculator)),
	incomingTimeout(other.incomingTimeout)
{
}


void
MultiBuffer::processIncoming(const wns::ldk::CompoundPtr& compound)
{
	getDeliverer()->getAcceptor(compound)->onData(compound);
} // processIncoming


void
MultiBuffer::processOutgoing(const wns::ldk::CompoundPtr& compound)
{
	wns::ldk::CommandReaderInterface* commandReader;
	dll::UpperCommand* ucCommand;
	commandReader = getFUN()->getCommandReader("upperConvergence");
	ucCommand = commandReader->readCommand<dll::UpperCommand>(compound->getCommandPool());
	int32_t address = ucCommand->peer.targetMACAddress.getInteger();;
	uint32_t compoundSize = (*sizeCalculator)(compound);
	ContainerType *buffer;	
	DestinationBuffers::const_iterator itr;
	bool bufferFull = false;

	// aggregated sizes of all send buffers would exceed the overall limit
	if (currentSize + compoundSize > maxSize) 
	{
		bufferFull = true;
	}
	else
        {
		if (this->hasTimeoutSet() == true)
		{
			cancelTimeout();
		}
		currentSize += compoundSize;
		if (sendBuffers.knows(address))
		{
			buffer = sendBuffers.find(address);
		}
		else // first time for this receiver
		{
			buffer = new ContainerType();
			sendBuffers.insert(address,buffer);
		}
		buffer->push_back(compound);
		if (incomingTimeout > 0)
		{
			this->setTimeout(this->incomingTimeout);
		}
	}
	if (currentBuffer == -1)  // no send buffer met the "flush" criteria so far, maybe after we (tried to) add this new compound
	{
		currentBuffer = queueSelector->getSendBuffer(sendBuffers,(!bufferFull and !impatient));
		if (currentBuffer != -1)
		{
			if (sendBuffers.find(currentBuffer)->size() >= bufferSendSize)
			{
				stilltoBeSent = bufferSendSize;
			}
			else
			{
				stilltoBeSent = sendBuffers.find(currentBuffer)->size();	
			}
		}
	}
} // processOutgoing


const wns::ldk::CompoundPtr
MultiBuffer::hasSomethingToSend() const
{
   if (currentBuffer == -1) 
   {
	return (wns::ldk::CompoundPtr());
   }
   return(sendBuffers.find(currentBuffer)->front());
} // hasSomethingToSend


wns::ldk::CompoundPtr
MultiBuffer::getSomethingToSend()
{
   wns::ldk::CompoundPtr it;
   ContainerType *buffer;   

   assure(hasSomethingToSend(), "Called getSomethingToSend, but nothing to send");
   // remove the head of the current send buffer queue
   buffer = sendBuffers.find(currentBuffer);
   it = buffer->front();   
   currentSize -= (*sizeCalculator)(it);
   buffer->pop_front();  
   stilltoBeSent--;
   // if all packages for the current "run" have been sent, call strategy for the next buffer (id)
   if (stilltoBeSent <= 0) 
   {
	currentBuffer = queueSelector->getSendBuffer(sendBuffers,!impatient);
	if (currentBuffer != -1)
	{
                        if (sendBuffers.find(currentBuffer)->size() >= bufferSendSize)
			{
                                stilltoBeSent = bufferSendSize;
			}
                        else
			{
                                stilltoBeSent = sendBuffers.find(currentBuffer)->size();
			}
	}
   }
   return it;
} // getSomethingToSend

uint32_t
MultiBuffer::getBufferSendSize() const
{
  return bufferSendSize;
}

void
MultiBuffer::onTimeout() 
{
	if (currentBuffer == -1) // no buffer meets the strategy criteria
	{	// get the buffer with the most compounds, to be sent next
		currentBuffer = queueSelector->getSendBuffer(sendBuffers,false);
		if (currentBuffer != -1) 
		{
			stilltoBeSent = sendBuffers.find(currentBuffer)->size();
		}
		else // no waiting compounds at all no new timer setting needed
		{
			return;
		}
	}
	// restart countdown
	this->setTimeout(this->incomingTimeout);
}
