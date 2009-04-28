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

import openwns.node
import openwns.FUN
import openwns.pyconfig
import openwns.Probe

import wifimac.Logger

class Service(object):
	nameInServiceFactory  = None
	serviceName = None

class SINR(Service):
	logger  = None
	windowSize = None

	def __init__(self, serviceName, windowSize = 1.0, parentLogger=None, **kw):
		self.nameInServiceFactory = 'wifimac.management.SINRInformationBase'
        	self.serviceName = serviceName
		self.windowSize = windowSize
        	self.logger = wifimac.Logger.Logger(name = 'SINR-MIB', parent = parentLogger)
        	openwns.pyconfig.attrsetter(self, kw)

class PERConfig(object):
		windowSize = 1.0
		minSamples = 10
		frameSizeThreshold = 10*8

		def __init__(self, **kw):
			openwns.pyconfig.attrsetter(self, kw)

class PER(Service):
	logger = None
	myConfig = None

	def __init__(self, serviceName, config, parentLogger=None, **kw):
		self.nameInServiceFactory = 'wifimac.management.PERInformationBase'
		self.serviceName = serviceName
		assert(config.__class__ == PERConfig)
		self.myConfig = config
		self.logger = wifimac.Logger.Logger(name = 'PER-MIB', parent = parentLogger)
		openwns.pyconfig.attrsetter(self, kw)

class CIBDefaultValues(object):
	readOutActive = None
	entries = None

	def __init__(self):
		self.readOutActive = False
		self.entries = []

	def add(self, key, value):
		assert(self.readOutActive == False)
		self.entries.append([key, value])

	def get(self, i):
		self.readOutActive = True
		assert(i < len(self.entries))
		return(self.entries[i])

	def size(self):
		return(len(self.entries))

class VirtualCIB(openwns.node.Component):
	nameInComponentFactory = "wifimac.management.VirtualCapabilityInformationBase"

	logger = None
	defaultValues = None

	def __init__(self, node, parentLogger=None):
		super(VirtualCIB, self).__init__(node, "VCIB")
		self.defaultValues = CIBDefaultValues()
		self.logger = wifimac.Logger.Logger(name="VCIB", parent=parentLogger)

class VirtualCababilityInformationService(openwns.node.Node):
	vcib = None
	def __init__(self, name):
		super(VirtualCababilityInformationService, self).__init__(name)
		self.vcib = VirtualCIB(node = self, parentLogger=self.logger)
