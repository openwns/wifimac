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

Bit
FrameLength::getPSDU(Bit msduFrameSize) const
{
    return (msduFrameSize + this->macDataHdr + this->macDataFCS);
}

Bit
FrameLength::getA_MPDU_PSDU(Bit msduFrameSize, unsigned int n_aggFrames) const
{
    // n-1 frames with padding, last one without
    Bit len = (n_aggFrames - 1) * pad(this->ampdu_delimiter + this->macDataHdr + msduFrameSize + this->macDataFCS, 32);
    return(len + this->ampdu_delimiter + this->macDataHdr + msduFrameSize + this->macDataFCS);
}

Bit
FrameLength::getA_MPDU_PSDU(const std::vector<Bit>& msduFrameSize) const
{
    assure(not msduFrameSize.empty(), "vector must have at least one entry");

    Bit len = 0;
    Bit last = 0;

    for(std::vector<Bit>::const_iterator it = msduFrameSize.begin();
        it != msduFrameSize.end();
        ++it)
    {
        last = pad(this->ampdu_delimiter + this->macDataHdr + (*it) + this->macDataFCS, 32);
        len += last;
    }
    len += this->ampdu_delimiter + this->macDataHdr + msduFrameSize.back() + this->macDataFCS - last;
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
FrameLength::pad(Bit msduFrameSize, Bit multiple) const
{
    if((msduFrameSize % multiple) == 0)
    {
        return msduFrameSize;
    }
    else
    {
        return (msduFrameSize + (multiple - (msduFrameSize % multiple)));
    }
}
