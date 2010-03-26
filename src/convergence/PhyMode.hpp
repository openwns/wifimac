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

// must be the first include!
#include <WNS/Python.hpp>
#include <WNS/PowerRatio.hpp>
#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>
#include <WNS/Ttos.hpp>
#include <WNS/simulator/Bit.hpp>

#include <WIFIMAC/management/protocolCalculatorPlugins/ConfigGetter.hpp>

#include <math.h>
#include <map>

namespace wifimac { namespace convergence {

    class PhyMode;

    class MCS {
        // PhyMode needs to access the nominator and denominator
        friend class PhyMode;
    public:
        MCS();
        MCS(const wns::pyconfig::View& config);
        MCS(const MCS& other);
        MCS(const wifimac::management::protocolCalculatorPlugins::ConfigGetter& config);

        std::string getModulation() const;
        std::string getRate() const;

        void
        setIndex(unsigned int index)
            { this->index = index;};
        unsigned int
        getIndex() const
            { return this->index;};

        wns::Ratio getMinSINR() const
            { return this->minSINR;};
        /**
		 * @brief Compare two MCSs using the number of bits / symbol
		 */
        bool operator <(const MCS& rhs) const;
        bool operator ==(const MCS& rhs) const;
        bool operator !=(const MCS& rhs) const;

    private:
        void setMCS(const std::string& newModulation, const std::string& newCodingRate);

        std::string modulation;
        std::string codingRate;

        unsigned int nominator;
        unsigned int denominator;

        unsigned int index;
        wns::Ratio minSINR;
    };

    inline std::ostream& operator<< (std::ostream& s, const MCS& mcs)
    {
        return s << mcs.getModulation() << "-" << mcs.getRate();
    };

    /**
	 * @brief Holder for all necessary information about a PhyMode
	 *
	 * - Data-bits / Symbol
	 * - Method to compute the PER out of rss, sinr, length
	 * - Method to compute the number of symbols for a given number of data bits
	 * - Description (modulation, codingRate, index)
     * - Number of spatial streams
	 */
    class PhyMode {
    public:
        /** Constructors **/
        PhyMode();
        PhyMode(const wns::pyconfig::View& config);
        PhyMode(const wifimac::management::protocolCalculatorPlugins::ConfigGetter& config);

        /**
         * @brief Returns the number of data bits per symbol
         */
        Bit getDataBitsPerSymbol() const;

        /**
         * @brief Get the number of spatial streams
         */
        unsigned int getNumberOfSpatialStreams() const
            { return this->spatialStreams.size(); };

        void setMCS(const MCS& mcs)
            { this->setUniformMCS(mcs, 1); };

        void setUniformMCS(const MCS& mcs, unsigned int numSS);

        std::vector<MCS> getSpatialStreams() const
            { return this->spatialStreams; };
        void setSpatialStreams(const std::vector<MCS>& ss);

        wns::Ratio getMinSINR() const;
            { return this->spatialStreams[0].getMinSINR();};

        unsigned int getNumberOfDataSubcarriers() const
            { return this->numberOfDataSubcarriers;};
        void setNumberOfDataSubcarriers(unsigned int ds)
            { this->numberOfDataSubcarriers = ds; };

        std::string getPreambleMode() const
            { return this->plcpMode;};
        void setPreambleMode(std::string pm)
            { this->plcpMode = pm;}

        wns::simulator::Time getGuardIntervalDuration() const
            { return this->guardIntervalDuration;};
        void setGuardIntervalDuration(wns::simulator::Time gi)
            { this->guardIntervalDuration = gi;};

        /**
		 * @brief Compare two phyModes using the number of bits / symbol
		 */
        bool operator <(const PhyMode& rhs) const;
        /**
		 * @brief Compare two phyModes using the number of bits / symbol
		 */
        bool operator ==(const PhyMode& rhs) const;
        bool operator !=(const PhyMode& rhs) const;

    private:
        std::vector<MCS> spatialStreams;
        unsigned int numberOfDataSubcarriers;
        std::string plcpMode;
        wns::simulator::Time guardIntervalDuration;
    };

    inline std::ostream& operator<< (std::ostream& s, const PhyMode& p)
    {
        s << "|";
        std::vector<MCS> ss = p.getSpatialStreams();
        for (std::vector<MCS>::iterator it = ss.begin();
             it != ss.end();
             ++it)
        {
            s << (*it) << "|";
        }
        s << "*" << wns::Ttos(p.getNumberOfDataSubcarriers())
                  << " (-> " << wns::Ttos(p.getDataBitsPerSymbol()) << " dbps)";
        return s;
    };
}}

#endif // WIFIMAC_CONVERGENCE_PHYMODE_HPP
