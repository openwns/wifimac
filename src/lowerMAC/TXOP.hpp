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

#ifndef WIFIMAC_LOWERMAC_TXOP_HPP
#define WIFIMAC_LOWERMAC_TXOP_HPP

#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/lowerMAC/ITXOPWindow.hpp>
#include <WIFIMAC/lowerMAC/RateAdaptation.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>
#include <WNS/ldk/Delayed.hpp>
#include <WNS/ldk/probe/Probe.hpp>

#include <WNS/events/CanTimeout.hpp>
#include <WNS/probe/bus/ContextCollector.hpp>

#include <WNS/Observer.hpp>

namespace wifimac { namespace lowerMAC {
	/** 
     * @brief FU implementing TXOP functionality
     *
     * this FU mainly is responsible for setting the NAV of outgoing compounds
     * correctly, according to 802.11 standard.
     * In order to calculate the NAV for a given compound, it uses an FU
     * implementing the ITXOPTimeWindow interface to determine if more
     * compounds follow the current one and if the transmission duration
     * of the next one will fit into the remaining TXOP duration (including
     * SIFSs and expected ACK duration).
     * If this is the case, the NAV of the current compound is set to cover
     * its successor. Furthermore the ITXOPTimeWindow FU is updated to the
     * the remaining TXOP duration in order to let the successor pass the FUN
     * up to the TXOP FU. Otherwise, the current TXOP is closed, another round
     * is initiated and the ITXOPTimeWindow instance is resetted to the TXOP
     * maximum limit, and let further compounds for the new round enter the FUN
     * up to the TXOP FU.
     * To disable TXOP the TXOP limit has to be set to 0
     */
    class TXOP:
        public wns::ldk::fu::Plain<TXOP, wns::ldk::EmptyCommand>,
        public wns::ldk::Processor<TXOP>,
	public wns::ldk::probe::Probe
    {
    public:

        TXOP(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        virtual
        ~TXOP();

        // observer TxStartEnd
        void
        onTxStart(const wns::ldk::CompoundPtr& compound);
        void
        onTxEnd(const wns::ldk::CompoundPtr& compound);

    private:
        /** @brief Processor Interface Implementation */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        bool doIsAccepting(const wns::ldk::CompoundPtr& compound) const;

        void onFUNCreated();

        const std::string managerName;
        const std::string protocolCalculatorName;
        const std::string txopWindowName;
        const std::string raName;

        /** @brief Duration of the Short InterFrame Space */
        const wns::simulator::Time sifsDuration;
        const wns::simulator::Time expectedACKDuration;
        const wns::simulator::Time txopLimit;
        const bool singleReceiver;

        wns::simulator::Time remainingTXOPDuration;
        wns::service::dll::UnicastAddress txopReceiver;

        wns::logger::Logger logger;

        wifimac::management::ProtocolCalculator* protocolCalculator;
        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
            wifimac::lowerMAC::ITXOPWindow* txopWindow;
            wifimac::lowerMAC::RateAdaptation* ra;
        } friends;

	wns::probe::bus::ContextCollectorPtr TXOPDurationProbe;
    };


} // lowerMAC
} // wifimac

#endif // WIFIMAC_LOWERMAC_TXOP_HPP
