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

#include <WIFIMAC/management/protocolCalculatorPlugins/ErrorProbability.hpp>

#include <WNS/Ttos.hpp>
#include <WNS/simulator/Time.hpp>

#include <cmath>
#include <algorithm>

using namespace wifimac::management::protocolCalculatorPlugins;

ErrorProbability::ErrorProbability()
{

}

double
ErrorProbability::getPER(wns::Ratio postSNR,
                         Bit packetLength,
                         wifimac::convergence::PhyMode phyMode) const
{
    double cyclicPrefixReduction = 0.0;
    if(phyMode.getGuardIntervalDuration() == 0.8e-6)
    {
        cyclicPrefixReduction = 0.8;
    }
    if(phyMode.getGuardIntervalDuration() == 0.4e-6)
    {
        cyclicPrefixReduction = 0.9;
    }
    assure(cyclicPrefixReduction > 0, "Unknown guard interval");

    postSNR.set_factor(postSNR.get_factor() * cyclicPrefixReduction);

    double bitSuccessRate = 1.0;

    std::vector<wifimac::convergence::MCS> v = phyMode.getSpatialStreams();
    assure(v.size() > 0, "Must have at least one spatial stream");

    for(std::vector<wifimac::convergence::MCS>::const_iterator it = v.begin();
        it != v.end();
        ++it)
    {
        double ser = 0.0;
        double ber = 0.0;

        if(it->getModulation() == "BPSK")
        {
            ser = Q(sqrt(2*postSNR.get_factor()));
            ber = ser;
        }
        else if(it->getModulation() == "QPSK")
        {
            ser = 2.0 * Q(sqrt(postSNR.get_factor())) * (1.0 - 0.5 * Q(sqrt(postSNR.get_factor())));
            ber = 0.5 * ser;
        }
        else if(it->getModulation() == "QAM16")
        {
            double P_sqrt16 = 3.0/2.0 * Q(sqrt((3.0/15.0) * postSNR.get_factor()));
            ser = 1 - pow((1 - P_sqrt16), 2);
            ber = ser / 4.0;
        }
        else if(it->getModulation() == "QAM64")
        {
            double P_sqrt64 = 7.0/4.0 * Q(sqrt((3.0/63.0) * postSNR.get_factor()));
            ser = 1 - pow((1 - P_sqrt64), 2);
            ber = ser / 6.0;
        }

        bitSuccessRate *= (1.0 - ber);

        assure((bitSuccessRate >= 0.0 and bitSuccessRate <= 1.0) and
               (ber >= 0.0 and ber <= 1.0) and
               (ser >= 0.0 and ser <= 1.0),
               "Calculated error probabilities are not valid");
    }

    double ber = (1.0 - bitSuccessRate);
    double u = 0.0;

    // only the first coding rate is taken
    if(phyMode.getSpatialStreams()[0].getRate() ==  "1/2")
    {
        u = std::min(1.0, Pu12(ber));
    }
    else if(phyMode.getSpatialStreams()[0].getRate() == "2/3")
    {
        u = std::min(1.0, Pu23(ber));
    }
    else if(phyMode.getSpatialStreams()[0].getRate() == "3/4")
    {
        u = std::min(1.0, Pu34(ber));
    }
    else if(phyMode.getSpatialStreams()[0].getRate() == "5/6")
    {
        u = std::min(1.0, Pu56(ber));
    }

    double per = 1.0 - pow(1.0 - u, static_cast<double>(packetLength));

    assure((per >= 0.0 and per <= 1.0) and
           (u >= 0.0 and u <= 1.0),
           "Calculated error probabilities are not valid");

    return per;
}

double
ErrorProbability::Pd(double p, double d) const
{
    // Chernoff - Approximation
    return (pow(4*p*(1-p), d/2));
}

double
ErrorProbability::Pu12(double rawBer) const
{
    return( 11 * Pd(rawBer,10) +
            38 * Pd(rawBer,12) +
            193 * Pd(rawBer, 14) +
            1331 * Pd(rawBer, 16) +
            7275 * Pd(rawBer, 18) +
            40406 * Pd(rawBer, 20) +
            234969 * Pd(rawBer, 22) );
}

double
ErrorProbability::Pu23(double rawBer) const
{
    return( Pd(rawBer,6) +
            16 * Pd(rawBer, 7) +
            48 * Pd(rawBer, 8) +
            158 * Pd(rawBer, 9) +
            642 * Pd(rawBer, 10) +
            2435 * Pd(rawBer, 11) +
            6174 * Pd(rawBer, 12) +
            34705 * Pd(rawBer, 13) +
            131585 * Pd(rawBer, 14) +
            499608 * Pd(rawBer, 15));
}

double
ErrorProbability::Pu34(double rawBer) const
{
    return( 8 * Pd(rawBer, 5) +
            31 * Pd(rawBer, 6) +
            160 * Pd(rawBer, 7) +
            892 * Pd(rawBer, 8) +
            4512 * Pd(rawBer, 9) +
            23307 * Pd(rawBer, 10) +
            121077 * Pd(rawBer, 11) +
            625059 * Pd(rawBer, 12) +
            3234886 * Pd(rawBer, 13) +
            16753077 * Pd(rawBer, 14));
}

double
ErrorProbability::Pu56(double rawBer) const
{
    return( 14 * Pd(rawBer, 4) +
            69 * Pd(rawBer, 5) +
            654 * Pd(rawBer, 6) +
            4996 * Pd(rawBer, 7) +
            39677 * Pd(rawBer, 8) +
            314973 * Pd(rawBer, 9) +
            2503576 * Pd(rawBer, 10) +
            19875546 * Pd(rawBer, 11));
}

double
ErrorProbability::Q(double x) const
{
    return( erfc(x/sqrt(2.0)) /  2.0);
}

