###############################################################################
# This file is part of openWNS (open Wireless Network Simulator)
# _____________________________________________________________________________
#
# Copyright (C) 2004-2008
# Chair of Communication Networks (ComNets)
# Kopernikusstr. 16, D-52074 Aachen, Germany
# phone: ++49-241-80-27910,
# fax: ++49-241-80-22242
# email: info@openwns.org
# www: http://www.openwns.org
# _____________________________________________________________________________
#
# openWNS is free software; you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License version 2 as published by the
# Free Software Foundation;
#
# openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

import math
import numpy

from PhyMode import PhyMode
from FrameLength import FrameLength

class Duration:

    symbol = None
    """ OFDM symbol duration for all modes"""
    basicDBPS = None
    """ Number of DBPS in the basic PHY mode for ACK, RTS, etc."""
    slot = None
    """ Duration of a slot """
    fl = None

    def __init__(self, guardInterval):
        self.symbol = 3.2e-6 + guardInterval
        self.slot = 9e-6
        pm = PhyMode()
        self.basicDBPS = pm.getDBPS("BPSK", "1/2")
        self.fl = FrameLength()

    def n_ofdm_sym(self, n_bits, n_dbps, n_streams, bandwidth):
        """ compute the number of ofdm symbols from
        * packet length
        * data bits per symbol
        * number of spatial streams
        * bandwidth in MHz
        """
        n_es = 1
        if(bandwidth == 40.0):
            if (n_dbps >= 208 and n_streams >= 3):
                n_es = 2
            elif(n_dbps == 156 and n_streams == 4):
                n_es = 2
        bwFac = 1
        if(bandwidth == 40.0):
            bwFac = 54.0/26.0

        n_dbps = int(n_dbps * bwFac) * n_streams
        n_symb = math.ceil((n_bits+self.fl.service_bit+self.fl.tail_bit*n_es)/float(n_dbps))

        return n_symb

    def getFrame(self, psdu_bit, n_dbps, n_streams, bandwidth, plcpMode):
        """ Computes the frame duration for a given PSDU length"""
        n_symb = self.n_ofdm_sym(n_bits = psdu_bit,
                                 n_dbps = n_dbps,
                                 n_streams = n_streams,
                                 bandwidth = bandwidth)
        t_Preamble = self.getPreamble(plcpMode = plcpMode,
                                            n_streams = n_streams)

        return(t_Preamble + n_symb*self.symbol)

    def getACK(self, n_streams, bandwidth, plcpMode):
        """ ACK frame length in us"""
        return(self.getFrame(psdu_bit = self.fl.ack_bit,
                                n_dbps = self.basicDBPS,
                                n_streams = n_streams,
                                bandwidth = bandwidth,
                                plcpMode = plcpMode))

    def getSIFS(self):
        """ SIFS duration in us (16us), (D802.11n/D4.00, Table 20-24) """
        return(self.symbol * 4)

    def getAIFS(self, n=2):
        """ AIFS value in us (Std802.11, 9.2.10); (See (Std802.11, 9.9.1.3) for AIFS) """
        # n = 1 -> PIFS; n = 2 -> DIFS
        return(self.getSIFS() + n * self.slot)

    def getEIFS(self, n_streams, bandwidth, plcpMode, aifsn):
        """ EIFS value in us, (Std802.11, 9.2.3.4 & 9.2.10)"""
        return(self.getSIFS() + self.getACK(n_streams, bandwidth, plcpMode) + self.getAIFS(aifsn))

    def getRTS(self, n_streams, bandwidth, plcpMode):
        """ RTS frame length in us"""
        return(self.getFrame(psdu_bit = self.fl.rts_bit,
                                n_dbps = self.basicDBPS,
                                n_streams = n_streams,
                                bandwidth = bandwidth,
                                plcpMode = plcpMode))

    def getCTS(self, n_streams, bandwidth, plcpMode):
        """ CTS frame length in bit"""
        return(self.getFrame(psdu_bit = self.fl.cts_bit,
                                n_dbps = self.basicDBPS,
                                n_streams = n_streams,
                                bandwidth = bandwidth,
                                plcpMode = plcpMode))

    def getPreamble(self, plcpMode, n_streams):
        """ Calculation of preamble duration (in us) """
        # number of DLFT and ELFT in HT-preamble (D802.11n,D4.00, Table 20-11~13)
        # for us: N_ss=N_sts=Mt, and N_ess=0
        n_DLTF = 0
        n_ELTF = 0
        if(n_streams == 3):
            n_DLTF = 4
        else:
            n_DLTF = n_streams

        if(plcpMode == "HT-Mix"):
            # Non-HT short training sequence (D802.11n,D4.00, Table 20-5)
            t_L_STF = 8    # us
            # Non_HT long training sequence (D802.11n,D4.00, Table 20-5)
            t_L_LTF = 8    # us
            # duration of non-HT preamble, (D802.11n,D4.00, 20.4.3):
            t_LEG_PRE = t_L_STF + t_L_LTF # us
            # signal symbol
            t_L_SIG = 4    # us
            # HT-STF duration
            t_HT_STF = 4   # us
            # HT first long training field duration for HT-mixed mode
            t_HT_LTF1_mix = 4 # us
            # HT second, and subsequent, long training fields duration
            t_HT_LTFs = 4     # us
        # calcualte HT preamble for mixed mode:
            t_HT_PRE = t_HT_STF + t_HT_LTF1_mix + (n_DLTF + n_ELTF - 1) * t_HT_LTFs
            # signal symbol for HT
            t_HT_SIG = 8      # us
            # total duration of HT-Mix mode
            return(t_LEG_PRE + t_L_SIG + t_HT_PRE + t_HT_SIG)
        elif(plcpMode == "HT-GF"):
            # HT-GF short traning field duration
            t_GF_STF = 8
            # HT first long traning field duration for HT-GF mode
            t_HT_LTF1_gf = 8
            # HT second, and subsequent, long training fields duration
            t_HT_LTFs = 4
            # HT preamble for greenfield mode
            t_HT_PRE = t_GF_STF + t_HT_LTF1_gf + (n_DLTF + n_ELTF - 1) * t_HT_LTFs;
            # signal symbol for HT mode
            t_HT_SIG = 8
            # total duration of HT-GF mode
            return(t_HT_PRE + t_HT_SIG)
        assert(false, "Unknown plcpMode " + str(plcpMode))
        return 0

    def getBlockACK(self, n_streams, bandwidth, plcpMode):
        """ BlockACK frame length in us"""
        return(self.getFrame(psdu_bit = self.fl.blockACK_bit,
                                n_dbps = self.basicDBPS,
                                n_streams = n_streams,
                                bandwidth = bandwidth,
                                plcpMode = plcpMode))

    def getPPDU(self, msduFrameSize_bit, aggMode, n_aggFrames, n_dbps, n_streams, bandwidth, plcpMode):
         return(self.getFrame(psdu_bit = self.fl.getPSDU_bit(msduFrameSize_bit, aggMode, n_aggFrames),
                                 n_dbps = n_dbps,
                                 n_streams = n_streams,
                                 bandwidth = bandwidth,
                                 plcpMode = plcpMode))

    def getIdleSlot(self):
        return(self.slot)

    def getCollisionSlot(self, n_streams, bandwidth, plcpMode, aifsn):
        return(self.getRTS(n_streams, bandwidth, plcpMode) + self.getEIFS(n_streams, bandwidth, plcpMode, aifsn))

    def getErrorSlot(self, msduFrameSize_bit, aggMode, n_aggFrames, n_dbps, n_streams, bandwidth, plcpMode, aifsn):
        return(self.getRTS(n_streams, bandwidth, plcpMode) +
               self.getCTS(n_streams, bandwidth, plcpMode) +
               self.getPPDU(msduFrameSize_bit, aggMode, n_aggFrames, n_dbps, n_streams, bandwidth, plcpMode) +
               self.getEIFS(n_streams, bandwidth, plcpMode, aifsn) +
               2 * self.getSIFS())

    def getSuccessSlot(self, msduFrameSize_bit, aggMode, n_aggFrames, n_dbps, n_streams, bandwidth, plcpMode, aifsn):
        ### TODO: BlockACK only in case of A-MPDU aggregation
        if(aggMode == "No"):
            return(self.getRTS(n_streams, bandwidth, plcpMode) +
                   self.getCTS(n_streams, bandwidth, plcpMode) +
                   self.getPPDU(msduFrameSize_bit, aggMode, n_aggFrames, n_dbps, n_streams, bandwidth, plcpMode) +
                   self.getACK(n_streams, bandwidth, plcpMode) +
                   3 * self.getSIFS() +
                   self.getAIFS())
        else:
            return(self.getRTS(n_streams, bandwidth, plcpMode) +
                   self.getCTS(n_streams, bandwidth, plcpMode) +
                   self.getPPDU(msduFrameSize_bit, aggMode, n_aggFrames, n_dbps, n_streams, bandwidth, plcpMode) +
                   self.getBlockACK(n_streams, bandwidth, plcpMode) +
                   3 * self.getSIFS() +
                   self.getAIFS())
