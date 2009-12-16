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
    symbolWithoutGI(config.get<wns::simulator::Time>("symbolDurationWithoutGI")),
    slot(config.get<wns::simulator::Time>("slot")),
    sifs(config.get<wns::simulator::Time>("sifs")),
    fl(fl_)
{

}

Duration::Duration( FrameLength* fl_, const ConfigGetter& config ):
    symbolWithoutGI(config.get<wns::simulator::Time>("symbolDurationWithoutGI", "d")),
    slot(config.get<wns::simulator::Time>("slot", "d")),
    sifs(config.get<wns::simulator::Time>("sifs", "d")),
    fl(fl_)
{

}

unsigned int
Duration::ofdmSymbols(Bit psduLength, const wifimac::convergence::PhyMode& pm) const
{
    unsigned int n_es = 1;
    // TODO: correct n_es for higher phy modes
    return(static_cast<int>(ceil(static_cast<double>(psduLength + fl->service + fl->tail*n_es) /
                                 static_cast<double>(pm.getDataBitsPerSymbol()))));
}

wns::simulator::Time
Duration::frame(Bit psduLength, const wifimac::convergence::PhyMode& pm) const
{
    return(preamble(pm) + ofdmSymbols(psduLength, pm) * symbol(pm));
}

wns::simulator::Time
Duration::frame(Bit psduLength, const double grossBitRate, const wifimac::convergence::PhyMode& pm) const
{
    unsigned int n_es = 1;
    // TODO: correct n_es for higher phy modes
    double psduFrameSize = fl->service + psduLength + fl->tail*n_es;
    double durationPSDU = psduFrameSize / grossBitRate;
    return(preamble(pm) + durationPSDU);
}

wns::simulator::Time
Duration::preamble(const wifimac::convergence::PhyMode& pm) const
{
    // number of DLFT and ELFT in HT-preamble (D802.11n,D4.00, Table 20-11~13)
    // here: N_ss=N_sts=Mt, and N_ess=0

    unsigned int dltf = 0;
    unsigned int eltf = 0;
    wns::simulator::Time s = symbol(pm);

    if(pm.getNumberOfSpatialStreams() == 3)
    {
        dltf = 4;
    }
    else
    {
        dltf = pm.getNumberOfSpatialStreams();
    }

    if(pm.getPreambleMode() == "Basic")
    {
        return(16e-6 + s);
    }

    if(pm.getPreambleMode() == "HT-Mix")
    {
        // Non-HT short training sequence (D802.11n,D4.00, Table 20-5): 8 us
        // Non-HT long training sequence (D802.11n,D4.00, Table 20-5): 8 us
        //  --> duration of non-HT preamble, (D802.11n,D4.00, 20.4.3): 16 us
        // plus signal symbol: 4us
        // HT-STF duration: 4 us
        // HT first long training field duration for HT-mixed mode: 4 us
        // HT second, and subsequent, long training fields duration: 4 us
        //  --> HT-preamble for mixed mode: 4+4+ (DLTF+ELTF)*4

        return(16e-6 + s + (2 + dltf + eltf - 1)*4e-6 + 2*s);
    }

    if(pm.getPreambleMode() == "HT-GF")
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
        return(8e-6 + 8e-6 + (dltf + eltf - 1)*4e-6 + 2*s);
    }

    assure(pm.getPreambleMode() == "Basic" or pm.getPreambleMode() == "HT-Mix" or pm.getPreambleMode() == "HT-GF", "Unknown plcpMode");

    return(1);
}

wns::simulator::Time
Duration::preambleProcessing(const wifimac::convergence::PhyMode& pm) const
{
    return(preamble(pm) + 1e-6);
}

wns::simulator::Time
Duration::ack(const wifimac::convergence::PhyMode& pm) const
{
    return(this->frame(fl->ack, pm));
}

wns::simulator::Time
Duration::aifs(unsigned int n) const
{
    return(sifs + n * slot);
}

wns::simulator::Time
Duration::eifs(const wifimac::convergence::PhyMode& pm, unsigned int aifsn) const
{
    return(sifs + ack(pm) + aifs(aifsn));
}

wns::simulator::Time
Duration::rts(const wifimac::convergence::PhyMode& pm) const
{
    return(this->frame(fl->rts, pm));
}

wns::simulator::Time
Duration::cts(const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->cts, pm));
}

wns::simulator::Time
Duration::blockACK(const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->blockACK, pm));
}

wns::simulator::Time
Duration::MSDU_PPDU(Bit msduFrameSize, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->getPSDU(msduFrameSize), pm));
}

wns::simulator::Time
Duration::MSDU_PPDU(Bit msduFrameSize, const double grossBitRate, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->getPSDU(msduFrameSize), grossBitRate, pm));
}

wns::simulator::Time
Duration::MPDU_PPDU(Bit mpduSize, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(mpduSize, pm));
}

wns::simulator::Time
Duration::MPDU_PPDU(Bit mpduSize, const double grossBitRate, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(mpduSize, grossBitRate, pm));
}

wns::simulator::Time
Duration::A_MPDU_PPDU(Bit mpduFrameSize, unsigned int n_aggFrames, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->getA_MPDU_PSDU(mpduFrameSize, n_aggFrames), pm));
}

wns::simulator::Time
Duration::A_MPDU_PPDU(Bit mpduFrameSize, unsigned int n_aggFrames, const double grossBitRate, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->getA_MPDU_PSDU(mpduFrameSize, n_aggFrames), grossBitRate, pm));
}

wns::simulator::Time
Duration::A_MPDU_PPDU(const std::vector<Bit>& mpduFrameSize, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->getA_MPDU_PSDU(mpduFrameSize), pm));
}

wns::simulator::Time
Duration::A_MSDU_PPDU(Bit msduFrameSize, unsigned int n_aggFrames, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->getA_MSDU_PSDU(msduFrameSize, n_aggFrames), pm));
}

wns::simulator::Time
Duration::A_MSDU_PPDU(Bit msduFrameSize, unsigned int n_aggFrames, const double grossBitRate, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->getA_MSDU_PSDU(msduFrameSize, n_aggFrames), grossBitRate, pm));
}

wns::simulator::Time
Duration::A_MSDU_PPDU(const std::vector<Bit>& msduFrameSize, const wifimac::convergence::PhyMode& pm) const
{
    return(frame(fl->getA_MSDU_PSDU(msduFrameSize), pm));
}

wns::simulator::Time
Duration::symbol(const wifimac::convergence::PhyMode &pm) const
{
    return(this->symbolWithoutGI + pm.getGuardIntervalDuration());
}
