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

#include <WIFIMAC/management/protocolCalculatorPlugins/FrameLength.hpp>

using namespace wifimac::management::protocolCalculatorPlugins;

FrameLength::FrameLength(const wns::pyconfig::View& config):
    macDataHdr(config.get<Bit>("macDataHdr")),
    macDataFCS(config.get<Bit>("macDataFCS")),
    amsdu_subhdr(config.get<Bit>("amsdu_subhdr")),
    ampdu_delimiter(config.get<Bit>("ampdu_delimiter")),
    service(config.get<Bit>("service")),
    tail(config.get<Bit>("tail")),
    ack(config.get<Bit>("ack")),
    rts(config.get<Bit>("rts")),
    cts(config.get<Bit>("cts")),
    blockACK(config.get<Bit>("blockACK")),
    blockACKreq(config.get<Bit>("blockACKreq")),
    beacon(config.get<Bit>("beacon"))
{

}

FrameLength::FrameLength( const ConfigGetter& config ):
    macDataHdr(config.get<Bit>("macDataHdr", "i")),
    macDataFCS(config.get<Bit>("macDataFCS", "i")),
    amsdu_subhdr(config.get<Bit>("amsdu_subhdr", "i")),
    ampdu_delimiter(config.get<Bit>("ampdu_delimiter", "i")),
    service(config.get<Bit>("service", "i")),
    tail(config.get<Bit>("tail", "i")),
    ack(config.get<Bit>("ack", "i")),
    rts(config.get<Bit>("rts", "i")),
    cts(config.get<Bit>("cts", "i")),
    blockACK(config.get<Bit>("blockACK", "i")),
    blockACKreq(config.get<Bit>("blockACKreq", "i")),
    beacon(config.get<Bit>("beacon", "i"))
{

}

Bit
FrameLength::getPSDU(Bit msduFrameSize) const
{
    return (msduFrameSize + this->macDataHdr + this->macDataFCS);
}

Bit
FrameLength::getA_MPDU_PSDU(Bit mpduFrameSize, unsigned int n_aggFrames) const
{
    // n-1 frames with padding, last one without
    Bit len = n_aggFrames * pad(this->ampdu_delimiter + mpduFrameSize, 32);
    len += blockACKreq;
    return(len + this->ampdu_delimiter + mpduFrameSize);
}

Bit
FrameLength::getA_MPDU_PSDU(const std::vector<Bit>& mpduFrameSize) const
{
    assure(not mpduFrameSize.empty(), "vector must have at least one entry");

    Bit len = 0;
    Bit last = 0;

    for(std::vector<Bit>::const_iterator it = mpduFrameSize.begin();
        it != mpduFrameSize.end();
        ++it)
    {
        last = pad(this->ampdu_delimiter + (*it), 32);
        len += last;
    }
    len += blockACKreq;
    return(len);
}

Bit
FrameLength::getA_MSDU_PSDU(Bit msduFrameSize, unsigned int n_aggFrames) const
{
    // n-1 frames with padding, last one without
    Bit len = (n_aggFrames - 1) * pad(this->amsdu_subhdr + msduFrameSize, 32);
    return(len + this->macDataHdr + this->amsdu_subhdr + msduFrameSize + this->macDataFCS);
}

Bit
FrameLength::getA_MSDU_PSDU(const std::vector<Bit>& msduFrameSize) const
{
    assure(not msduFrameSize.empty(), "vector must have at least one entry");

    Bit len = 0;
    Bit last = 0;

    for(std::vector<Bit>::const_iterator it = msduFrameSize.begin();
        it != msduFrameSize.end();
        ++it)
    {
        last = pad(this->amsdu_subhdr + (*it), 32);
        len += last;
    }
    len += this->macDataHdr + this->amsdu_subhdr + msduFrameSize.back() + this->macDataFCS - last;
    return(len);
}

Bit
FrameLength::pad(Bit frameSize, Bit multiple) const
{
    if((frameSize % multiple) == 0)
    {
        return frameSize;
    }
    else
    {
        return (frameSize + (multiple - (frameSize % multiple)));
    }
}
