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

from Duration import Duration
from FrameLength import FrameLength

import wifimac.Logger

import openwns.node
import openwns.FUN
import openwns.pyconfig

names = dict()
names['protocolCalculator'] = 'protocolCalculator'

class Service(object):
	nameInServiceFactory  = None
	serviceName = None

class Config:
	duration = None
	frameLength = None

	def __init__(self, dur = None, fl = None):
		if(dur is None):
			self.duration = Duration(fl)
		else:
			self.duration = dur(fl)

		if(fl is None):
			self.frameLength = FrameLength()
		else:
			self.frameLength = fl

class ProtocolCalculator(Service):
	logger  = None
	myConfig = None

	def __init__(self, serviceName, config, parentLogger=None, **kw):
		self.nameInServiceFactory = 'wifimac.management.ProtocolCalculator'
        	self.serviceName = serviceName
		self.myConfig = config

        	openwns.pyconfig.attrsetter(self, kw)
        	self.logger = wifimac.Logger.Logger(name = 'PC', parent = parentLogger)
