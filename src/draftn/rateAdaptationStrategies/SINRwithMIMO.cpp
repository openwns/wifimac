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
    wns::service::dll::UnicastAddress _receiver,
    wifimac::management::PERInformationBase* _per,
    wifimac::management::SINRInformationBase* _sinr,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    PERwithMIMO(_config, _receiver, _per, _sinr, _manager, _phyUser, _logger),
    sinr(dynamic_cast<wifimac::draftn::SINRwithMIMOInformationBase*>(_sinr)),
    singleStreamRA(_config, _receiver, _per, _sinr, _manager, _phyUser, _logger),
    retransmissionLQMReduction(_config.get<double>("retransmissionLQMReduction")),
    myReceiver(_receiver),
    logger(_logger)
{
    friends.phyUser = _phyUser;
    friends.manager = _manager;
    curSpatialStreams = 1;
}

wifimac::convergence::PhyMode
SINRwithMIMO::getPhyMode(size_t numTransmissions, const wns::Ratio lqm) const
{
    unsigned int numTx = friends.manager->getNumAntennas();
    unsigned int numRx = 1;
    if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(myReceiver, "numAntennas"))
    {
        numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(myReceiver, "numAntennas");
    }

    unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;

    MESSAGE_BEGIN(NORMAL, *logger, m, "RA");
    m << " to receiver " << myReceiver;
    m << " lqm: " << lqm;
    m << " transmissions: " << numTransmissions;
    m << " numTx: " << numTx;
    m << " numRx: " << numRx;
    m << " maxNumSS: " << maxNumSS;
    MESSAGE_END();

    wifimac::convergence::PhyMode bestPM = friends.phyUser->getPhyModeProvider()->getDefaultPhyMode();
    bestPM.setUniformMCS(bestPM.getSpatialStreams()[0], 1);
    bool foundBestPM = false;

    for(unsigned int numSS = maxNumSS; numSS >= 1; --numSS)
    {
        if(sinr->knowsPeerFactor(myReceiver, numSS))
        {
            wifimac::convergence::PhyMode pm = friends.phyUser->getPhyModeProvider()->getDefaultPhyMode();
            std::vector<wifimac::convergence::MCS> mcs;
            bool reduceAntennas = false;

            std::vector<wns::Ratio> sinrFactor = sinr->getPeerFactor(myReceiver, numSS);

#ifndef WNS_NO_LOGGING
            MESSAGE_BEGIN(NORMAL, *logger, m, "peerFactor for ");
            m << numSS << " streams is known:";
            for(std::vector<wns::Ratio>::iterator it = sinrFactor.begin();
                it != sinrFactor.end();
                ++it)
            {
                m << " " << *it;
            }
            MESSAGE_END();
#endif
            for(std::vector<wns::Ratio>::iterator it = sinrFactor.begin();
                it != sinrFactor.end() and (not reduceAntennas);
                ++it)
            {
                wns::Ratio streamLQM = lqm + *it;
                if(streamLQM < friends.phyUser->getPhyModeProvider()->getMinSINR())
                {
                    MESSAGE_SINGLE(NORMAL, *logger, "lqm of " << streamLQM << " is too small -> reduce antennas");
                    reduceAntennas = true;
                }
                else
                {
                    wifimac::convergence::PhyMode singleStreamPM = singleStreamRA.getPhyMode(numTransmissions, streamLQM);
                    MESSAGE_SINGLE(NORMAL, *logger, "lqm for stream is " << streamLQM << " -> " << singleStreamPM);
                    mcs.push_back(singleStreamPM.getSpatialStreams()[0]);
                }
            }
            if(not reduceAntennas)
            {
                wifimac::convergence::PhyMode pm = friends.phyUser->getPhyModeProvider()->getDefaultPhyMode();
                pm.setSpatialStreams(mcs);
                if(pm.getDataBitsPerSymbol() > bestPM.getDataBitsPerSymbol())
                {
                    MESSAGE_SINGLE(NORMAL, *logger, "Found phyMode: " << pm << " with " << pm.getDataBitsPerSymbol() << "dbps -> new candidate");
                    bestPM = pm;
                    foundBestPM = true;
                }
                else
                {
                    MESSAGE_SINGLE(NORMAL, *logger, "Found phyMode: " << pm << ", but only " << pm.getDataBitsPerSymbol() << "dbps");
                }
            }
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, *logger, "peerFactor for " << numSS << " streams is not known");
        }
    }

    MESSAGE_SINGLE(NORMAL, *logger, "RA selects " << bestPM);
    return(bestPM);
}

wifimac::convergence::PhyMode
SINRwithMIMO::getPhyMode(size_t numTransmissions) const
{
    if(sinr->knowsPeerSINR(myReceiver))
    {
        return(getPhyMode(numTransmissions, sinr->getPeerSINR(myReceiver)));
    }
    else
    {
        return(PERwithMIMO::getPhyMode(numTransmissions));
    }
}
