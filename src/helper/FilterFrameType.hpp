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

#ifndef WIFIMAC_HELPER_FILTERFRAMETYPE_HPP
#define WIFIMAC_HELPER_FILTERFRAMETYPE_HPP

#include <WIFIMAC/FrameType.hpp>
#include <DLL/compoundSwitch/Filter.hpp>

namespace wifimac { namespace helper {

    class FilterFrameType:
        public dll::compoundSwitch::Filter
    {
    public:
        FilterFrameType(dll::compoundSwitch::CompoundSwitch* compoundSwitch, wns::pyconfig::View& config);
        ~FilterFrameType();

        virtual void
        onFUNCreated();

        virtual bool
        filter(const wns::ldk::CompoundPtr& compound) const;

    private:
        wifimac::FrameType acceptingFrameType;
        wns::ldk::CommandReaderInterface* commandReader;
    };
} // Helper
} // WiFiMac

#endif
