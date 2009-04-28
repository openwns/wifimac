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

#ifndef WIFIMAC_CONVERGENCE_PHYMODEPROVIDER_HPP
#define WIFIMAC_CONVERGENCE_PHYMODEPROVIDER_HPP

#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/PowerRatio.hpp>

namespace wifimac { namespace convergence {

	/**
	 * @brief The PhyModeProvider holds all configured PhyModes and
	 *    provides methods for their simple access by the rate adaption
	 *
	 * The PhyModes are ordered by their number of data bits per symbol (see
	 * PhyMode.hpp).  This allows for the methods mcs[Up|Down],
	 * has[Lowest|Highest]. Of course, their robustness to interference should
	 * be ordered in the other way round.
	 */
    class PhyModeProvider {
    public:
        PhyModeProvider(const wns::pyconfig::View& config);

        /**
		 * @brief Returns the PhyMode with more dbps, but also with higher SINR requirement
		 */
        void mcsUp(PhyMode& pm) const;
        /**
		 * @brief Returns the next lower PhyMode
		 */
        void mcsDown(PhyMode& pm) const;

        /**
		 * @brief Returns if the phy mode uses the lowest MCS
		 */
        bool hasLowestMCS(const PhyMode& pm) const;

        /**
		 * @brief Returns if the phy mode uses the highest
		 */
        bool hasHighestMCS(const PhyMode& pm) const;

        /**
		 * @brief Returns the PhyMode which is used to model
		 *   the preamble transmissions.
		 */
        PhyMode getPreamblePhyMode(PhyMode pmFrame) const;

        PhyMode getDefaultPhyMode() const;

        /** @brief return the optimal MCS for a suggested sinr */
        MCS getMCS(wns::Ratio postSINR) const;

        /** @brief returns the minimal SINR for a connection */
        wns::Ratio getMinSINR() const;

    private:
        std::map<wns::Ratio, MCS> sinr2mcs;

        wns::Ratio switchingPointOffset;

        PhyMode defaultPhyMode;
        PhyMode preamblePhyMode;
    };
}}

#endif // WIFIMAC_CONVERGENCE_PHYMODEPROVIDER_HPP
