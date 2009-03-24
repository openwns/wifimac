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

class Constant:
    __plugin__ = 'Constant'
    phyModeId = None

    def __init__(self, phyModeId = 0):
        self.phyModeId = phyModeId

class Opportunistic:
    __plugin__ = 'Opportunistic'

    perForGoingDown = 0.25
    """ if the per is above this value, then the next lower phy mode will be used """
    perForGoingUp = 0.05
    """ if the per is below this value, then the next higher phy mode will be used """

class SINR(Opportunistic):
    __plugin__ = 'SINR'

    retransmissionLQMReduction = 3.0
    """ Reduce the expected lqm by this value [in dB] for every retransmission of the packet """

class SINRwithMIMO(SINR):
    __plugin__ = 'SINRwithMIMO'


class RateAdaptationConfig(object):
    raStrategy = None
    raForACKFrames = False
    ackFramesRateId = 0

    def __init__(self, **kw):
        self.raStrategy = Constant()
        openwns.pyconfig.attrsetter(self, kw)

class RateAdaptation(openwns.FUN.FunctionalUnit):
    __plugin__ = 'wifimac.lowerMAC.RateAdaptation'

    logger = None
    phyUserName = None
    managerName = None
    arqName = None
    sinrMIBServiceName = None
    perMIBServiceName = None

    myConfig = None

    def __init__(self,
                 functionalUnitName,
                 commandName,
                 phyUserName,
                 managerName,
                 arqName,
                 sinrMIBServiceName,
                 perMIBServiceName,
                 config,
                 parentLogger=None, **kw):
        super(RateAdaptation, self).__init__(functionalUnitName = functionalUnitName, commandName = commandName)
        self.logger = wifimac.Logger.Logger(name = "RateAdaptation", parent = parentLogger)
        self.phyUserName = phyUserName
        self.managerName = managerName
        self.arqName = arqName
        self.sinrMIBServiceName = sinrMIBServiceName
        self.perMIBServiceName = perMIBServiceName
        assert(config.__class__ == RateAdaptationConfig)
        self.myConfig = config
        openwns.pyconfig.attrsetter(self, kw)
