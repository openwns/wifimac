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

import openwns.FUN
import openwns.pyconfig

import wifimac.Logger

import wifimac.convergence.PhyMode

from openwns import dB, fromdB

class PhyUserConfig(object):
	initFrequency = None
	initBandwidthMHz = 20

	# Turnaround time must be below 2E-6 for OFDM-Phy, see IEEE 802.11-2007, Table 17-15
	txrxTurnaroundDelay = 1E-6
	phyModesDeliverer = None

	def __init__(self, initFrequency, phyModesDeliverer = None):
		self.initFrequency = initFrequency
		if phyModesDeliverer is None:
			self.phyModesDeliverer = wifimac.convergence.PhyMode.IEEE80211a()
		else:
			self.phyModesDeliverer = phyModesDeliverer()

class PhyUser(openwns.FUN.FunctionalUnit):
	__plugin__ = 'wifimac.convergence.PhyUser'
	"""Name in FunctionalUnitFactory"""

	logger = None

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

		openwns.pyconfig.attrsetter(self, kw)

