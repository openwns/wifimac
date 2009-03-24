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

class ManagerConfig(object):
	sifsDuration = 16E-6
	expectedACKDuration = 44E-6
	numAntennas = 1
	""" number of antennas for MIMO """
	msduLifetimeLimit = 0
	""" maximum msdu lifetime, set to zero for unlimited lifetime """

class Manager(openwns.FUN.FunctionalUnit):
	__plugin__ = 'wifimac.lowerMAC.Manager'

	logger = None
	myMACAddress = None
	phyDataTransmission = None
	phyNotification = None
	phyCarrierSense = None
	phyUserName = None
	channelStateName = None
	receiveFilterName = None
	upperConvergenceCommandName = None

	myConfig = None

	def __init__(self,
		     functionalUnitName,
		     commandName,
		     phyUserName,
		     channelStateName,
		     receiveFilterName,
		     upperConvergenceCommandName,
		     config,
		     parentLogger = None, **kw):
		super(Manager, self).__init__(functionalUnitName=functionalUnitName,
						      commandName=commandName)
		self.logger = wifimac.Logger.Logger(name = "Manager", parent = parentLogger)
		self.phyUserName = phyUserName
		self.channelStateName = channelStateName
		self.receiveFilterName = receiveFilterName
		self.upperConvergenceCommandName = upperConvergenceCommandName

		assert(config.__class__ == ManagerConfig)
		self.myConfig = config

		openwns.pyconfig.attrsetter(self, kw)

	def setMACAddress(self, address):
		if self.myMACAddress is not None: raise AssertionError, "Do you really want to re-set the MACAddress?"
		self.myMACAddress = address

	def setPhyDataTransmission(self, serviceName):
		self.phyDataTransmission = serviceName

	def setPhyNotification(self, serviceName):
		self.phyNotification = serviceName

	def setPhyCarrierSense(self, serviceName):
		self.phyCarrierSense = serviceName


