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

#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/Assure.hpp>
#include <WNS/Exception.hpp>

using namespace wifimac::convergence;

MCS::MCS():
    modulation("ERROR"),
    codingRate("ERROR"),
    minSINR(),
    index(0)
{}

MCS::MCS(const MCS& other):
    modulation(other.modulation),
    codingRate(other.codingRate),
    nominator(other.nominator),
    denominator(other.denominator),
    index(other.index),
    minSINR(other.minSINR)
{}

MCS::MCS(const wns::pyconfig::View& config) :
    modulation(config.get<std::string>("modulation")),
    codingRate(config.get<std::string>("codingRate")),
    minSINR(config.get<wns::Ratio>("minSINR")),
    index(0),
    nominator(1),
    denominator(1)
{
    this->setMCS(modulation, codingRate);
}

MCS::MCS(const wifimac::management::protocolCalculatorPlugins::ConfigGetter& config):
    modulation(config.get<char*>("modulation", "s")),
    codingRate(config.get<char*>("codingRate", "s")),
    minSINR(wns::Ratio::from_dB(0.0)),
    index(0),
    nominator(1),
    denominator(1)
{
    std::istringstream os(config.get<char*>("minSINR", "s"));
    os >> minSINR;

    this->setMCS(modulation, codingRate);
}

std::string MCS::getModulation() const
{
    return this->modulation;
}

std::string MCS::getRate() const
{
    return this->codingRate;
}

void MCS::setMCS(const std::string& newModulation, const std::string& newCodingRate)
{
    assure(newModulation == "BPSK" or
           newModulation == "QPSK" or
           newModulation == "QAM16" or
           newModulation == "QAM64",
           "Unknown new modulation" << newModulation);
    assure(newCodingRate == "1/2" or
           newCodingRate == "2/3" or
           newCodingRate == "3/4" or
           newCodingRate == "5/6",
           "Unknown new coding rate" << newCodingRate);

    codingRate = newCodingRate;
    modulation = newModulation;

    if(modulation == "BSPK")
    {
        nominator = 1;
    }
    else if(modulation == "QPSK")
    {
        nominator = 2;
    }
    else if(modulation == "QAM16")
    {
        nominator = 4;
    }
    else if(modulation == "QAM64")
    {
        nominator = 6;
    }

    if(codingRate == "1/2")
    {
        denominator = 2;
    }
    else if(codingRate == "2/3")
    {
        nominator *= 2;
        denominator = 3;
    }
    else if(codingRate == "3/4")
    {
        nominator *= 3;
        denominator = 4;
    }
    else if(codingRate == "5/6")
    {
        nominator *= 5;
        denominator = 6;
    }
}


bool MCS::operator <(const MCS& rhs) const
{
    return(minSINR < rhs.minSINR);
}

bool MCS::operator ==(const MCS& rhs) const
{
    return(minSINR == rhs.minSINR and
           nominator == rhs.nominator and
           denominator == rhs.denominator);
}

bool MCS::operator !=(const MCS& rhs) const
{
    return(minSINR != rhs.minSINR or
           nominator != rhs.nominator or
           denominator != rhs.denominator);
}

PhyMode::PhyMode():
    spatialStreams(),
    numberOfDataSubcarriers(0),
    plcpMode("ERROR"),
    guardIntervalDuration(0)
{}

PhyMode::PhyMode(const wns::pyconfig::View& config) :
    spatialStreams(),
    numberOfDataSubcarriers(config.get<unsigned int>("numberOfDataSubcarriers")),
    plcpMode(config.get<std::string>("plcpMode")),
    guardIntervalDuration(config.get<wns::simulator::Time>("guardIntervalDuration"))
{
    assure(config.get<int>("len(spatialStreams)") > 0,
           "cannot have less than 1 spatial stream");
    assure(plcpMode == "Basic" or plcpMode == "HT-Mix" or plcpMode == "HT-GF",
           "Unknown plcpMode");
    assure(guardIntervalDuration == 0.8e-6 or guardIntervalDuration == 0.4e-6,
           "Unknown guard interval");
    assure(numberOfDataSubcarriers > 0,
           "cannot have less than 1 data subcarriers");

    for (int i = 0; i < config.get<int>("len(spatialStreams)"); ++i)
    {
        std::string s = "spatialStreams[" + wns::Ttos(i) + "]";
        spatialStreams.push_back(MCS(config.get(s)));
    }
}

