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

#ifndef WIFIMAC_MANAGEMENT_PROTOCOLCALCULATORPLUGINS_DURATION_HPP
#define WIFIMAC_MANAGEMENT_PROTOCOLCALCULATORPLUGINS_DURATION_HPP

// must be the first include!
#include <WNS/Python.hpp>

#include <WIFIMAC/management/protocolCalculatorPlugins/ConfigGetter.hpp>
#include <WIFIMAC/management/protocolCalculatorPlugins/FrameLength.hpp>
#include <WIFIMAC/convergence/PhyMode.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/simulator/Time.hpp>


namespace wifimac { namespace management {
        class ProtocolCalculator;
}}

namespace wifimac { namespace management { namespace protocolCalculatorPlugins {
    /**
     * @brief Class to determine transmission duration for different PDUs
     *
     * this class calculates the actual duration of different PDUs under given
     * phy parameters like data bits per symbol, number of streams etc
     * the (A)M(S/P)DU duration functions expect the proper size(s) of the PDU(s)
     * e.g. a vector with frame sizes for an aggregated MPDU
     */
    class Duration
    {
    public:
        Duration( wifimac::management::protocolCalculatorPlugins::FrameLength* fl_, const wns::pyconfig::View& config );

        Duration( wifimac::management::protocolCalculatorPlugins::FrameLength* fl_, const ConfigGetter& config);

        virtual ~Duration() {};

        unsigned int
        ofdmSymbols(Bit length, const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        frame(Bit psduLength, const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        ack(const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        aifs(unsigned int n) const;

        wns::simulator::Time
        eifs(const wifimac::convergence::PhyMode& pm, unsigned int aifsn) const;

        wns::simulator::Time
        rts(const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        cts(const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        preamble(const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        preambleProcessing(const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        blockACK(const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        MSDU_PPDU(Bit msduFrameSize, const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        MPDU_PPDU(Bit mpduSize, const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        A_MPDU_PPDU(Bit mpduFrameSize, unsigned int n_aggFrames, const wifimac::convergence::PhyMode& pm) const;
        wns::simulator::Time
        A_MPDU_PPDU(const std::vector<Bit>& mpduFrameSize, const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        A_MSDU_PPDU(Bit msduFrameSize, unsigned int n_aggFrames, const wifimac::convergence::PhyMode& pm) const;
        wns::simulator::Time
        A_MSDU_PPDU(const std::vector<Bit>& msduFrameSize, const wifimac::convergence::PhyMode& pm) const;

        wns::simulator::Time
        symbol(const wifimac::convergence::PhyMode &pm) const;

        const wns::simulator::Time sifs;
        const wns::simulator::Time slot;

    private:
        const wns::simulator::Time symbolWithoutGI;
        const wifimac::management::protocolCalculatorPlugins::FrameLength* fl;


    };
} // protocolCalculatorPlugins
} // management
} // wifimac

#endif
