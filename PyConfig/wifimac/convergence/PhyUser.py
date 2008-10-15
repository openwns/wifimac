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
import wns.PyConfig
from wns.PyConfig import Sealed

import wifimac.Logger

import wifimac.convergence.PhyMode

from wns import dB, fromdB

class PhyUserConfig(Sealed):
	initFrequency = None

	numPhyModes = 8
	symbolDuration = 4E-6
	PLCPPreambleDuration = 16E-6
	PLCPServiceBits = 16
	PLCPTailBits = 6
	# Turnaround time must be below 2E-6 for OFDM-Phy, see IEEE 802.11-2007, Table 17-15
	txrxTurnaroundDelay = 1E-6
	bandwidth = 20
	phyModes = None

	switchingPointOffset = dB(0.0)

	def __init__(self, initFrequency):
		self.initFrequency = initFrequency
		self.phyModes = wifimac.convergence.PhyMode.PhyModesImproved

class PhyUser(wns.FUN.FunctionalUnit):
	__plugin__ = 'wifimac.convergence.PhyUser'
	"""Name in FunctionalUnitFactory"""


	logger = None
	# Declaration of PHY-Modes
	PhyModePreamble = None
	PhyMode0 = None
	PhyMode1 = None
	PhyMode2 = None
	PhyMode3 = None
	PhyMode4 = None
	PhyMode5 = None
	PhyMode6 = None
	PhyMode7 = None

	managerName = None
	txDurationProviderCommandName = None

	myConfig = None

	def __init__(self,
		     functionalUnitName,
		     commandName,
		     managerName,
		     txDurationProviderCommandName,
		     config,
		     parentLogger = None,
		     **kw):
		super(PhyUser, self).__init__(functionalUnitName = functionalUnitName, commandName = commandName)
		self.logger = wifimac.Logger.Logger("PhyUser", parentLogger)
		self.managerName = managerName
		self.txDurationProviderCommandName = txDurationProviderCommandName

		assert(config.__class__ == PhyUserConfig)
		self.myConfig = config

		phyModes = config.phyModes(self.logger)
		wns.PyConfig.attrsetter(self, kw)

		self.PhyModePreamble = phyModes.getPhyModePreamble() 
		self.PhyMode0 = phyModes.getPhyMode0()
		self.PhyMode1 = phyModes.getPhyMode1()
		self.PhyMode2 = phyModes.getPhyMode2()
		self.PhyMode3 = phyModes.getPhyMode3()
		self.PhyMode4 = phyModes.getPhyMode4()
		self.PhyMode5 = phyModes.getPhyMode5()
		self.PhyMode6 = phyModes.getPhyMode6()
		self.PhyMode7 = phyModes.getPhyMode7()

