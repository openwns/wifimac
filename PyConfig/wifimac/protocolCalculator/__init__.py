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
from ErrorProbability import ErrorProbability
from MIMO import MIMO
from PhyMode import *
from Throughput import Throughput
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

class ProtocolCalculator(Service):
    logger  = None
    windowSize = None
    errorProbability = None

    def __init__(self, serviceName, parentLogger=None, **kw):
        self.nameInServiceFactory = 'wifimac.management.ProtocolCalculator'
        self.serviceName = serviceName
        self.logger = wifimac.Logger.Logger(name = 'PC', parent = parentLogger)
        self.errorProbability = ErrorProbability(guardInterval = 0.8e-6)
        self.duration = Duration(guardInterval = 0.8e-6)
        self.frameLength = FrameLength()
        openwns.pyconfig.attrsetter(self, kw)
