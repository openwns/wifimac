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

#include <WIFIMAC/draftn/rateAdaptationStrategies/OpportunisticwithMIMO.hpp>
#include <WIFIMAC/management/VirtualCapabilityInformationBase.hpp>

#include <algorithm>

using namespace wifimac::draftn::rateAdaptationStrategies;

STATIC_FACTORY_REGISTER_WITH_CREATOR(OpportunisticwithMIMO,
                                     wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy,
                                     "OpportunisticwithMIMO",
                                     wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategyCreator);

OpportunisticwithMIMO::OpportunisticwithMIMO(
    const wns::pyconfig::View& _config,
    wifimac::management::PERInformationBase* _per,
    wifimac::management::SINRInformationBase* _sinr,
    wifimac::lowerMAC::Manager* _manager,
    wifimac::convergence::PhyUser* _phyUser,
    wns::logger::Logger* _logger):
    wifimac::lowerMAC::rateAdaptationStrategies::IRateAdaptationStrategy(_config, _per, _sinr, _manager, _phyUser, _logger),
    per(_per),
    perForGoingDown(_config.get<double>("perForGoingDown")),
    perForGoingUp(_config.get<double>("perForGoingUp")),
    phyModeIncreaseOnAntennaDecrease(_config.get<unsigned int>("phyModeIncreaseOnAntennaDecrease")),
    phyModeDecreaseOnAntennaIncrease(_config.get<unsigned int>("phyModeDecreaseOnAntennaIncrease")),
    logger(_logger)
{
    friends.phyUser = _phyUser;
    friends.manager = _manager;
    curPhyMode = _config.getView("initialPhyMode");
}

wifimac::convergence::PhyMode
OpportunisticwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions, const wns::Ratio /*lqm*/) const
{
    return(this->getPhyMode(receiver, numTransmissions));
}

wifimac::convergence::PhyMode
OpportunisticwithMIMO::getPhyMode(const wns::service::dll::UnicastAddress receiver, size_t numTransmissions) const
{
    unsigned int numTx = friends.manager->getNumAntennas();
    unsigned int numRx = 1;
    if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(receiver, "numAntennas"))
    {
        numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(receiver, "numAntennas");
    }
    unsigned int maxNumSS = (numTx < numRx) ? numTx : numRx;

    wifimac::convergence::PhyMode pm = curPhyMode;

    if(not per->knowsPER(receiver))
    {
        assure(numTransmissions >= 1, "Must have at least one transmission");

        // if no information about the PER is available (due to a recent change),
        // we return the current phyMode

        return(pm);
    }

    double curPER = per->getPER(receiver);

    if(curPER > perForGoingDown)
    {
        reducePhyMode(pm, maxNumSS);
    }

    if(curPER < perForGoingUp)
    {
        // nearly all frames are successful -> go up
        increasePhyMode(pm, maxNumSS);
    }

    return(pm);
}

void
OpportunisticwithMIMO::reducePhyMode(wifimac::convergence::PhyMode& pm, unsigned int maxNumSS) const
{
    if(friends.phyUser->getPhyModeProvider()->hasLowestMCS(pm))
    {
        if(pm.getNumberOfSpatialStreams() > 1)
        {
            pm.setUniformMCS(pm.getSpatialStreams()[0], pm.getNumberOfSpatialStreams() - 1);
            for(int i = 0; i < phyModeIncreaseOnAntennaDecrease; ++i)
            {
                friends.phyUser->getPhyModeProvider()->mcsUp(pm);
            }
        }
    }
    else
    {
        friends.phyUser->getPhyModeProvider()->mcsDown(pm);
    }
}

void
OpportunisticwithMIMO::increasePhyMode(wifimac::convergence::PhyMode& pm, unsigned int maxNumSS) const
{
    // nearly all frames are successful -> go up
    if(friends.phyUser->getPhyModeProvider()->hasHighestMCS(pm))
    {
        if(pm.getNumberOfSpatialStreams() < maxNumSS)
        {
            pm.setUniformMCS(pm.getSpatialStreams()[0], pm.getNumberOfSpatialStreams() + 1);
            for(int i = 0; i < phyModeDecreaseOnAntennaIncrease; ++i)
            {
                friends.phyUser->getPhyModeProvider()->mcsDown(pm);
            }
        }
    }
    else
    {
        friends.phyUser->getPhyModeProvider()->mcsUp(pm);
    }
}



void
OpportunisticwithMIMO::setCurrentPhyMode(const wns::service::dll::UnicastAddress receiver,wifimac::convergence::PhyMode pm)
{

    if(curPhyMode != pm)
    {
	    
	unsigned int numTx = friends.manager->getNumAntennas();
	unsigned int numRx = 1;
	if(wifimac::management::TheVCIBService::Instance().getVCIB()->knows(receiver, "numAntennas"))
	{
		numRx = wifimac::management::TheVCIBService::Instance().getVCIB()->get<int>(receiver, "numAntennas");
	}
	if(per->knowsPER(receiver))
	{
	        MESSAGE_BEGIN(NORMAL, *logger, m, "RA");
	        m << " to receiver " << receiver;
	        m << " per: " << per->getPER(receiver);
	        m << " numTx: " << numTx;
	        m << " numRx: " << numRx;
	        m << " going from " << curPhyMode;
	        m << " to " << pm;
	        MESSAGE_END();

	        per->reset(receiver);
	}
	else
	{
	        MESSAGE_BEGIN(NORMAL, *logger, m, "RA");
	        m << " to receiver " << receiver;
	        m << " , no currently known PER, ";
	        m << " numTx: " << numTx;
	        m << " numRx: " << numRx;
	        m << " going from " << curPhyMode;
	        m << " to " << pm;
	        MESSAGE_END();
	}
        curPhyMode = pm;
    }
}
