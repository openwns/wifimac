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
    this->setModulation(modulation);
    this->setCodingRate(codingRate);
}

MCS::MCS(const wifimac::management::protocolCalculatorPlugins::ConfigGetter& config):
    modulation(config.get<std::string>("modulation", "s")),
    codingRate(config.get<std::string>("codingRate", "s")),
    minSINR(wns::Ratio::from_dB(0.0)),
    index(0),
    nominator(1),
    denominator(1)
{
    std::istringstream os(config.get<std::string>("minSINR", "s"));
    os >> minSINR;

    this->setModulation(modulation);
    this->setCodingRate(codingRate);
}

void MCS::setModulation(const std::string& modulation)
{
    assure(modulation == "BPSK" or modulation == "QPSK" or modulation == "QAM16" or modulation == "QAM64",
           "Unknown modulation" << modulation);

    if(modulation == "QPSK")
    {
        nominator = 2;
    }
    if(modulation == "QAM16")
    {
        nominator = 4;
    }
    if(modulation == "QAM64")
    {
        nominator = 6;
    }
}

void MCS::setCodingRate(const std::string codingRate)
{
    assure(codingRate == "1/2" or codingRate == "2/3" or codingRate == "3/4" or codingRate == "5/6",
           "Unknown coding rate" << codingRate);

    if(codingRate == "1/2")
    {
        denominator = 2;
    }
    if(codingRate == "2/3")
    {
        nominator *= 2;
        denominator = 3;
    }
    if(codingRate == "3/4")
    {
        nominator *= 3;
        denominator = 4;
    }
    if(codingRate == "5/6")
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
    mcs(),
    numberOfSpatialStreams(0),
    numberOfDataSubcarriers(0),
    plcpMode("ERROR"),
    guardIntervalDuration(0)
{}

PhyMode::PhyMode(const wns::pyconfig::View& config) :
    mcs(config),
    numberOfSpatialStreams(config.get<unsigned int>("numberOfSpatialStreams")),
    numberOfDataSubcarriers(config.get<unsigned int>("numberOfDataSubcarriers")),
    plcpMode(config.get<std::string>("plcpMode")),
    guardIntervalDuration(config.get<wns::simulator::Time>("guardIntervalDuration"))
{
    assure(numberOfSpatialStreams >= 1,
           "cannot have less than 1 spatial stream");
    assure(plcpMode == "Basic" or plcpMode == "HT-Mix" or plcpMode == "HT-GF",
           "Unknown plcpMode");
    assure(guardIntervalDuration == 0.8e-6 or guardIntervalDuration == 0.4e-6,
           "Unknown guard interval");
    assure(numberOfDataSubcarriers > 0,
           "cannot have less than 1 data subcarriers");
}

PhyMode::PhyMode(const wifimac::management::protocolCalculatorPlugins::ConfigGetter& config) :
    mcs(config),
    numberOfSpatialStreams(config.get<unsigned int>("numberOfSpatialStreams", "I")),
    numberOfDataSubcarriers(config.get<unsigned int>("numberOfDataSubcarriers", "I")),
    plcpMode(config.get<std::string>("plcpMode", "s")),
    guardIntervalDuration(config.get<wns::simulator::Time>("guardIntervalDuration", "d"))
{
    assure(numberOfSpatialStreams >= 1,
           "cannot have less than 1 spatial stream");
    assure(plcpMode == "Basic" or plcpMode == "HT-Mix" or plcpMode == "HT-GF",
           "Unknown plcpMode");
    assure(guardIntervalDuration == 0.8e-6 or guardIntervalDuration == 0.4e-6,
           "Unknown guard interval");
    assure(numberOfDataSubcarriers > 0,
           "cannot have less than 1 data subcarriers");
}

Bit PhyMode::getDataBitsPerSymbol() const
{
    assure(mcs.modulation != "ERROR",
           "modulation not set");
    assure(mcs.codingRate != "ERROR",
           "codingRate not set");
    assure(numberOfSpatialStreams > 0,
           "number of spatial streams not set");
    assure(numberOfDataSubcarriers > 0,
           "cannot have less than 1 data subcarriers");

    return(numberOfDataSubcarriers * numberOfSpatialStreams * mcs.nominator / mcs.denominator);
}

bool PhyMode::operator <(const PhyMode& rhs) const
{
    return(this->getDataBitsPerSymbol() < rhs.getDataBitsPerSymbol());
}

bool PhyMode::operator ==(const PhyMode& rhs) const
{
    assure(mcs.modulation != "ERROR", "modulation not set in lhs");
    assure(mcs.codingRate != "ERROR", "codingRate not set in lhs");
    assure(numberOfSpatialStreams > 0, "number of spatial streams not set in lhs");
    assure(numberOfDataSubcarriers > 0, "number of DataSubcarriers not set in lhs");
    assure(plcpMode != "ERROR", "plcpMode not set in lhs");

    assure(rhs.mcs.modulation != "ERROR", "modulation not set in rhs");
    assure(rhs.mcs.codingRate != "ERROR", "codingRate not set in rhs");
    assure(rhs.numberOfSpatialStreams > 0, "number of spatial streams not set in rhs");
    assure(rhs.numberOfDataSubcarriers > 0, "number of DataSubcarriers not set in rhs");
    assure(rhs.plcpMode != "ERROR", "plcpMode not set in rhs");


    return((mcs == rhs.mcs) and
           (numberOfSpatialStreams == rhs.numberOfSpatialStreams) and
           (numberOfDataSubcarriers == rhs.numberOfDataSubcarriers) and
           (plcpMode == rhs.plcpMode));
}

bool PhyMode::operator !=(const PhyMode& rhs) const
{
    return(not this->operator==(rhs));
}
