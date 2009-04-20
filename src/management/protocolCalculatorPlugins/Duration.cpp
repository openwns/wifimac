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

#include <WIFIMAC/management/protocolCalculatorPlugins/Duration.hpp>
#include <cmath>

using namespace wifimac::management::protocolCalculatorPlugins;

Duration::Duration( FrameLength* fl_, const wns::pyconfig::View& config):
    symbol(config.get<wns::simulator::Time>("symbol")),
    slot(config.get<wns::simulator::Time>("slot")),
    basicDBPS(config.get<Bit>("basicDBPS")),
    fl(fl_)
{

}

Duration::Duration( FrameLength* fl_, const ConfigGetter& config ):
    symbol(config.get<wns::simulator::Time>("symbol", "d")),
    slot(config.get<wns::simulator::Time>("slot", "d")),
    basicDBPS(config.get<Bit>("basicDBPS", "i")),
    fl(fl_)
{

}

unsigned int
Duration::getOFDMSymbols(Bit psduLength, Bit dbps, unsigned int streams, unsigned int bandwidth) const
{
    unsigned int n_es = 1;
    if(bandwidth == 40)
    {
        if(dbps >= 208 and streams >= 3)
        {
            n_es = 2;
        }
        if(dbps == 156 and streams == 4)
        {
            n_es = 2;
        }
    }

    assure(bandwidth == 20 or bandwidth == 40,
           "Unknown bandwidth");

    if(bandwidth == 20)
    {
        dbps = dbps * streams;
    }
    else
    {
        dbps = dbps * streams * 54 / 26;
    }

    return(static_cast<int>(ceil(static_cast<double>(psduLength + fl->service + fl->tail*n_es)/static_cast<double>(dbps))));
}

wns::simulator::Time
Duration::getFrame(Bit psduLength, Bit dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    unsigned int numSym = getOFDMSymbols(psduLength, dbps, streams, bandwidth);
    wns::simulator::Time preamble = getPreamble(streams, plcpMode);
    return(preamble + numSym * symbol);
}

wns::simulator::Time
Duration::getPreamble(unsigned int streams, std::string plcpMode) const
{
    // number of DLFT and ELFT in HT-preamble (D802.11n,D4.00, Table 20-11~13)
    // here: N_ss=N_sts=Mt, and N_ess=0

    unsigned int dltf = 0;
    unsigned int eltf = 0;

    if(streams == 3)
    {
        dltf = 4;
    }
    else
    {
        dltf = streams;
    }

    if(plcpMode == "Basic")
    {
        return(16e-6 + symbol);
    }

    if(plcpMode == "HT-Mix")
    {
        // Non-HT short training sequence (D802.11n,D4.00, Table 20-5): 8 us
        // Non-HT long training sequence (D802.11n,D4.00, Table 20-5): 8 us
        //  --> duration of non-HT preamble, (D802.11n,D4.00, 20.4.3): 16 us
        // plus signal symbol: 4us
        // HT-STF duration: 4 us
        // HT first long training field duration for HT-mixed mode: 4 us
        // HT second, and subsequent, long training fields duration: 4 us
        //  --> HT-preamble for mixed mode: 4+4+ (DLTF+ELTF)*4

        return(16e-6 + symbol + (2 + dltf + eltf - 1)*4e-6 + 2*symbol);
    }

    if(plcpMode == "HT-GF")
    {
        // Greenfield mode
        // HT-GF short traning field duration
        // t_GF_STF = 8
        // HT first long traning field duration for HT-GF mode
        // t_HT_LTF1_gf = 8
        // HT second, and subsequent, long training fields duration
        // t_HT_LTFs = 4
        // HT preamble for greenfield mode
        // t_HT_PRE = t_GF_STF + t_HT_LTF1_gf + (n_DLTF + n_ELTF - 1) * t_HT_LTFs;
        // signal symbol for HT mode
        // t_HT_SIG = 8
        // total duration of HT-GF mode: t_HT_PRE + t_HT_SIG
        return(8e-6 + 8e-6 + (dltf + eltf - 1)*4e-6 + 2*symbol);
    }

    assure(plcpMode == "Basic" or plcpMode == "HT-Mix" or plcpMode == "HT-GF", "Unknown plcpMode");

    return(1);
}

wns::simulator::Time
Duration::getPreambleProcessing(unsigned int streams, std::string plcpMode) const
{
    return(getPreamble(streams, plcpMode) + 1e-6);
}

wns::simulator::Time
Duration::getACK(unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->ack,
                    basicDBPS,
                    streams,
                    bandwidth,
                    plcpMode));
}

wns::simulator::Time
Duration::getSIFS() const
{
    return(4 * symbol);
}

wns::simulator::Time
Duration::getAIFS(unsigned int n) const
{
    return(getSIFS() + n * slot);
}

wns::simulator::Time
Duration::getEIFS(unsigned int streams, unsigned int bandwidth, std::string plcpMode, unsigned int aifsn) const
{
    return(getSIFS() + getACK(streams, bandwidth, plcpMode) + getAIFS(aifsn));
}

wns::simulator::Time
Duration::getRTS(unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->rts,
                    basicDBPS,
                    streams,
                    bandwidth,
                    plcpMode));
}

wns::simulator::Time
Duration::getCTS(unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->cts,
                    basicDBPS,
                    streams,
                    bandwidth,
                    plcpMode));
}

wns::simulator::Time
Duration::getBlockACK(unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->blockACK,
                    basicDBPS,
                    streams,
                    bandwidth,
                    plcpMode));
}

wns::simulator::Time
Duration::getMSDU_PPDU(Bit msduFrameSize, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->getPSDU(msduFrameSize),
                    dbps,
                    streams,
                    bandwidth,
                    plcpMode));
}

wns::simulator::Time
Duration::getMPDU_PPDU(Bit mpduSize, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(mpduSize,
                    dbps,
                    streams,
                    bandwidth,
                    plcpMode));
}


wns::simulator::Time
Duration::getA_MPDU_PPDU(Bit mpduFrameSize, unsigned int n_aggFrames, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->getA_MPDU_PSDU(mpduFrameSize, n_aggFrames),
                    dbps,
                    streams,
                    bandwidth,
                    plcpMode));
}


wns::simulator::Time
Duration::getA_MPDU_PPDU(const std::vector<Bit>& mpduFrameSize, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->getA_MPDU_PSDU(mpduFrameSize),
                    dbps,
                    streams,
                    bandwidth,
                    plcpMode));
}

wns::simulator::Time
Duration::getA_MSDU_PPDU(Bit msduFrameSize, unsigned int n_aggFrames, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->getA_MSDU_PSDU(msduFrameSize, n_aggFrames),
                    dbps,
                    streams,
                    bandwidth,
                    plcpMode));
}

wns::simulator::Time
Duration::getA_MSDU_PPDU(const std::vector<Bit>& msduFrameSize, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const
{
    return(getFrame(fl->getA_MSDU_PSDU(msduFrameSize),
                    dbps,
                    streams,
                    bandwidth,
                    plcpMode));
}


