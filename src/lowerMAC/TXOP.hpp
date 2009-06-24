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
#include <WIFIMAC/lowerMAC/ITXOPObserver.hpp>
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
     * implementing the ITXOPTimeWindow interface to determine the duration of a possible subsequent transmission
     * and wether it fits into the remaining TXOP duration or not.
     * If this is the case, the NAV of the current compound is set to cover
     * its successor. Otherwise, the current TXOP is closed, another round
     * is initiated. 
     * To disable TXOP the TXOP limit has to be set to 0
     * patient / impatient: when impatient, TXOP is done for every transmission, whereas
     * in patient mode, in order to have a(ny) (TXOP) transmission, startTXOP() has to be called
     * in order to trigger this FU and let compounds be passed down. Note that after the current TXOP duration
     * in patient mode has been exceeded, the FU is not accepting any further compounds from upper FUs until it has
     * been triggered once again. (not even ARQ retransmissions)
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

	/** @brief in patient mode: starts an TXOP round 
	*
	* after duration has been exceeded, the TXOP FU doesn't accept any further compounds
	* from above FUs until another call off startTXOP()
	*/
	bool
	startTXOP(wns::simulator::Time duration);

	void
	registerObserver(wifimac::lowerMAC::ITXOPObserver *observer) {observers.push_back(observer);}

    private:
        /** @brief Processor Interface Implementation */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

        bool doIsAccepting(const wns::ldk::CompoundPtr& compound) const;

        void onFUNCreated();


	void closeTXOP();

        const std::string managerName;
        const std::string protocolCalculatorName;
        const std::string txopWindowName;
        const std::string raName;

        /** @brief Duration of the Short InterFrame Space */
        const wns::simulator::Time sifsDuration;
        const wns::simulator::Time expectedACKDuration;
        const bool singleReceiver;
	const bool impatient;

	wns::simulator::Time txopLimit;
        wns::simulator::Time remainingTXOPDuration;
        wns::service::dll::UnicastAddress txopReceiver;

        wns::logger::Logger logger;
	bool firstTXOPCompound;

	std::vector<wifimac::lowerMAC::ITXOPObserver *> observers;
        wifimac::management::ProtocolCalculator* protocolCalculator;
        struct Friends
        {
            wifimac::lowerMAC::Manager* manager;
            wifimac::lowerMAC::ITXOPWindow* txopWindow;
            wifimac::lowerMAC::RateAdaptation* ra;
        } friends;

	wns::probe::bus::ContextCollectorPtr TXOPDurationProbe;
	bool useTXOP;
    };

} // lowerMAC
} // wifimac

#endif // WIFIMAC_LOWERMAC_TXOP_HPP
