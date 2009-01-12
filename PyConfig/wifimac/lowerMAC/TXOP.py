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

class TXOPConfig(object):
    sifsDuration = 16E-6
    expectedACKDuration = 44E-6
    txopLimit = 0
    singleReceiver = False

class TXOP(openwns.FUN.FunctionalUnit):

    __plugin__ = 'wifimac.lowerMAC.TXOP'
    """ Name in FU Factory """

    logger = None

    phyUserName = None
    managerName = None
    nextFrameHolderName = None

    myConfig = None

    def __init__(self, functionalUnitName, commandName, managerName, phyUserName, nextFrameHolderName, config, parentLogger = None, **kw):
        super(TXOP, self).__init__(functionalUnitName = functionalUnitName, commandName = commandName)
        self.managerName = managerName
        self.phyUserName = phyUserName
        self.nextFrameHolderName = nextFrameHolderName
        assert(config.__class__ == TXOPConfig)
        self.myConfig = config
        self.logger = wifimac.Logger.Logger(name = "TXOP", parent = parentLogger)
        openwns.pyconfig.attrsetter(self, kw)

