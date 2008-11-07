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

class ErrorModelling(wns.FUN.FunctionalUnit):
	"""This class maps the cir to ser and calculates PER"""
	name = 'ErrorModelling'
	logger = None
	phyUserCommandName = None
	managerCommandName = None
	cyclicPrefixReduction = 0.8
	__plugin__ = 'wifimac.convergence.ErrorModelling'

	def __init__(self,
		     name,
		     commandName,
		     phyUserCommandName,
		     managerCommandName,
		     parentLogger = None, **kw):
		super(ErrorModelling, self).__init__(functionalUnitName=name, commandName=commandName)
		self.phyUserCommandName = phyUserCommandName
		self.managerCommandName = managerCommandName
		self.logger = wifimac.Logger.Logger("ErrorModelling", parent = parentLogger)
		wns.PyConfig.attrsetter(self, kw)
