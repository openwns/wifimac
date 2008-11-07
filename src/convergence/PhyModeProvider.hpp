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

#ifndef WIFIMAC_CONVERGENCE_PHYMODEPROVIDER_HPP
#define WIFIMAC_CONVERGENCE_PHYMODEPROVIDER_HPP

#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/container/Registry.hpp>
#include <WNS/PowerRatio.hpp>

namespace wifimac { namespace convergence {

	/**
	 * @brief The PhyModeProvider holds all configured PhyModes and
	 *    provides methods for their simple access by the rate adaption
	 *
	 * The PhyModes are ordered by their number of data bits per symbol (see PhyMode.hpp).
	 * This allows for the methods rate[Up|Down], get[Lowest|Highest].
	 * Of course, their robustness to interference should be ordered in the other way round.
	 */
	class PhyModeProvider {
	public:
		PhyModeProvider(const wns::pyconfig::View& config);

		/**
		 * @brief Returns the next higher PhyMode 
		 */
		PhyMode rateUp(PhyMode pm) const;
		PhyMode rateUp(int id) const;
		/**
		 * @brief Returns the next lower PhyMode 
		 */
		PhyMode rateDown(PhyMode pm) const;
		PhyMode rateDown(int id) const;

		/**
		 * @brief Returns the lowest PhyMode 
		 */
		PhyMode getLowest() const;

		/**
		 * @brief Returns the highest PhyMode 
		 */
		PhyMode getHighest() const;

		/**
		 * @brief Returns the highest/lowest PhyModeId 
		 */
		int getHighestId() const;
		int getLowestId() const;

		/**
		 * @brief Returns the PhyMode which is used to model
		 *   the preamble transmissions.
		 */
		PhyMode getPreamblePhyMode() const;

		/**
		 * @brief Returns the PhyMode by its id
		 */
		PhyMode getPhyMode(const int id) const;

		/**
		 * @brief Returns the duration of one OFDM symbol
		 */
		wns::simulator::Time getSymbolDuration() const;

        /** @brief return the optimal phymode for a suggested sinr */
        PhyMode getPhyMode(wns::Ratio sinr) const;
        int getPhyModeId(wns::Ratio sinr) const;

        /** @brief returns the minimal SINR for a connection */
        wns::Ratio getMinSINR() const;

	private:
		wns::container::Registry<int, PhyMode> id2phyMode;
		wns::container::Registry<PhyMode, int> phyMode2id;

		PhyMode preamblePhyMode;

		int numPhyModes;
		wns::simulator::Time symbolDuration;
        wns::Ratio switchingPointOffset;
	};
}}

#endif // WIFIMAC_CONVERGENCE_PHYMODEPROVIDER_HPP
