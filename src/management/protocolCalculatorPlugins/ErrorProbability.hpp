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

#ifndef WIFIMAC_MANAGEMENT_PROTOCOLCALCULATORPLUGINS_ERRORPROBABILITY_HPP
#define WIFIMAC_MANAGEMENT_PROTOCOLCALCULATORPLUGINS_ERRORPROBABILITY_HPP

#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/PowerRatio.hpp>

namespace wifimac { namespace management { namespace protocolCalculatorPlugins {

    struct ErrorStatistic
    {
        /// Final packet error rate
        double per;
        /// Raw bit error rate
        double ber;
        /// First error event probability during decoding
        double u;
        /// Symbol error rate
        double ser;

        ErrorStatistic()
            {
                per = -1.0;
                ber = -1.0;
                u = -1.0;
                ser = -1.0;
            };

        bool valid() const
            {
                return((per >= 0.0 and per <= 1.0) and
                       (ber >= 0.0 and ber <= 1.0) and
                       (u >= 0.0 and u <= 1.0) and
                       (ser >= 0.0 and ser <= 1.0));
            }
    };

    /**
     * @brief This class calculates the packet|bit|symbol error probability for
     * a given SNR, packet length and wifimac::convergence::PhyMode.
     */
    class ErrorProbability
    {
    public:
        ErrorProbability();
        virtual ~ErrorProbability() {};

        ErrorStatistic
        getError(wns::Ratio postSNR, Bit packetLength, wifimac::convergence::PhyMode phyMode) const;

        double
        getPER(wns::Ratio postSNR, Bit packetLength, wifimac::convergence::PhyMode phyMode) const;

    private:
        /// @brief Calculate probability of error in pairwise comparison of two
        /// paths that differ in d bits
        double Pd(double rawBer, double d) const;

        /// @brief Calculate first error event probability of coding rate 1/2
        double Pu12(double rawBer) const;

        /// @brief Calculate first error event probability of coding rate 2/3
        double Pu23(double rawBer) const;

        /// @brief Calculate first error event probability of coding rate 3/4
        double Pu34(double rawBer) const;

        /// @brief Calculate first error event probability of coding rate 5/6
        double Pu56(double rawBer) const;

        /// @brief Computes the Q function as found in Proakis (3rd ed, 1995, eq. 2-1-98)
        double Q(double x) const;

    };
} // protocolCalculatorPlugins
} // management
} // wifimac

#endif
