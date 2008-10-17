/******************************************************************************
 * WiFiMac                                                                    *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2005-2007                                                    *
 * Lehrstuhl fuer Kommunikationsnetze (ComNets)                               *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
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
