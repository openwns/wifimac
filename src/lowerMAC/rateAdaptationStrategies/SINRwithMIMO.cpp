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

#include <WIFIMAC/lowerMAC/rateAdaptationStrategies/SINRwithMIMO.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

using namespace wifimac::lowerMAC::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(SINRwithMIMO, IRateAdaptationStrategy, "SINRwithMIMO", IRateAdaptationStrategyCreator);

SINRwithMIMO::SINRwithMIMO(
    wifimac::management::PERInformationBase* _per,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    SINR(_per, _manager, _phyUser, _logger),
    logger(_logger)
{
    friends.phyUser = _phyUser;
    friends.manager = _manager;
    curSpatialStreams = 1;
}
//1.0062461
wifimac::convergence::PhyMode
SINRwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver, const wns::Ratio lqm)
{
    if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(receiver, "numAntennas"))
    {
        // find the number of streams which maximizes the number of data bits
        // per symbol
        unsigned int numTx = friends.manager->getNumAntennas();
        unsigned int numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(receiver, "numAntennas");
        unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;
        int bestDBPS = 0;
        wifimac::convergence::PhyMode bestPM;
        unsigned int bestNumSS = 0;
        unsigned int numSS = maxNumSS;

        while(numSS > 0)
        {
            wns::Ratio postSINR = lqm
                - friends.phyUser->getExpectedPostSINRFactor(1, numRx)
                + friends.phyUser->getExpectedPostSINRFactor(numSS, numRx);
            if(friends.phyUser->getPhyModeProvider()->getMinSINR() < postSINR)
            {
                wifimac::convergence::PhyMode pm = friends.phyUser->getPhyModeProvider()->getPhyMode(postSINR);
                pm.setNumberOfSpatialStreams(numSS);

                // check performance of numSS
                if(pm.getDataBitsPerSymbol() > bestDBPS)
                {
                    bestDBPS = pm.getDataBitsPerSymbol();
                    bestNumSS = numSS;
                    bestPM = pm;
                }
            }
            --numSS;
        }

        if(bestNumSS == 0)
        {
            MESSAGE_BEGIN(NORMAL, *logger, m, "RA");
            m << " to receiver " << receiver;
            m << " lqm: " << lqm;
            m << " numTx: " << numTx;
            m << " numRx: " << numRx;
            m << " postSINR with 1 stream: " << lqm;
            m << " too bad for any transmission, selecting lowest phy mode";
            MESSAGE_END();
            bestPM = friends.phyUser->getPhyModeProvider()->getLowest();
            bestPM.setNumberOfSpatialStreams(1);
            return(bestPM);
        }

        MESSAGE_BEGIN(NORMAL, *logger, m, "RA getPhyMode with");
        m << " lqm: " << lqm;
        m << " numTx: " << numTx;
        m << " numRx: " << numRx;
        m << " -> numSS: " << bestNumSS;
        m << " postSINR: " << lqm
            - friends.phyUser->getExpectedPostSINRFactor(1, numRx)
            + friends.phyUser->getExpectedPostSINRFactor(bestNumSS, numRx);
        m << " -> " << bestPM;
        MESSAGE_END();

        return(bestPM);
    }
    else
    {
        return(SINR::getPhyMode(receiver, lqm));
    }
}

wifimac::convergence::PhyMode
SINRwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver)
{
    return(SINR::getPhyMode(receiver));
}
