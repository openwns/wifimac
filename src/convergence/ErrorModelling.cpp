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

#include <WIFIMAC/convergence/ErrorModelling.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>
#include <WIFIMAC/lowerMAC/Manager.hpp>

#include <WNS/Assure.hpp>
#include <WNS/Exception.hpp>

#include <cmath>
#include <algorithm>

using namespace wifimac::convergence;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::convergence::ErrorModelling,
    wns::ldk::FunctionalUnit,
    "wifimac.convergence.ErrorModelling",
    wns::ldk::FUNConfigCreator);

ErrorModelling::ErrorModelling(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& _config) :
    wns::ldk::fu::Plain<ErrorModelling, ErrorModellingCommand>(fun),
    config(_config),
    logger(config.get<wns::pyconfig::View>("logger")),
    phyUserCommandName(config.get<std::string>("phyUserCommandName")),
    managerCommandName(config.get<std::string>("managerCommandName")),
    cyclicPrefixReduction(config.get<double>("cyclicPrefixReduction"))
{

}

void ErrorModelling::processIncoming(const wns::ldk::CompoundPtr& compound)
{
    wns::Ratio sinr = getFUN()->getCommandReader(phyUserCommandName)->
        readCommand<wifimac::convergence::CIRProviderCommand>(compound->getCommandPool())->getCIR();
    wns::Power rss = getFUN()->getCommandReader(phyUserCommandName)->
        readCommand<wifimac::convergence::CIRProviderCommand>(compound->getCommandPool())->getRSS();
    wifimac::convergence::PhyMode phyMode = getFUN()->getCommandReader(managerCommandName)->
        readCommand<wifimac::lowerMAC::ManagerCommand>(compound->getCommandPool())->getPhyMode();

    ErrorModellingCommand* emc = activateCommand(compound->getCommandPool());

    Bit commandPoolSize = 0;
    Bit dataSize = 0;
    this->calculateSizes(compound->getCommandPool(), commandPoolSize, dataSize);

    // Reduction due to cyclic prefix
    sinr.set_factor(sinr.get_factor() * cyclicPrefixReduction);

    // Symbol error probability
    double P_ser = -1;
    // Raw bit error probability
    double P_ber = -1;

    if(phyMode.getModulation() == "BPSK")
    {
        P_ser = Q(sqrt(2*sinr.get_factor()));
        P_ber = P_ser;
    }
    if(phyMode.getModulation() == "QPSK")
    {
        P_ser = 2.0 * Q(sqrt(sinr.get_factor())) * (1.0 - 0.5 * Q(sqrt(sinr.get_factor())));
        P_ber = 0.5 * P_ser;
    }
    if(phyMode.getModulation() == "16QAM")
    {
        double P_sqrt16 = 3.0/2.0 * Q(sqrt((3.0/15.0) * sinr.get_factor()));
        P_ser = 1 - pow((1 - P_sqrt16), 2);
        P_ber = P_ser / 4.0;
    }
    if(phyMode.getModulation() == "64QAM")
    {
        double P_sqrt64 = 7.0/4.0 * Q(sqrt((3.0/63.0) * sinr.get_factor()));
        P_ser = 1 - pow((1 - P_sqrt64), 2);
        P_ber = P_ser / 6.0;
    }

    assure(P_ser >= 0.0 and P_ser <= 1.0,
           "Error computing P_ser to " << P_ser << ", modulation is " << phyMode.getModulation());
    assure(P_ber >= 0.0 and P_ber <= 1.0,
           "Error computing P_ber to " << P_ber << ", modulation is " << phyMode.getModulation());

    // First error event probability
    double P_u = -1;

    if(phyMode.getRate() ==  "1/2")
    {
        P_u = std::min(1.0, Pu12(P_ber));
    }
    if(phyMode.getRate() == "2/3")
    {
        P_u = std::min(1.0, Pu23(P_ber));
    }
    if(phyMode.getRate() == "3/4")
    {
        P_u = std::min(1.0, Pu34(P_ber));
    }
    if(phyMode.getRate() == "5/6")
    {
        P_u = std::min(1.0, Pu56(P_ber));
    }

    assure(P_u >= 0.0,
           "Error computing P_u to " << P_u << ", coding rate is " << phyMode.getRate());

    // Packet error probability
    double P_per = 1.0 - pow(1.0 - P_u, static_cast<double>(commandPoolSize + dataSize));
    assure(P_per >= 0.0 and P_per <= 1.0,
           "Error computing P_per to " << P_per);

    MESSAGE_BEGIN(NORMAL, logger, m, "New compound with SNR " << sinr);
    m << " len " << commandPoolSize + dataSize;
    m << " phyMode " << phyMode;
    m << " ser " << P_ser;
    m << " raw ber " << P_ber;
    m << " ber " << P_u;
    m << " per " << P_per;
    MESSAGE_END();

    emc->local.per = P_per;
}

void ErrorModelling::processOutgoing(const wns::ldk::CompoundPtr& /*compound*/)
{

}

double
ErrorModelling::Pd(double p, double d) const
{
    // Chernoff - Approximation
    return (pow(4*p*(1-p), d/2));
}

double
ErrorModelling::Pu12(double rawBer) const
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
ErrorModelling::Pu23(double rawBer) const
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
ErrorModelling::Pu34(double rawBer) const
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
ErrorModelling::Pu56(double rawBer) const
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
ErrorModelling::Q(double x) const
{
    return( erfc(x/sqrt(2.0)) /  2.0);
}
