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

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/lowerMAC/RateAdaptation.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>
#include <WIFIMAC/lowerMAC/ITXOPWindow.hpp>

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
    typedef wns::container::Registry<unsigned int, ContainerType*> DestinationBuffers;

    class MultiBuffer;

   /**
    * @brief Interface for send buffer selecting strategies.
    *
    */
   class IQueuing :
        public virtual wns::CloneableInterface
   {
   public:
       typedef wns::Creator<IQueuing> Creator;
       typedef wns::StaticFactory<Creator> Factory;


   	/**
   	* @brief returns ID of the next queue to be sent
   	*
   	* If strict, either this method returns the buffer that 
	* meets the strategy requirements or -1 otherwise
	* if non strict it returns the ID of the buffer with 
	* most data if no other send queue meets the send size criteria
   	* or -1 if all queues are empty
	*/
       virtual int
       getSendBuffer(const DestinationBuffers& buffers, bool strict) = 0;

   	/** @brief "register" MultiBuffer to work on
	*/
       virtual void
       setMultiBuffer(MultiBuffer* buffer, wns::logger::Logger* l) = 0;

       virtual
       ~IQueuing()
           {}
   };


   /**
    * @brief Round Robin queue selection strategy
    * 
    * this strategy selects the next send queue that has reached the send threshold
    * of the MultiBuffer for transmission
    */
    class ConstantRRSelector :
        public IQueuing,
        public wns::Cloneable<ConstantRRSelector>
    {
        virtual int
        getSendBuffer(const DestinationBuffers& buffers, bool strict);

        virtual void
        setMultiBuffer(MultiBuffer* buffer, wns::logger::Logger* l);

    protected:
        MultiBuffer* multiBuffer;
        int lastBuffer;
        wns::logger::Logger* logger;
    };


    /**
     * @brief Multi queue buffer
     *
     * incoming compounds are stored in seperate queues, depending on the receiver address
     * The buffer drops incoming compounds if adding them would exceed the overall buffer size (buffer is marked FULL then)
     * the queue selector strategy determines which queue will be transmitted next.
     * This class aims to improve the BlockACK / frame aggregation of .11n
     *  - after timeout timeframe without any new compounds to be queued, MultiBuffer
     *    selects the biggest send queue for next transmission, if none is selected currently
     *  - an impatient MultiBuffer always select a new send queue for next transmission
     *    (check for a queue that meets the transmission criteria first, if none, select the largest queue)
     * The MultiBuffer class also implements the TXOPTimeWindow interface in order to let the TXOP FU
     * determine the size/number of PDUs to be processed (= passed to lower FUs) to maximize usage of the
     * (remaining) TXOP duration. The buffer passes outgoing compounds only after a setDuration() call
     * and only for a sequence of following compounds which transmission duration together fits into the
     * time frame. After those compounds have been passed to the next FU, the buffer becomes inactive, until
     * a new "run" is started by calling setDuration() again.
     */

    class MultiBuffer :
        public wns::ldk::fu::Plain<MultiBuffer, wns::ldk::EmptyCommand>,
        public wns::ldk::Delayed<MultiBuffer>,
        public wns::events::CanTimeout,
	public wifimac::lowerMAC::ITXOPWindow
    {
    public:
        MultiBuffer(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& /*config*/);

        /// Copy constructor required if std::auto_ptr are used
        MultiBuffer(const MultiBuffer& other);

        virtual bool
        hasCapacity() const
            { return true;}

        virtual void
        processIncoming(const wns::ldk::CompoundPtr& compound);

	/** @brief indicates outgoing compound
	*
	* outgoing PDUs are stored depending on the receiver in the apropriate
	* queue, until the total buffer size is exceeded
	*/
        virtual void
        processOutgoing(const wns::ldk::CompoundPtr&);

	/** @brief indicates outgoing compound
	*
	* only if a non-empty queue is selected and 
	* the buffer is active due to a call of setDuration()
	* and for this "run" there are compounds left that fit into
	* the time window this method returns a non empty pointer
	*/
        virtual const wns::ldk::CompoundPtr
        hasSomethingToSend() const;

	/** @brief returns outgoing compounds
	*
	* if outgoing compounds are waiting, this methods returns the next
	* one of the currently selected queue. After all compounds which
	* fit into the time window are retrieved, this method deactivates 
	* the buffer
	*/
        virtual wns::ldk::CompoundPtr
        getSomethingToSend();

	/** @brief returns threshold for a queue to be sent
	*
	* this method is called by the queue selection strategy to determine
	* which queues have enough compounds to be sent
	*/
        unsigned int
        getBufferSendSize() const;

	/** @brief returns the size of the passed queue
	*
	* the size of the passed queue is returned, depending on the buffer's 
	* size unit (e.g. bits or number of PDUs)
	*/
        unsigned int
        getSize(const ContainerType& buffer) const;

        /** @brief timeout for small traffic
	*
	* the MultiBuffer can be set to wait a certain amount of time after
        * the arrival of the last outgoing PDU (= inactivity due to small outgoing
	* traffic). On timeout, if no queue is currently selected,
        * the selection strategy is called to return a queue that either fits the 
	* selection criteria or has the most data waiting
	*/
        virtual void
        onTimeout();

	/** @brief sets the time window for outgoing PDUs
	*
	* when called (by the TXOP FU), this method calculates
	* the amount of outgoing data of the currently selected queue
	* that fits into the time window and activates the MultiBuffer
	*/ 
	virtual void
	setDuration(wns::simulator::Time duration);

	/** @brief returns the actually used transmission time
	*
	* this method returns for the passed duration the actual
	* amount of time it takes to transmit PDUs fitting into
	* that time window
	*/ 
	virtual wns::simulator::Time 
	getActualDuration(wns::simulator::Time duration);

    private:
	void onFUNCreated();

	/** @brief sets internal parameters for outgoing PDUs
	*
	* this method is called internally to calculate the size of data
	* (number of PDUs or bits) to be sent in the next "run" started by
	* setDuration() as well as the transmission time for those data
	* that fits into the set time window
	*/
	void calculateSendParameters();

        const std::string managerName;
        const std::string raName;
        const std::string protocolCalculatorName;
        wifimac::management::ProtocolCalculator* protocolCalculator;

        struct Friends
        {
	    wifimac::lowerMAC::RateAdaptation* ra;
            wifimac::lowerMAC::Manager* manager;
        } friends;

        std::auto_ptr<wns::ldk::buffer::SizeCalculator> sizeCalculator;
	// indicates new round for TXOP
	bool isActive;
        DestinationBuffers sendBuffers;
        wns::logger::Logger logger;
        const int bufferSendSize;
        const int maxSize;
        int currentSize;
        int currentBuffer;
        int stilltoBeSent;
        const bool impatient;
        std::auto_ptr<IQueuing> queueSelector;
        const wns::simulator::Time incomingTimeout;
	wns::simulator::Time maxDuration;
	wns::simulator::Time actualDuration;
    };


} // lowerMAC
} // wifimac

#endif // WIFIMAC__LOWERMAC_MULTIBUFFER_HPP
