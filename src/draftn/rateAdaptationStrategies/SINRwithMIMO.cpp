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

#include <WIFIMAC/draftn/rateAdaptationStrategies/SINRwithMIMO.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

using namespace wifimac::draftn::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(SINRwithMIMO,
                                     wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy, 
                                     "SINRwithMIMO",
                                     wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategyCreator);

SINRwithMIMO::SINRwithMIMO(
    const wns::pyconfig::View& _config,
    wifimac::management::PERInformationBase* _per,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    OpportunisticwithMIMO(_config, _per, _manager, _phyUser, _logger),
    retransmissionLQMReduction(_config.get<double>("retransmissionLQMReduction")),
    logger(_logger)
{
    friends.phyUser = _phyUser;
    friends.manager = _manager;
    curSpatialStreams = 1;
}

wifimac::convergence::PhyMode
SINRwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions, const wns::Ratio lqm)
{
    unsigned int numTx = friends.manager->getNumAntennas();
    unsigned int numRx = 1;
    if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(receiver, "numAntennas"))
    {
        numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(receiver, "numAntennas");
    }

    // Reduce lqm by 3dB for every retransmission
    wns::Ratio myLQM = wns::Ratio::from_dB(lqm.get_dB() - (numTransmissions-1)*retransmissionLQMReduction);

    // find the number of streams which maximizes the number of data bits per
    // symbol
    unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;
    int bestDBPS = 0;
    wifimac::convergence::PhyMode bestPM;
    unsigned int bestNumSS = 0;
    unsigned int numSS = maxNumSS;

    while(numSS > 0)
    {
        wns::Ratio postSINR = lqm
            + friends.phyUser->getExpectedPostSINRFactor(numSS, numRx);
        if(friends.phyUser->getPhyModeProvider()->getMinSINR() < postSINR)
        {
            wifimac::convergence::PhyMode pm = friends.phyUser->getPhyModeProvider()->getDefaultPhyMode();
            pm.setMCS(friends.phyUser->getPhyModeProvider()->getMCS(postSINR));
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
        m << " transmissions: " << numTransmissions;
        m << " numTx: " << numTx;
        m << " numRx: " << numRx;
        m << " postSINR with 1 stream: " << lqm;
        m << " too bad for any transmission, selecting lowest phy mode";
        MESSAGE_END();
        bestPM = friends.phyUser->getPhyModeProvider()->getDefaultPhyMode();
        bestPM.setNumberOfSpatialStreams(1);
        return(bestPM);
    }

    MESSAGE_BEGIN(NORMAL, *logger, m, "RA getPhyMode with");
    m << " lqm: " << lqm;
    m << " transmissions: " << numTransmissions;
    m << " numTx: " << numTx;
    m << " numRx: " << numRx;
    m << " -> numSS: " << bestNumSS;
    m << " postSINR: " << lqm
        + friends.phyUser->getExpectedPostSINRFactor(bestNumSS, numRx);
    m << " -> " << bestPM;
    MESSAGE_END();

    return(bestPM);
}

wifimac::convergence::PhyMode
SINRwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions)
{
    return(OpportunisticwithMIMO::getPhyMode(receiver, numTransmissions));
}
