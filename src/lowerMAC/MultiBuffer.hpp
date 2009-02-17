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

#ifndef WIFIMAC_LOWERMAC_MULTIBUFFER_HPP
#define WIFIMAC_LOWERMAC_MULTIBUFFER_HPP

#include <WNS/ldk/FunctionalUnit.hpp>
#include <WNS/ldk/FUNConfigCreator.hpp>
#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/buffer/Buffer.hpp>
#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Command.hpp>
#include <WNS/service/dll/Address.hpp>
#include <WNS/events/CanTimeout.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/StaticFactory.hpp>

#include <list>

namespace wifimac { namespace lowerMAC {

	typedef std::list<wns::ldk::CompoundPtr> ContainerType;
	typedef wns::container::Registry<uint32_t, ContainerType *> DestinationBuffers;

	/**
        * @brief Interface for send buffer selecting strategies.
        * 
        */
	class MultiBuffer;

	class Queuing :
		public virtual wns::CloneableInterface
	{
	public:
		typedef wns::Creator<Queuing> Creator;
                typedef wns::StaticFactory<Creator> Factory;

		// return ID of next buffer to be sent. If strict, either returns buffer that meets strategy requirements, else -1
		// non strict: returns buffer that meets selection criteria, if none, returns buffer with most compounds or -1 if all empty
                virtual int32_t
                getSendBuffer(const DestinationBuffers &buffers, bool strict) = 0;

		virtual void
		setMultiBuffer(MultiBuffer *buffer, wns::logger::Logger *l) = 0;

                virtual
                ~Queuing()
                {}
        };


	class ConstantRRSelector :
		public Queuing,
		public wns::Cloneable<ConstantRRSelector>
	{
		virtual int32_t
                getSendBuffer(const DestinationBuffers &buffers, bool strict);
		
		virtual void
		setMultiBuffer(MultiBuffer *buffer, wns::logger::Logger *l);

	protected:		
		MultiBuffer *multiBuffer;
		int32_t lastBuffer;
		wns::logger::Logger *logger;
	};

	/**
        * @brief Interface for the multi queue buffer
        * incoming compounds are stored in seperate queues, depending on the receiver address
	* The buffer drops incoming compounds if adding them would exceed the overall buffer size (buffer is marked FULL then)
	* the queue selector strategy determines which queue will be transmitted next.
	* This class aims to improve the BlockACK / frame aggregation of .11n
	*  - after timeout timeframe without any new compounds to be queued, MultiBuffer
        *    selects the biggest send queue for next transmission, if none is selected currently
	*  - an impatient MultiBuffer always select a new send queue for next transmission
	*    (check for a queue that meets the transmission criteria first, if none, select the largest queue)
        */

	class MultiBuffer :
		public wns::ldk::fu::Plain<MultiBuffer, wns::ldk::EmptyCommand>,
        	public wns::ldk::Delayed<MultiBuffer>,
		public wns::events::CanTimeout
	{
	public:
		MultiBuffer(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& /*config*/);
		MultiBuffer(const MultiBuffer& other);


        // Delayed interface
		virtual bool hasCapacity() const { return true;}
		virtual void processIncoming(const wns::ldk::CompoundPtr& compound);
		virtual void processOutgoing(const wns::ldk::CompoundPtr&);
		virtual const wns::ldk::CompoundPtr hasSomethingToSend() const;
		virtual wns::ldk::CompoundPtr getSomethingToSend();
		uint32_t getBufferSendSize() const;

	// CanTimeout interface
		virtual void onTimeout();

    private:
	std::auto_ptr<wns::ldk::buffer::SizeCalculator> sizeCalculator;
	DestinationBuffers sendBuffers;
	wns::logger::Logger logger;
	int32_t bufferSendSize;
	int32_t maxSize;
	int32_t currentSize;
	int32_t currentBuffer;
	int32_t stilltoBeSent;
	bool impatient;
	uint32_t getCurrentSendBuffer();
	std::auto_ptr<Queuing> queueSelector;
	double incomingTimeout;
    };


} // lowerMAC
} // wifimac

#endif // WIFIMAC__LOWERMAC_MULTIBUFFER_HPP
