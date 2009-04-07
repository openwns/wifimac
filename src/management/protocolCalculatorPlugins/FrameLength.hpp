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

#ifndef WIFIMAC_MANAGEMENT_PROTOCOLCALCULATORPLUGINS_FRAMELENGTH_HPP
#define WIFIMAC_MANAGEMENT_PROTOCOLCALCULATORPLUGINS_FRAMELENGTH_HPP

#include <WNS/pyconfig/View.hpp>
#include <WNS/simulator/Bit.hpp>

namespace wifimac { namespace management { namespace protocolCalculatorPlugins {
	/** 
     * @brief plugin to determine PSDU sizes for different MAC data units
     *
     * this class calculates the resulting PSDU length in bits for
     * MSDUs, A-MPDUs, A-MSDUs
     */
    class FrameLength
    {
    public:
        FrameLength( const wns::pyconfig::View& config );
        virtual ~FrameLength() {};

	// @brief returns size of the resulting PSDU
        Bit getPSDU(Bit msduFrameSize) const;

 	// @brief calculates PSDU size of A-MPDU with fixed sized frames
        Bit getA_MPDU_PSDU(Bit mpduFrameSize, unsigned int n_aggFrames) const;

 	// @brief calculates PSDU size of A-MPDU with arbitrary sized frames
        Bit getA_MPDU_PSDU(const std::vector<Bit>& mpduFrameSize) const;

 	// @brief calculates PSDU size of A-MSDU with fixed sized frames
        Bit getA_MSDU_PSDU(Bit msduFrameSize, unsigned int n_aggFrames) const;

 	// @brief calculates PSDU size of A-MSDU with arbitrary sized frames
        Bit getA_MSDU_PSDU(const std::vector<Bit>& msduFrameSize) const;

        Bit pad(Bit frameSize, Bit multiple) const;

        const Bit macDataHdr;
        const Bit macDataFCS;
        const Bit amsdu_subhdr;
        const Bit ampdu_delimiter;
        const Bit service;
        const Bit tail;
        const Bit ack;
        const Bit rts;
        const Bit cts;
        const Bit blockACK;
        const Bit blockACKreq;
        const Bit beacon;

    };
} // protocolCalculatorPlugins
} // management
} // wifimac

#endif
