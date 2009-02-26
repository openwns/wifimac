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


class MultiBufferConfig(object):
    impatient = True
    sendSize = 10
    timeout = 0
    queueSelector = 'ConstantRRSelector'

class MultiBuffer(openwns.FUN.FunctionalUnit):

    __plugin__ = 'wifimac.lowerMAC.MultiBuffer'
    """ Name in FU Factory """

    size = None
    sizeUnit = None

    def __init__(self, functionalUnitName, commandName, config, parentLogger, size, sizeUnit, **kw):
        super(MultiBuffer, self).__init__(functionalUnitName = functionalUnitName, commandName = commandName)
        self.logger = wifimac.Logger.Logger(name = "MultiBuffer", parent = parentLogger)
        openwns.pyconfig.attrsetter(self, kw)
	self.size = size
	self.sizeUnit = sizeUnit
	assert(config.__class__ == MultiBufferConfig)
	self.myConfig = config
