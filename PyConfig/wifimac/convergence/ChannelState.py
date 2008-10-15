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
import wns.FUN
import wns.Probe
from wns.PyConfig import Sealed

import wifimac.Logger

from wns import dBm

class ChannelStateConfig(Sealed):
	useRawEnergyDetection = False
	usePhyCarrierSense = True
	usePhyPacketLength = True
	useNAV = True
	useOwnTx = True
	useOwnRx = True

	rawEnergyThreshold = dBm(-62)
	phyCarrierSenseThreshold = dBm(-82)

	""" To probe the channel busy fraction """
	windowSize = 1.0
	sampleInterval = 1.0

	sifsDuration = 16E-6
	preambleProcessingDelay = 21E-6
	slotDuration = 9E-6
	expectedCTSDuration = 44E-6

class ChannelState(wns.Probe.Probe):
	__plugin__ = 'wifimac.convergence.ChannelState'

	logger = None

	myConfig = None

	managerName = None
	phyUserCommandName = None
	rtsctsCommandName = None
	txStartEndName = None
	rxStartEndName = None

	""" To probe the channel busy fraction """
	busyFractionProbeName = None
	ownTxFractionProbeName = None

	def __init__(self, name, commandName, managerName, phyUserCommandName, rtsctsCommandName, txStartEndName, rxStartEndName, probePrefix, config, parentLogger = None, **kw):
		super(ChannelState, self).__init__(name=name, commandName=commandName)
		self.logger = wifimac.Logger.Logger(name = "ChannelState", parent = parentLogger)
		assert(config.__class__ == ChannelStateConfig)
		self.myConfig = config
		self.managerName = managerName
		self.phyUserCommandName = phyUserCommandName
		self.rtsctsCommandName = rtsctsCommandName
		self.txStartEndName = txStartEndName
		self.rxStartEndName = rxStartEndName
		self.busyFractionProbeName = probePrefix + ".busy"
		self.ownTxFractionProbeName = probePrefix + ".tx"

		wns.PyConfig.attrsetter(self, kw)


