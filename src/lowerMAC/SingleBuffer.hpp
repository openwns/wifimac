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

#ifndef WIFIMAC_LOWERMAC_SINGLEBUFFER_HPP
#define WIFIMAC_LOWERMAC_SINGLEBUFFER_HPP

#include <WIFIMAC/lowerMAC/ITXOPWindow.hpp>
#include <WIFIMAC/lowerMAC/RateAdaptation.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>

#include <WNS/ldk/buffer/Dropping.hpp>

#include <list>
#include <memory>

namespace wifimac { namespace lowerMAC {

	/**
	 * @brief Discarding buffer of a fixed size.
	 *
	 * FIFO buffer, discarding some compounds, when the buffer is
	 * full.
	 * The maximum size is given as number of compounds to store.
	 *
	 */
	class SingleBuffer :
		public wns::ldk::buffer::Buffer,
		public wns::ldk::fu::Plain<SingleBuffer>,
		public wns::ldk::Delayed<SingleBuffer>,
		public wifimac::lowerMAC::ITXOPWindow
	{
		typedef uint32_t PDUCounter;

	public:

		/**
		 * @brief Constructor
		 */
		SingleBuffer(wns::ldk::fun::FUN* fuNet, const wns::pyconfig::View& config);

		/**
		 * @brief Copy Constructor
		 */
		SingleBuffer(const SingleBuffer& other);

		/**
		 * @brief Destructor
		 */
		virtual
		~SingleBuffer();

		//
		// Delayed interface
		//
		virtual void
		processIncoming(const wns::ldk::CompoundPtr& compound);

		virtual bool
		hasCapacity() const;

		virtual void
		processOutgoing(const wns::ldk::CompoundPtr& compound);

		virtual const wns::ldk::CompoundPtr
		hasSomethingToSend() const;

		virtual	wns::ldk::CompoundPtr
		getSomethingToSend();

		//
		// Buffer interface
		//
		virtual uint32_t
		getSize();

		virtual uint32_t
		getMaxSize();

		virtual void
		setDuration(wns::simulator::Time duration);

		virtual wns::simulator::Time 
		getActualDuration(wns::simulator::Time duration);

	protected:
		wns::ldk::buffer::dropping::ContainerType buffer;

	private:
		void onFUNCreated();
		wns::simulator::Time firstCompoundDuration() const;	
		uint32_t maxSize;
		uint32_t currentSize;
		bool isActive;
		std::auto_ptr<wns::ldk::buffer::SizeCalculator> sizeCalculator;
		std::auto_ptr<wns::ldk::buffer::dropping::Drop> dropper;

		PDUCounter totalPDUs;
		PDUCounter droppedPDUs;

	        const std::string raName;
	        const std::string protocolCalculatorName;
	        wifimac::management::ProtocolCalculator* protocolCalculator;

	        struct Friends
	        {
		    wifimac::lowerMAC::RateAdaptation* ra;
	            wifimac::lowerMAC::Manager* manager;
	        } friends;

		wns::logger::Logger logger;
		wns::simulator::Time maxDuration;
	};

}}


#endif // NOT defined WIFIMAC_LOWERMAC_SINGLEBUFFER_HPP


