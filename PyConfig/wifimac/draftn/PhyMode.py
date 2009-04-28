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

import wifimac.convergence.PhyMode

class PhyModes(wifimac.convergence.PhyMode.PhyModesDeliverer):
	""" Deliverer class for the phy modes of DraftN """

	def __init__(self):
		self.switchingPointOffset = dB(1.0)
		self.phyModePreamble = makeBasicPhyMode("BPSK", "1/2", dB(6.0))
		self.defaultPhyMode = makeBasicPhyMode("BPSK", "1/2", dB(6.0))
		self.MCSs = [wifimac.convergence.PhyMode.MCS("BPSK", "1/2",  dB(6.0)),
			     wifimac.convergence.PhyMode.MCS("QPSK", "1/2",  dB(8.8)),
			     wifimac.convergence.PhyMode.MCS("QPSK", "3/4",  dB(12.0)),
			     wifimac.convergence.PhyMode.MCS("QAM16", "1/2", dB(15.4)),
			     wifimac.convergence.PhyMode.MCS("QAM16", "3/4", dB(18.8)),
			     wifimac.convergence.PhyMode.MCS("QAM64", "2/3", dB(23.5)),
			     wifimac.convergence.PhyMode.MCS("QAM64", "3/4", dB(24.8)),
			     wifimac.convergence.PhyMode.MCS("QAM64", "5/6", dB(26.5))
			     ]

	def getLowest(self):
		return(makeBasicPhyMode("BPSK", "1/2", dB(6.0)))


def makeBasicPhyMode(modulation, codingRate, minSINR):
	return wifimac.convergence.PhyMode.PhyMode(modulation = modulation,
						   codingRate = codingRate,
						   numberOfSpatialStreams = 1,
						   numberOfDataSubcarriers = 52, #increased n_DS in comparison to basic
						   plcpMode = "HT-GF", # can also be HT-Mix
						   guardIntervalDuration = 0.8e-6, # set to 0.4e-6 for short GI
						   minSINR = minSINR)

