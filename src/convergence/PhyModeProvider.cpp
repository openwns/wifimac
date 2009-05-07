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

#include <WIFIMAC/convergence/PhyModeProvider.hpp>

#include <stdio.h>

using namespace wifimac::convergence;

PhyModeProvider::PhyModeProvider(const wns::pyconfig::View& config) :
    switchingPointOffset(config.get<wns::Ratio>("switchingPointOffset"))
{
    assure(config.get<int>("len(MCSs)") > 0, "There must be at least one MCS!");

    char bf[15];

    for (int id=0; id < config.get<int>("len(MCSs)"); ++id)
    {
        sprintf(bf, "MCSs[%d]", id);
        wns::pyconfig::View configview = config.getView(bf);
        MCS mcs(configview);
        assure(sinr2mcs.find(mcs.getMinSINR()) == sinr2mcs.end(),
               "Cannot have two MCS with the same minSINR " << mcs.getMinSINR());
        sinr2mcs[mcs.getMinSINR()] = mcs;
    }

    unsigned int index = 0;
    for(std::map<wns::Ratio, MCS>::iterator it = sinr2mcs.begin();
        it != sinr2mcs.end();
        ++it)
    {
        it->second.setIndex(index);
        ++index;
    }

    wns::pyconfig::View configPreamble = config.getView("phyModePreamble");
    preamblePhyMode = PhyMode(configPreamble);

    wns::pyconfig::View configPhyMode = config.getView("defaultPhyMode");
    defaultPhyMode = PhyMode(configPhyMode);
}

void
PhyModeProvider::mcsUp(PhyMode& pm) const
{
    std::map<wns::Ratio, MCS>::const_iterator it = sinr2mcs.find(pm.getMCS().getMinSINR());
    ++it;
    if(it != sinr2mcs.end())
    {
        pm.setMCS(it->second);
    }
}

void
PhyModeProvider::mcsDown(PhyMode& pm) const
{
    std::map<wns::Ratio, MCS>::const_iterator it = sinr2mcs.find(pm.getMinSINR());
    if(it != sinr2mcs.begin())
    {
        --it;
        pm.setMCS(it->second);
    }
}

bool
PhyModeProvider::hasLowestMCS(const PhyMode& pm) const
{
    return(pm.getMCS() == sinr2mcs.begin()->second);
}


bool
PhyModeProvider::hasHighestMCS(const PhyMode& pm) const
{
    return(pm.getMCS() == (--sinr2mcs.end())->second);
}

PhyMode
PhyModeProvider::getPreamblePhyMode(PhyMode pmFrame) const
{
    PhyMode pm = preamblePhyMode;
    pm.setGuardIntervalDuration(pmFrame.getGuardIntervalDuration());
    pm.setNumberOfSpatialStreams(pmFrame.getNumberOfSpatialStreams());
    pm.setPreambleMode(pmFrame.getPreambleMode());

    return(pm);
}

PhyMode
PhyModeProvider::getDefaultPhyMode() const
{
    PhyMode pm = defaultPhyMode;
    return pm;
}

MCS
PhyModeProvider::getMCS(wns::Ratio sinr) const
{
    sinr = sinr - switchingPointOffset;

    if(sinr < sinr2mcs.begin()->second.getMinSINR())
    {
        return(sinr2mcs.begin()->second);
    }

    if(sinr > (--sinr2mcs.end())->second.getMinSINR())
    {
        return( (--sinr2mcs.end())->second);
    }

    std::map<wns::Ratio, MCS>::const_iterator it = sinr2mcs.lower_bound(sinr);
    --it;
    return(it->second);
}

wns::Ratio
PhyModeProvider::getMinSINR() const
{
    return(sinr2mcs.begin()->second.getMinSINR());
}
