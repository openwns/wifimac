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
import wns.Node
import wns.FUN
import wns.PyConfig
import wns.Probe
from wns.PyConfig import Sealed
from wns import dBm, dB, fromdB

import wifimac.Logger

class MeshForwarding(wns.FUN.FunctionalUnit):
	""" Forwarding based on association (STA <-> AP) and mesh-routing"""

	__plugin__ = 'wifimac.pathselection.MeshForwarding'
	logger = None
	upperConvergenceName = None
	dot11MeshTTL = None
	pathSelectionServiceName = None

	def __init__(self, functionalUnitName, commandName, upperConvergenceName, parentLogger=None, **kw):
		super(MeshForwarding, self).__init__(functionalUnitName=functionalUnitName, commandName=commandName)
		self.upperConvergenceName = upperConvergenceName
		self.logger = wifimac.Logger.Logger(name = "MeshForwarding", parent = parentLogger)
		self.dot11MeshTTL = 15
		self.pathSelectionServiceName = 'PATHSELECTIONOVERVPS'
		wns.PyConfig.attrsetter(self, kw)

class StationForwarding(wns.FUN.FunctionalUnit):
	""" Forwarding for STAs: Always to the associated AP"""

	__plugin__ = 'wifimac.pathselection.StationForwarding'
	logger = None
	upperConvergenceName = None

	def __init__(self, functionalUnitName, commandName, upperConvergenceName, parentLogger=None, **kw):
		super(StationForwarding, self).__init__(functionalUnitName=functionalUnitName, commandName=commandName)
		self.upperConvergenceName = upperConvergenceName
		self.logger = wifimac.Logger.Logger(name = "STAForwarding", parent = parentLogger)
		wns.PyConfig.attrsetter(self, kw)
