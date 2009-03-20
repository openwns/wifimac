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

class NextAggFrameGetter(openwns.FUN.FunctionalUnit):
    __plugin__ = 'wifimac.lowerMAC.NextAggFrameGetter'

    bufferName = None
    protocolCalculatorName = None
    
    def __init__(self, functionalUnitName, commandName, bufferName, protocolCalculatorName, **kw):
        super(NextAggFrameGetter, self).__init__(functionalUnitName = functionalUnitName, commandName = commandName)
	self.bufferName = bufferName
	self.protocolCalculatorName = protocolCalculatorName
        openwns.pyconfig.attrsetter(self, kw)
