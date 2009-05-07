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
    """ FCS field of data frames length in bit """
    amsdu_subhdr = None
    """ Subheader for each aggregated msdu for A-MSDU"""
    ampdu_delimiter = None
    """ Delimiter size for each aggregated msdu for A-MPDU"""
    service = None
    tail = None
    ack = None
    """ ACK frame length in bit 
        * Basic IEEE 802.11: 10B + 4B FCS
        * IEEE 802.11n: transmitted in Wrapper Control Frame,
        which is 20 bytes including FCS. (D802.11n,D4.00, 7.2.1.9) """
    rts = None
    """ RTS frame length in bit 
        * Basic IEEE 802.11: 16B + 4B FCS
        * IEEE 802.11n: transmitted in wrapper control frame,
        which is 22 bytes including FCS (D802.11n, D4.00, 7.2.1.9) """
    cts = None
    """ CTS frame length in bit
        * Basic IEEE 802.11: 10B + 4B FCS
        * IEEE 802.11n: transmitted in wrapper control frame,
        which is 20 bytes including FCS (D802.11n, D4.00, 7.2.1.9) """
    blockACK = None
    """ BlockACK frame length in bit (transmitted in wrapper control
        frame, and compressed info field, which is 32 bytes including
        FCS.)(D802.11n, D4.00, 7.2.1.8.1) """
    blockACKreq = None
    beacon = None
    """ Size of beacon without any extra IEs"""

    def __init__(self):
        self.macDataHdr = 30*8
        self.macDataFCS = 4*8
        self.amsdu_subhdr = 14*8
        self.ampdu_delimiter = 4*8
        self.service = 16
        self.tail = 6
        self.ack = 10*8+4*8
        self.rts = 16*8+4*8
        self.cts = 10*8+4*8
        self.blockACK = 32*8
        self.blockACKreq = 24*8
        # managementHdr + timestamp + Interval + capability + ssid + supportedRates + tim
        self.beacon = 24*8 + 8*8 + 2*2 + 2*8 + 2*8 + 3*8 + 6*8
