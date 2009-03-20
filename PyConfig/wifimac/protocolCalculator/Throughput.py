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
from Duration import Duration
from ErrorProbability import ErrorProbability
from MIMO import MIMO
from PhyMode import PhyMode
from FrameLength import FrameLength

class Throughput:

    duration = None
    errorProbability = None
    mimo = None
    phyMode = None
    frameLength = None

    cwMin = None
    maxBackoffStage = None
    plcpMode = None
    aifsn = None

    def __init__(self, guardInterval_us = 0.8, cwMin = 15, maxBackoffStage = 2, plcpMode="HT-Mix", aifsn=2):
        self.duration = Duration(guardInterval_us = guardInterval_us)
        self.errorProbability = ErrorProbability(guardInterval_us = guardInterval_us)
        self.mimo = MIMO()
        self.phyMode = PhyMode()
        self.frameLength = FrameLength()

        self.cwMin = cwMin
        self.maxBackoffStage = maxBackoffStage
        self.plcpMode = plcpMode
        self.aifsn = aifsn

    def tau_p_on_W_m_pe(self, n_users, cwMin, maxBackoffStage, per):
        assert(n_users == 1)

        tau = 2.0*(1.0-2.0*per)/((1.0-2.0*per)*(cwMin+1.0)+per*cwMin*(1.0-(2.0*per)**maxBackoffStage))
        return(tau, per)

    def getPER_PPDU(self, packetLength_bit, aggMode, n_aggFrames, modulation, coding, postSNR_dB):
        if(aggMode == "No"):
            return(self.getPER_MPDU(packetLength_bit, "No", 0, modulation, coding, postSNR_dB))
        elif(aggMode == "A-MSDU"):
            return(self.getPER_MPDU(packetLength_bit, aggMode, n_aggFrames, modulation, coding, postSNR_dB))
        elif(aggMode == "A-MPDU"):
            return(self.getPER_MPDU(packetLength_bit, "No", 0, modulation, coding, postSNR_dB)**n_aggFrames)
        else:
            assert(False)

    def getPER_MPDU(self, packetLength_bit, aggMode, n_aggFrames, modulation, coding, postSNR_dB):
        if(aggMode == "No"):
            mpduLength = self.frameLength.getPSDU_bit(packetLength_bit, "No", 0)
            return(self.errorProbability.getPER(postSNR_dB, mpduLength, modulation, coding))
        elif(aggMode == "A-MSDU"):
            fullLength_bit = self.frameLength.getAggMSDU_bit(packetLength_bit, n_aggFrames)
            return(self.errorProbability.getPER(postSNR_dB, fullLength_bit, modulation, coding))
        elif(aggMode == "A-MPDU"):
            segmentLength_bit = self.frameLength.getPSDU_bit(packetLength_bit, "No", 0)
            return(self.errorProbability.getPER(postSNR_dB, segmentLength_bit, modulation, coding))
        else:
            assert(False)

    def getThroughput(self, packetLength_bit, aggMode, n_aggFrames, modulation, coding, preSNR_dB, n_streams, n_rxAntennas, bandwidth, n_users):
        n_dbps = self.phyMode.getDBPS(modulation, coding)
        postSNR_dB = self.mimo.getPostSNR_dB(preSNR_dB, n_streams, n_rxAntennas)
        per = self.getPER_PPDU(packetLength_bit, aggMode, n_aggFrames, modulation, coding, postSNR_dB)
        (tau, per) = self.tau_p_on_W_m_pe(n_users, self.cwMin, self.maxBackoffStage, per)
        # probability of channel idle slot
        p_idle = (1.0 - tau)**n_users
        # probability of transmission without collision
        p_trxNoCollision = (n_users * tau * (1.0 - tau)**(n_users-1))
        # probability of having at least one STA transmitting
        p_tr = 1 - p_idle

        # expected slot length
        E_us  = (self.duration.getIdleSlot_us() * p_idle +
                 self.duration.getCollisionSlot_us(n_streams, bandwidth, self.plcpMode, self.aifsn) * (p_tr - p_trxNoCollision) +
                 self.duration.getErrorSlot_us(packetLength_bit, aggMode, n_aggFrames, n_dbps, n_streams, bandwidth, self.plcpMode, self.aifsn) * p_trxNoCollision * per +
                 (self.duration.getSuccessSlot_us(packetLength_bit, aggMode, n_aggFrames, n_dbps, n_streams, bandwidth, self.plcpMode, self.aifsn) *
                  (p_trxNoCollision - p_trxNoCollision * per)))

        # expected packet body size
        E_p = 0
        if(aggMode == "No"):
            E_p = packetLength_bit * p_trxNoCollision * (1.0 - per)
        elif(aggMode == "A-MSDU"):
            E_p = packetLength_bit * n_aggFrames * p_trxNoCollision * (1.0 - per)
        elif(aggMode == "A-MPDU"):
            E_p = (packetLength_bit *
                   (1.0 - self.getPER_MPDU(packetLength_bit, aggMode, n_aggFrames, modulation, coding, postSNR_dB)) *
                   n_aggFrames *
                   p_trxNoCollision *
                   (1.0 - per))
        else:
            assert(False)

        return(E_p / E_us)

