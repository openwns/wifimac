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

#ifndef WIFIMAC_LOWERMAC_NEXTFRAMEGETTER_HPP
#define WIFIMAC_LOWERMAC_NEXTFRAMEGETTER_HPP

#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Command.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>

namespace wifimac { namespace lowerMAC {

    /**
	 * @brief NextFrameGetter enables peek-ahead of the next compound
	 *
     * On first sight, NextFrameGetter is a simple store-and-forward FU, without
     * any functionality that alters the compound itself. The only difference to
     * e.g. the Synchronizer in the ldk::tools (which is exactly a simple
     * store-and-forward FU) is the implementation of the tryToSend() routine:
     * Instead of first sending, then wakeup to upper FU, it works in the
     * following way:
     *  1. Check is lower FU is accepting
     *  2. If yes, store the compound temporarly
     *  3. Wakeup the upper FU (and loose control of the process)
     *  4. Transmit compound
     * Recursion by multiple doSendData is disabled by the use of a "inWakeup"
     * variable.
     */
    class NextFrameGetter:
        public wns::ldk::fu::Plain<NextFrameGetter, wns::ldk::EmptyCommand>,
        public wns::ldk::DelayedInterface
    {
    public:

        NextFrameGetter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& /*config*/);

        // Delayed interface
        virtual void processIncoming(const wns::ldk::CompoundPtr& compound);
        virtual void processOutgoing(const wns::ldk::CompoundPtr&);
        virtual bool hasCapacity() const;
        virtual const wns::ldk::CompoundPtr hasSomethingToSend() const;
        virtual wns::ldk::CompoundPtr getSomethingToSend();

	virtual Bit getNextSize() const;

        // compoundHandlerInterface
        void
        doSendData(const wns::ldk::CompoundPtr& compound)
            {
                processOutgoing(compound);
                tryToSend();
            };// doSendData
        void
        doOnData(const wns::ldk::CompoundPtr& compound)
            {
                processIncoming(compound);
                tryToSend();
            };// doOnData

        bool
        doIsAccepting(const wns::ldk::CompoundPtr& /* compound */) const
            {
                return hasCapacity();
            }; // doIsAccpting

        void
        doWakeup()
            {
                tryToSend();
            }; // doWakeup

        // the special tryToSend method...
        void tryToSend();

    private:
	void onFUNCreated();

	const std::string protocolCalculatorName;
        wifimac::management::ProtocolCalculator *protocolCalculator;

        wns::ldk::CompoundPtr storage;
        bool inWakeup;

    };


} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_NEXTFRAMEGETTER_HPP
