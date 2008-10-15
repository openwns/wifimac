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
from wns.Sealed import Sealed

import wifimac.Logger

class BeaconConfig(Sealed):
	enabled = True
	delay = 0.1
	""" Initial start delay"""
	period = 0.1
	""" Beacon transmission period, common value is 100ms """
	scanDuration = 0.3
	""" duration of scanning(for each frequency) before association for STAs """
	scanFrequencies = None
	""" freqencies to scan for beacons """
	beaconPhyModeId = 0

	def __init__(self, **kw):
		self.scanFrequencies = []
		wns.PyConfig.attrsetter(self, kw)

class Beacon(wns.FUN.FunctionalUnit):
	__plugin__ = 'wifimac.management.Beacon'

	logger = None
	phyUserCommandName = None
	managerName = None
	myConfig = None

	def __init__(self, functionalUnitName, commandName, managerName, phyUserCommandName, config, parentLogger = None, **kw):
		super(Beacon, self).__init__(functionalUnitName = functionalUnitName, commandName=commandName)
		self.logger = wifimac.Logger.Logger(name = "Beacon", parent = parentLogger)
		self.managerName = managerName
		self.phyUserCommandName = phyUserCommandName
		assert(config.__class__ == BeaconConfig)
		self.myConfig = config
		wns.PyConfig.attrsetter(self, kw)
