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

#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/Assure.hpp>
#include <WNS/Exception.hpp>

using namespace wifimac::convergence;

PhyMode::PhyMode():
    dataBitsPerSymbol_(0),
    modulation_("ERROR"),
    codingRate_("ERROR"),
    numberOfSpatialStreams_(1),
    logger_(),
    index(0)
{}

PhyMode::PhyMode(const wns::pyconfig::View& config, int index_) :
    logger_(config.get<wns::pyconfig::View>("logger")),
    index(index_)
{
    configPhyMode(config);
}

void PhyMode::configPhyMode(const wns::pyconfig::View& config)
{
    dataBitsPerSymbol_ = config.get<Bit>("dataBitsPerSymbol");
    modulation_ = config.get<std::string>("modulation");
    codingRate_ = config.get<std::string>("codingRate");
    constFrameSize_ = config.get<Bit>("constFrameSizeBits");
    minRSS_ = config.get<wns::Power>("minRSS");
    minSINR = config.get<wns::Ratio>("minSINR");
    numberOfSpatialStreams_ = 1;

    MESSAGE_BEGIN(VERBOSE, logger_, m, "New PhyMode: ");
    m << modulation_  << "-" << codingRate_;
    m << " (" << dataBitsPerSymbol_ << "DBPS)";
    MESSAGE_END();
}

int PhyMode::getNumSymbols(Bit length) const
{
    if(dataBitsPerSymbol_ <= 0)
    {
        throw wns::Exception("dataBitsPerSymbol <= 0! Did you initialize this PhyMode?");
    }

    int numSymbols = static_cast<int>(ceil(static_cast<double>(length)/static_cast<double>(dataBitsPerSymbol_*numberOfSpatialStreams_)));
    MESSAGE_SINGLE(VERBOSE, logger_, length << "b --> " << numSymbols << " Symbols");

    return numSymbols;
}

Bit PhyMode::getDataBitsPerSymbol() const
{
    return(dataBitsPerSymbol_*numberOfSpatialStreams_);
}

void PhyMode::setNumberOfSpatialStreams(unsigned int nss)
{
    this->numberOfSpatialStreams_ = nss;
}

unsigned int PhyMode::getNumberOfSpatialStreams() const
{
    return(this->numberOfSpatialStreams_);
}

bool PhyMode::operator <(const PhyMode& rhs) const
{
    assure(dataBitsPerSymbol_ > 0, "dataBitsPerSymbol <= 0 in lhs");
    assure(rhs.dataBitsPerSymbol_ > 0, "dataBitsPerSymbol <= 0 in rhs");

    return(dataBitsPerSymbol_ < rhs.dataBitsPerSymbol_);
}

bool PhyMode::operator ==(const PhyMode& rhs) const
{
    assure(dataBitsPerSymbol_ > 0, "dataBitsPerSymbol <= 0 in lhs");
    assure(rhs.dataBitsPerSymbol_ > 0, "dataBitsPerSymbol <= 0 in rhs");

    return(dataBitsPerSymbol_ == rhs.dataBitsPerSymbol_);
}

bool PhyMode::operator !=(const PhyMode& rhs) const
{
    return(not this->operator==(rhs));
}
