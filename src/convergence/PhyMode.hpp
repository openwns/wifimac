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
#ifndef WIFIMAC_CONVERGENCE_PHYMODE_HPP
#define WIFIMAC_CONVERGENCE_PHYMODE_HPP

#include <WNS/PowerRatio.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>

#include <WNS/simulator/Bit.hpp>

#include <math.h>
#include <map>

namespace wifimac { namespace convergence {
	/**
	 * @brief Holder for all necessary information about a PhyMode
	 *
	 * - Data-bits / Symbol
	 * - SINR to BER mapping
	 * - Method to compute the PER out of rss, sinr, length
	 * - Method to compute the number of symbols for a given number of data bits
	 * - Description (modulation, codingRate, index)
	 */
	class PhyMode {
	public:
		/** Constructors **/
		PhyMode();
		PhyMode(const wns::pyconfig::View& config, int index);

		/**
		 * @brief Read in the configuration for this Phy-Mode
		 */
		void configPhyMode(const wns::pyconfig::View& config);

		/**
		 * @brief Compute the number of symbols required to transmit
		 *    length data bits
		 */
		int getNumSymbols(Bit length) const;

        /**
         * @brief Returns the number of bits per symbol, incorporating multiple
         * streams
         */
        Bit getDataBitsPerSymbol() const;

        /**
         * @brief Set the number of spatial streams for transmission
         */
        void setNumberOfSpatialStreams(unsigned int nss);

        /**
         * @brief Get the number of spatial streams for transmission
         */
        unsigned int getNumberOfSpatialStreams() const;

		/**
		 * @brief Output the phyMode description
		 */
        std::string getString() const { return modulation_+"-"+codingRate_;};

        std::string getModulation() const { return modulation_;};
        std::string getRate() const { return codingRate_;};

		int getIndex() const { return index; };

		/**
		 * @brief Compare two phyModes using the number of bits / symbol
		 */
		bool operator <(const PhyMode& rhs) const;
		/**
		 * @brief Compare two phyModes using the number of bits / symbol
		 */
		bool operator ==(const PhyMode& rhs) const;
        bool operator !=(const PhyMode& rhs) const;

        wns::Ratio getMinSINR()
            { return this->minSINR; }

	private:
		Bit dataBitsPerSymbol_;
		std::string modulation_;
		std::string codingRate_;
		Bit constFrameSize_;
		wns::Power minRSS_;
        wns::Ratio minSINR;
        unsigned int numberOfSpatialStreams_;

		wns::logger::Logger logger_;

		int index;
	};

	inline std::ostream& operator<< (std::ostream& s, const PhyMode& p)
	{
		return s << p.getString();
	}
}}

#endif // WIFIMAC_CONVERGENCE_PHYMODE_HPP
