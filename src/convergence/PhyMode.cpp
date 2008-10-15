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

    wns::pyconfig::View sinr2berView = config.getView("sinr2ber");
	for(int i=0; i < sinr2berView.len("mapping"); ++i)
    {
		wns::pyconfig::View curSinr2BerView = sinr2berView.getView("mapping", i);
		wns::Ratio sinr = wns::Ratio::from_dB(curSinr2BerView.get<double>("sinr"));
		sinr2ber_[sinr] = curSinr2BerView.get<double>("ber");
    }
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

double PhyMode::getPer(wns::Power rss, wns::Ratio sinr, Bit length) const
{
    if (constFrameSize_ > 0)
	{
		length = constFrameSize_;
	}

    assure(dataBitsPerSymbol_ > 0, "dataBitsPerSymbol <= 0! Did you initialize this PhyMode?");
    assure(length >= 0, "Cannot have negative length");

	// Minimum length is one Symbol
	if(length < dataBitsPerSymbol_)
    {
		length = dataBitsPerSymbol_;
    }

	// if the model includes an RSS threshold, apply it (--> old WARP2 model)
    if((minRSS_ != wns::Power::from_dBm(0)) && (rss < minRSS_))
    {
        MESSAGE_SINGLE(NORMAL, logger_, length << " with rss " << rss << " < " << minRSS_ << " -> per = 1.0");
		return(1.0);
    }

	// Get the next higher and lower sinr-points which are closest to the actual sinr
	std::map<wns::Ratio, double>::const_iterator it = sinr2ber_.begin();
	// SINR is below the lowest declared point -> PER = 1
    if (sinr < it->first)
    {
		MESSAGE_SINGLE(NORMAL, logger_, length << "b @ " << sinr << " -> per = 1.0");
		return (1.0);
    }

    wns::Ratio sinrLower;
    wns::Ratio sinrUpper;
    double berLower = -1.0;
    double berUpper = -1.0;

    for (; (it != sinr2ber_.end()) && (it->first <= sinr); ++it)
    {
		sinrLower = it->first;
		berLower = it->second;

		std::map<wns::Ratio, double>::const_iterator tmp = it;
		tmp++;

		if (tmp == sinr2ber_.end())
		{
			sinrUpper = sinrUpper + wns::Ratio::from_dB(1.0);
			berUpper = berLower;
		}
		else
		{
			sinrUpper = tmp->first;
			berUpper = tmp->second;
		}
    }

    assure(berLower >= 0.0, "Not properly initialized");
    assure(berUpper >= 0.0, "Not properly initialized");

    double ber;
    if (berUpper == berLower)
    {
		ber = berLower;
    }
    else
    {
        if (sinrLower < wns::Ratio::from_dB(1E-10))
		{
			sinrLower = wns::Ratio::from_dB(1E-10);
		}
		if (sinrUpper < wns::Ratio::from_dB(1E-10))
		{
			sinrUpper = wns::Ratio::from_dB(1E-10);
		}

		// logarithmic interpolation
		double berLowerLog = log(berLower)/log(10.0);
		double berUpperLog = log(berUpper)/log(10.0);
		double berLog = berLowerLog + ((sinrUpper - sinrLower).convertForAveraging()) * (berUpperLog - berLowerLog);
		ber = pow(10.0, berLog);
    }

    // Calculate PER
    // TODO: BER to PER using simple Bernoulli does not take into account the
    // Viterbi Decoder...
    double per = 1 - pow((1-ber), length);

    if (constFrameSize_ > 0)
    {
        MESSAGE_SINGLE(NORMAL, logger_, length << "b (const) @ " << sinr << " -> ber = " << ber << " -> per = " << per);
    }
    else
    {
		MESSAGE_SINGLE(NORMAL, logger_, length << "b @ " << sinr << " -> ber = " << ber << " -> per = " << per);
    }

    return(per);
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
