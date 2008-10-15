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

#include <WIFIMAC/convergence/PhyModeProvider.hpp>

#include <stdio.h>

using namespace wifimac::convergence;

PhyModeProvider::PhyModeProvider(const wns::pyconfig::View& config):
	numPhyModes(config.get<int>("myConfig.numPhyModes")),
    symbolDuration(config.get<wns::simulator::Time>("myConfig.symbolDuration")),
    switchingPointOffset(config.get<wns::Ratio>("myConfig.switchingPointOffset"))
{
	assure(numPhyModes > 0, "There must be at least one PhyMode!");
	assure(numPhyModes < 1000, "There can be at most 1000 PhyModes!");
	char bf[12];

	for (int id=0; id < numPhyModes; ++id)
	{
		sprintf(bf, "PhyMode%d", id);
		wns::pyconfig::View configview = config.getView(bf);
		PhyMode pm(configview, id+1);
		id2phyMode.insert(id, pm);
		phyMode2id.insert(pm, id);
	}

	wns::pyconfig::View configview = config.getView("PhyModePreamble");
	PhyMode pm(configview, 0);
	preamblePhyMode = pm;
    assure(preamblePhyMode.getNumSymbols(1) > 0, "wrong preamblePhyMode");
}

PhyMode PhyModeProvider::rateUp(PhyMode pm) const
{
	return(rateUp(phyMode2id.find(pm)));
}

PhyMode PhyModeProvider::rateUp(int id) const
{

	if (id == numPhyModes-1)
		return(id2phyMode.find(id));

	return(id2phyMode.find(id+1));
}

PhyMode PhyModeProvider::rateDown(PhyMode pm) const
{
	return(rateDown(phyMode2id.find(pm)));
}

PhyMode PhyModeProvider::rateDown(int id) const
{
	if (id == 0)
		return(id2phyMode.find(id));

	return(id2phyMode.find(id-1));
}

PhyMode PhyModeProvider::getLowest() const
{
	return(id2phyMode.find(0));
}

int PhyModeProvider::getLowestId() const
{
	return(0);
}

int PhyModeProvider::getHighestId() const
{
	return(numPhyModes-1);
}

PhyMode PhyModeProvider::getHighest() const
{
	return(id2phyMode.find(numPhyModes-1));
}

PhyMode PhyModeProvider::getPreamblePhyMode() const
{
	return(preamblePhyMode);
}

PhyMode PhyModeProvider::getPhyMode(const int id) const
{
	assure(id >= 0, "id must be higher than 0!");
	assure(id < numPhyModes, "id must be lower than numPhyModes!");

	return(id2phyMode.find(id));
}

wns::simulator::Time PhyModeProvider::getSymbolDuration() const
{
	return(symbolDuration);
}

int PhyModeProvider::getPhyModeId(wns::Ratio sinr) const
{
    if(this->getMinSINR() > sinr)
    {
        return(-1);
    }
    int optId = this->getLowestId();

    for (int id=0; id < numPhyModes; ++id)
    {
        PhyMode cand = id2phyMode.find(id);
        if((cand.getMinSINR() < (sinr-switchingPointOffset)) and (getPhyMode(optId) < cand))
        {
            optId = id;
        }
    }
    return(optId);
}


PhyMode PhyModeProvider::getPhyMode(wns::Ratio sinr) const
{
    return(getPhyMode(getPhyModeId(sinr)));
}

wns::Ratio PhyModeProvider::getMinSINR() const
{
    return(this->getLowest().getMinSINR() + switchingPointOffset);
}