PhyMode::PhyMode(const wifimac::management::protocolCalculatorPlugins::ConfigGetter& config) :
    spatialStreams(),
    numberOfDataSubcarriers(config.get<unsigned int>("numberOfDataSubcarriers", "I")),
    plcpMode(config.get<std::string>("plcpMode", "s")),
    guardIntervalDuration(config.get<wns::simulator::Time>("guardIntervalDuration", "d"))
{
    assure(config.get<int>("len(spatialStreams)", "I") > 0,
           "cannot have less than 1 spatial stream");
    assure(plcpMode == "Basic" or plcpMode == "HT-Mix" or plcpMode == "HT-GF",
           "Unknown plcpMode");
    assure(guardIntervalDuration == 0.8e-6 or guardIntervalDuration == 0.4e-6,
           "Unknown guard interval");
    assure(numberOfDataSubcarriers > 0,
           "cannot have less than 1 data subcarriers");

    for (int i = 0; i < config.get<int>("len(spatialStreams)", "I"); ++i)
    {
        std::string s = "spatialStreams[" + wns::Ttos(i) + "]";
        spatialStreams.push_back(MCS(config.get(s.c_str())));
    }
}

Bit PhyMode::getDataBitsPerSymbol() const
{
    unsigned int dbps = 0;
    assure(spatialStreams.size() > 0,
           "ERROR: No spatial streams");
    assure(numberOfDataSubcarriers > 0,
           "cannot have less than 1 data subcarriers");

    for (std::vector<MCS>::const_iterator it = spatialStreams.begin();
         it != spatialStreams.end();
         ++it)
    {
        dbps += (numberOfDataSubcarriers * it->nominator / it->denominator);
    }
    return(dbps);
}

void PhyMode::setUniformMCS(const MCS& mcs, unsigned int numSS)
{
    this->spatialStreams.assign(numSS, mcs);
}

void PhyMode::setSpatialStreams(const std::vector<MCS>& ss)
{
    assure(ss.size() > 0, "ERROR: No spatial streams");

    this->spatialStreams.clear();
    for(std::vector<MCS>::const_iterator it = ss.begin();
        it != ss.end();
        ++it)
    {
        this->spatialStreams.push_back(*it);
    }
}

bool PhyMode::operator <(const PhyMode& rhs) const
{
    return(this->getDataBitsPerSymbol() < rhs.getDataBitsPerSymbol());
}

bool PhyMode::operator ==(const PhyMode& rhs) const
{
    assure(spatialStreams.size() > 0, "number of spatial streams not set in lhs");
    assure(numberOfDataSubcarriers > 0, "number of DataSubcarriers not set in lhs");
    assure(plcpMode != "ERROR", "plcpMode not set in lhs");

    assure(rhs.spatialStreams.size() > 0, "number of spatial streams not set in rhs");
    assure(rhs.numberOfDataSubcarriers > 0, "number of DataSubcarriers not set in rhs");
    assure(rhs.plcpMode != "ERROR", "plcpMode not set in rhs");

    if(spatialStreams.size() != rhs.spatialStreams.size())
    {
        return false;
    }

    std::vector<MCS>::const_iterator it = spatialStreams.begin();
    std::vector<MCS>::const_iterator itRHS = rhs.spatialStreams.begin();
    for(;
        (it != spatialStreams.end()) and (itRHS != rhs.spatialStreams.end());
        ++it, ++itRHS)
    {
        if((*it) != (*itRHS))
        {
            return false;
        }
    }
    return((numberOfDataSubcarriers == rhs.numberOfDataSubcarriers) and
           (plcpMode == rhs.plcpMode));
}

bool PhyMode::operator !=(const PhyMode& rhs) const
{
    return(not this->operator==(rhs));
}
