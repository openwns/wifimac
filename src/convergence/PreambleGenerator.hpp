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

#ifndef WIFIMAC_CONVERGENCE_PREAMBLEGENERATOR_HPP
#define WIFIMAC_CONVERGENCE_PREAMBLEGENERATOR_HPP

#include <WIFIMAC/lowerMAC/Manager.hpp>
#include <WIFIMAC/convergence/PhyUser.hpp>
#include <WIFIMAC/management/ProtocolCalculator.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Delayed.hpp>

namespace wifimac { namespace convergence {

    class PreambleGeneratorCommand:
        public wns::ldk::Command
    {
    public:
        struct {} local;
        struct {
            // id of the upcoming frame to which the preamble belongs to
            wns::Birthmark frameId;
        } peer;
        struct {} magic;

        wns::Birthmark getFrameId() const
        {
            return(this->peer.frameId);
        }
    };

    /**
     * @brief Prefixes an OFDM-preamble to any compound
     *
     * A IEEE 802.11 OFDM transmission (according to .11a/g) consists of several
     * elements:
     * - The PLCP preamble
     * - The SIGNAL (containing rate, length, parity)
     * - SERVICE bits
     * - The PSDU
     * - Tail and padding bits
     * The last three elements are coded using the code rate as defined by the
     * MCS. The first two have a constant duration (e.g. 12+1 symbol in .11g)
     * and are neccessary to decode the following PSDU. In the simulator, this
     * first part is simulated as a separate transmission, before the actual
     * PSDU transmission, called the "Preamble". The PreambleGenerator creates
     * this preamble, sends it and delays the actual PSDU.
     */
    class PreambleGenerator:
        public wns::ldk::fu::Plain<PreambleGenerator, PreambleGeneratorCommand>,
        public wns::ldk::Delayed<PreambleGenerator>
	{
	public:
		PreambleGenerator(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

		virtual
		~PreambleGenerator();

	private:
		/** @brief Delayed Interface */
		void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);
        bool hasCapacity() const;
        const wns::ldk::CompoundPtr hasSomethingToSend() const;
        wns::ldk::CompoundPtr getSomethingToSend();

        void onFUNCreated();

        const std::string phyUserName;
        const std::string protocolCalculatorName;
        const std::string managerName;

        wns::logger::Logger logger;

        wns::ldk::CompoundPtr pendingCompound;
        wns::ldk::CompoundPtr pendingPreamble;
	wifimac::management::ProtocolCalculator* protocolCalculator;

	struct Friends
	{
		wifimac::convergence::PhyUser* phyUser;
        	wifimac::lowerMAC::Manager* manager;
	} friends;
	};


} // mac
} // wifimac

#endif // WIFIMAC_CONVERGENCE_PREAMBLEGENERATOR_HPP
