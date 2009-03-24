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
#include <WIFIMAC/lowerMAC/Manager.hpp>
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
	 * The SingleBuffer class also implements the TXOPTimeWindow interface in order to let the TXOP FU
	 * determine wether a waiting compound passes to the next lower FU (if its transmission fits into
	 * the (remaining) TXOP duration or it has to wait till a new TXOP "run" starts.
	 * The buffer gets activated by calling setDuration() which also sets the time frame in which PDU
	 * transmissions have to fit
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

		/** @brief enqueues outgoing PDUs from above FUs
		* outgoing compounds that are passed to the buffer from above
		* FUs are stored in a FIFO fashion, as long as the overall sizs of
		* the buffer isn't exceeded. Otherwise the compound is dropped
		*/
		virtual void
		processOutgoing(const wns::ldk::CompoundPtr& compound);

		/** @brief indicates a waiting PDU
		*
		* if the buffer is not empty, the transmission duration of
		* the next compound fits into the set time frame and the buffer
		* is active, this method returns a pointer to that compound
		*/
		virtual const wns::ldk::CompoundPtr
		hasSomethingToSend() const;

		/** @brief returns the next waiting PDU
		*
		* returns and removes the next compound from the buffer
		* (see  hasSomethingToSend())
		*/
		virtual	wns::ldk::CompoundPtr
		getSomethingToSend();

		//
		// Buffer interface
		//
		/** @brief returns the currently used buffer size
		*/
		virtual uint32_t
		getSize();

		/** @brief returns the maximum size of the buffer
		*/
		virtual uint32_t
		getMaxSize();

		/** @brief sets the time window for outgoing compounds
		*
		* this method limits the transmission time for the next
		* outgoing compound. If the compound doesn't fit into it
		* it is not passed to the next FU. This method also activates
		* the buffer 
		*/
		virtual void
		setDuration(wns::simulator::Time duration);

		/** @brief returns the actually used transmission time
		*
		* assing a time window this method returns the transmission
		* time of the next waiting PDU, if it fits into the window
		*/
		virtual wns::simulator::Time 
		getActualDuration(wns::simulator::Time duration);

	protected:
		wns::ldk::buffer::dropping::ContainerType buffer;

	private:
		void onFUNCreated();
		wns::simulator::Time firstCompoundDuration() const;	
        void checkLifetime();

		uint32_t maxSize;
		uint32_t currentSize;
		bool isActive;
		std::auto_ptr<wns::ldk::buffer::SizeCalculator> sizeCalculator;
		std::auto_ptr<wns::ldk::buffer::dropping::Drop> dropper;

		PDUCounter totalPDUs;
		PDUCounter droppedPDUs;

        const std::string raName;
        const std::string protocolCalculatorName;
        const std::string managerName;
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


