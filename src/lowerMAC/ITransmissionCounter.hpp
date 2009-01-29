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

#ifndef WIFIMAC_LOWERMAC_ITRANSMISSIONCOUNTER_HPP
#define WIFIMAC_LOWERMAC_ITRANSMISSIONCOUNTER_HPP

namespace wifimac { namespace lowerMAC {

    class ITransmissionCounter
    {
    public:
        virtual ~ITransmissionCounter()
            {}

        virtual size_t
        getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const = 0;

        virtual void
        copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst) = 0;

        // indication from outside that the transmission has failed
        virtual void
        transmissionHasFailed(const wns::ldk::CompoundPtr& compound) = 0;

    };
}
}

#endif // WIFIMAC_LOWERMAC_ITRANSMISSIONCOUNTER_HPP