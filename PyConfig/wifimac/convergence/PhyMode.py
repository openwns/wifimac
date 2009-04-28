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
from openwns import dBm, dB, fromdB

import wifimac.Logger

class MCS(object):
    modulation = None
    codingRate = None
    minSINR = None

    def __init__(self, modulation, codingRate, minSINR):
        self.modulation = modulation
	self.codingRate = codingRate
	self.minSINR = minSINR

class PhyMode(MCS):
    numberOfSpatialStreams = None
    numberOfDataSubcarriers = None
    plcpMode = None
    guardIntervalDuration = None

    def __init__(self, modulation, codingRate, numberOfSpatialStreams, numberOfDataSubcarriers, plcpMode, guardIntervalDuration, minSINR):
        super(PhyMode, self).__init__(modulation=modulation, codingRate=codingRate, minSINR=minSINR)
	self.numberOfSpatialStreams = numberOfSpatialStreams
	self.numberOfDataSubcarriers = numberOfDataSubcarriers
	self.plcpMode = plcpMode
	self.guardIntervalDuration = guardIntervalDuration

class PhyModesDeliverer(object):
    """ Super class for all phy modes """
    defaultPhyMode = None
    phyModePreamble = None
    MCSs = None
    switchingPointOffset = None


class IEEE80211a(PhyModesDeliverer):
    """ Deliverer class for the basic phy modes of IEEE 802.11a """

    def __init__(self):
        self.switchingPointOffset = dB(1.0)
        self.phyModePreamble = makeBasicPhyMode("BPSK", "1/2", dB(6.0))
	self.defaultPhyMode = makeBasicPhyMode("BPSK", "1/2", dB(6.0))
	self.MCSs = [MCS("BPSK", "1/2",  dB(6.0)),
		     MCS("BPSK", "3/4",  dB(8.7)),
		     MCS("QPSK", "1/2",  dB(8.8)),
		     MCS("QPSK", "3/4",  dB(12.0)),
		     MCS("QAM16", "1/2", dB(15.4)),
		     MCS("QAM16", "3/4", dB(18.8)),
		     MCS("QAM64", "2/3", dB(23.5)),
		     MCS("QAM64", "3/4", dB(24.8))]

    def getLowest(self):
        return(makeBasicPhyMode("BPSK", "1/2", dB(6.0)))


def makeBasicPhyMode(modulation, codingRate, minSINR):
    return PhyMode(modulation = modulation,
		   codingRate = codingRate,
		   numberOfSpatialStreams = 1,
		   numberOfDataSubcarriers = 48,
		   plcpMode = "Basic",
		   guardIntervalDuration = 0.8e-6,
		   minSINR = minSINR)

