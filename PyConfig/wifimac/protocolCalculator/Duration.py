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

import FrameLength

class Duration:

    symbolDurationWithoutGI = None
    """ OFDM symbol duration for all modes"""
    slot = None
    """ Duration of a slot """
    sifs = None
    """ Duration of a short interframe space """
    fl = None

    def __init__(self, fl=None):
        self.symbolDurationWithoutGI = 3.2e-6
        self.slot = 9e-6
        self.sifs = 16e-6
        if(fl is not None):
            self.fl = fl
        else:
            self.fl = FrameLength.FrameLength()


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
