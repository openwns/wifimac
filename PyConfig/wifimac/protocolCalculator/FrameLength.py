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

class FrameLength(object):
    macDataHdr = None
    """ mac header length for data and management frames in bit """
    macDataFCS = None
    """ FCS field length in bit """
    amsdu_subhdr = None
    """ Subheader for each aggregated msdu for A-MSDU"""
    ampdu_delimiter = None
    """ Delimiter size for each aggregated msdu for A-MPDU"""
    service = None
    tail = None
    ack = None
    """ ACK frame length in bit (transmitted in Wrapper Control Frame,
        which is 20 bytes including FCS.) (D802.11n,D4.00, 7.2.1.9) """
    rts = None
    """ RTS frame length in bit (transmitted in wrapper control frame,
        which is 22 bytes including FCS.)(D802.11n, D4.00, 7.2.1.9) """
    cts = None
    """ CTS frame length in bit (transmitted in wrapper control frame,
        which is 20 bytes including FCS.)(D802.11n, D4.00, 7.2.1.9) """
    blockACK = None
    """ BlockACK frame length in bit (transmitted in wrapper control frame,
        and compressed info field, which is 43 bytes including FCS.)(D802.11n, D4.00, 7.2.1.8.1) """
    blockACKreq = None
    beacon = None
    """ Size of beacon without any extra IEs"""

    def __init__(self):
        self.macDataHdr = 36*8
        self.macDataFCS = 4*8
        self.amsdu_subhdr = 14*8
        self.ampdu_delimiter = 4*8
        self.service = 16
        self.tail = 6
        self.ack = 20*8
        self.rts = 22*8
        self.cts = 20*8
        self.blockACK = 32*8
        self.blockACKreq = 24*8
        # managementHdr + timestamp + Interval + capability + ssid + supportedRates + tim
        self.beacon = 24*8 + 8*8 + 2*2 + 2*8 + 2*8 + 3*8 + 6*8

    def getAggMSDU(self, msduFrameSize, n_aggFrames = 0):
        if(isinstance(msduFrameSize, int)):
            """ All MSDUs have equal size """
            assert(n_aggFrames > 0)
            msduFrameSize = [msduFrameSize]*n_aggFrames
        else:
            """ msduFrameSize is an array """
            assert(n_aggFrames == 0)
            assert(len(msduFrameSize) > 0)

        a_msduLen = 0
        for l in msduFrameSize[0:-1]:
            a_msduLen += 32*int(math.ceil((self.amsdu_subhdr + l)/32.0))
        a_msduLen += (self.amsdu_subhdr + l) + self.macDataHdr + self.macDataFCS
        return(a_msduLen)

    def getAggMPDU(self, msduFrameSize, n_aggFrames = 0):
        if(isinstance(msduFrameSize, int)):
            # All MSDUs have equal size
            assert(n_aggFrames > 0)
            msduFrameSize = [msduFrameSize]*n_aggFrames
        else:
            # msduFrameSize is an array
            assert(n_aggFrames == 0)
            assert(len(msduFrameSize) > 0)

        a_mpduLen = 0

        for l in msduFrameSize:
            a_mpduLen += 32*int(math.ceil((self.ampdu_delimiter + self.macDataHdr + l + self.macDataFCS)/32.0))

        # add blockACKreq, but without padding
        a_mpduLen += self.ampdu_delimiter + self.macDataHdr + self.blockACKreq + self.macDataFCS

        return(a_mpduLen)

    def getPSDU(self, msduFrameSize, aggMode, n_aggFrames):
        if(aggMode == "No"):
            return(self.macDataHdr + self.macDataFCS + msduFrameSize)
        elif(aggMode == "A-MSDU"):
            return(self.getAggMSDU(msduFrameSize, n_aggFrames))
        elif(aggMode == "A-MPDU"):
            return(self.getAggMPDU(msduFrameSize, n_aggFrames))
        else:
            assert(False)

        return(0)

