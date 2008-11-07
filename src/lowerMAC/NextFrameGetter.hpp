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

namespace wifimac { namespace lowerMAC {

	class NextFrameGetter:
        public wns::ldk::fu::Plain<NextFrameGetter, wns::ldk::EmptyCommand>,
        public wns::ldk::Delayed<NextFrameGetter>
	{
	public:

		NextFrameGetter(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& /*config*/) :
            wns::ldk::fu::Plain<NextFrameGetter, wns::ldk::EmptyCommand>(fun),
            storage(),
            flowTime(0.0)
        {}

        // Delayed interface
		virtual void processIncoming(const wns::ldk::CompoundPtr& compound);
		virtual void processOutgoing(const wns::ldk::CompoundPtr&);
		virtual bool hasCapacity() const;
		virtual const wns::ldk::CompoundPtr hasSomethingToSend() const;
		virtual wns::ldk::CompoundPtr getSomethingToSend();

    private:
        std::deque<wns::ldk::CompoundPtr> storage;
        wns::simulator::Time flowTime;
    };


} // mac
} // wifimac

#endif // WIFIMAC_LOWERMAC_NEXTFRAMEGETTER_HPP
