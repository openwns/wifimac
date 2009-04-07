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
        Duration( wifimac::management::ProtocolCalculator* pc_, const wns::pyconfig::View& config );
        virtual ~Duration() {};

        unsigned int
        getOFDMSymbols(Bit length, Bit dbps, unsigned int streams, unsigned int bandwidth) const;

        wns::simulator::Time
        getFrame(Bit psduLength, Bit dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getACK(unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getSIFS() const;

        wns::simulator::Time
        getAIFS(unsigned int n) const;

        wns::simulator::Time
        getEIFS(unsigned int streams, unsigned int bandwidth, std::string plcpMode, unsigned int aifsn) const;

        wns::simulator::Time
        getRTS(unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getCTS(unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getPreamble(unsigned int streams, std::string plcpMode) const;

        wns::simulator::Time
        getPreambleProcessing(unsigned int streams, std::string plcpMode) const;

        wns::simulator::Time
        getBlockACK(unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getMSDU_PPDU(Bit msduFrameSize, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;
        wns::simulator::Time
        getMPDU_PPDU(Bit mpduSize, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getA_MPDU_PPDU(Bit mpduFrameSize, unsigned int n_aggFrames, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getA_MPDU_PPDU(const std::vector<Bit>& mpduFrameSize, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getA_MSDU_PPDU(Bit msduFrameSize, unsigned int n_aggFrames, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        wns::simulator::Time
        getA_MSDU_PPDU(const std::vector<Bit>& msduFrameSize, unsigned int dbps, unsigned int streams, unsigned int bandwidth, std::string plcpMode) const;

        const wns::simulator::Time symbol;
        const wns::simulator::Time slot;

    private:
        const wifimac::management::ProtocolCalculator* pc;
        const Bit basicDBPS;


    };
} // protocolCalculatorPlugins
} // management
} // wifimac

#endif
