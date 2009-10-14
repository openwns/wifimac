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

#ifndef WIFIMAC_CONVERGENCE_GUIWRITER_HPP
#define WIFIMAC_CONVERGENCE_GUIWRITER_HPP

#include <WNS/ldk/fun/FUN.hpp>

#include <WNS/probe/bus/ContextCollector.hpp>

namespace wifimac { namespace lowerMAC {
        class Manager;
}}
namespace wifimac { namespace convergence {
    class PhyUser;

    /**
     * @brief Writer to Gui log
     *
     * The GuiWriter will write the log file that can be read by Gui.
     * 
     */
    class GuiWriter
    {

    public:
        GuiWriter();
        virtual ~GuiWriter();

        void setManagerAndFun(wifimac::lowerMAC::Manager* manager, wns::ldk::fun::FUN* fun);
        void writeToProbe(const wns::ldk::CompoundPtr& compound, wns::simulator::Time frameTxDuration);

    private:
        bool first_created;
        wns::probe::bus::ContextCollectorPtr guiProbe_;
        wifimac::lowerMAC::Manager* manager_;
        wns::ldk::fun::FUN* fun_;

        static int gui_station_counter_;

        int gui_station_id_;
    };

} // namespace convergence
} // namespace wifimac

#endif // NOT defined WIFIMAC_CONVERGENCE_GUIWRITER_HPP


