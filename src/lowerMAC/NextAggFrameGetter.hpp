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

#ifndef WIFIMAC_LOWERMAC_NEXTAGGFRAMEGETTER_HPP
#define WIFIMAC_LOWERMAC_NEXTAGGFRAMEGETTER_HPP

#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Command.hpp>
#include <WIFIMAC/lowerMAC/NextFrameGetter.hpp>
#include <WIFIMAC/lowerMAC/MultiBuffer.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>

namespace wifimac { namespace lowerMAC {

    /**
	 * @brief NextAggFrameGetter enables peek-ahead of the next compound(s)
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
    class NextAggFrameGetter:
	  public wifimac::lowerMAC::NextFrameGetter
    {
    public:

        NextAggFrameGetter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& /*config*/);

        // Delayed interface
        virtual void processOutgoing(const wns::ldk::CompoundPtr&);

	virtual Bit getNextSize() const;

    private:
	void onFUNCreated();

        const std::string bufferName;
        const std::string protocolCalculatorName;
        wifimac::management::ProtocolCalculator *protocolCalculator;

	struct Friends
        {
            wifimac::lowerMAC::MultiBuffer* buffer;
        } friends;

	int32_t numOfCompounds;
	Bit PSDUSize;
    };


} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_NEXTAGGFRAMEGETTER_HPP
