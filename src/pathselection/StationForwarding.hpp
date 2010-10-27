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

#ifndef WIFIMAC_PATHSELECTION_STATIONFORWARDING_HPP
#define WIFIMAC_PATHSELECTION_STATIONFORWARDING_HPP

#include <WIFIMAC/Layer2.hpp>
#include <WIFIMAC/pathselection/ForwardingCommand.hpp>
#include <WIFIMAC/pathselection/IPathSelection.hpp>

#include <WNS/ldk/ManagementServiceInterface.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/service/dll/Address.hpp>

#include <DLL/UpperConvergence.hpp>


namespace wifimac { namespace pathselection {

    /**
	 * @brief Forwarding of data frames from a STA perspective
     *
     * The forwarding of data frames in a STA is very easy: Frames are always
     * send to the AP to which the STA is associated to.
	 */
    class StationForwarding :
        public wns::ldk::fu::Plain<StationForwarding, ForwardingCommand>
    {
    public:
        /** @brief Constructor */
        StationForwarding(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        /** @brief Destructor */
        virtual
        ~StationForwarding();

    private:
        /** @brief If associated, we accept if the lower FU is accepting */
        virtual bool
        doIsAccepting(const wns::ldk::CompoundPtr& _compound) const;

        /** @brief The target address becomes the final destination, the
         * association address becomes the target. */
        virtual void doSendData(const wns::ldk::CompoundPtr& _compound);

        /** @brief Forward the wakeup call  */
        virtual void
        doWakeup();

        /** @brief Exchange target source agains original source and deliver */
        virtual void
        doOnData(const wns::ldk::CompoundPtr& _compound);

        /** @brief Initialize the FU */
        virtual void onFUNCreated();

        /** @brief  The config */
        const wns::pyconfig::View config;

        /** @brief  The logger */
        wns::logger::Logger logger;

        /** @brief Name of the upper convergence FU  */
        const std::string ucName;

        /** @brief Pointer to Layer2  */
        wifimac::Layer2* layer2;
    };
} // mac
} // wifimac

#endif // WIFIMAC_SCHEDULER_HPP
