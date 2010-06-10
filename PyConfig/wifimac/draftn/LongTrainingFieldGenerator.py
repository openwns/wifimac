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

import wifimac.Logger

class LongTrainingFieldGeneratorConfig(object):
    ltfDuration = 4e-6
    """ duration of a single Long Training Field"""

    reducePreambleByDuration = True
    """ Reduce the preamble duration by the duration needed for the LTFs -
        simplifies preamble length calculation
    """

class LongTrainingFieldGenerator(openwns.FUN.FunctionalUnit):
    logger = None
    phyUserName = None
    managerName = None
    protocolCalculatorName = None
    txDurationSetterName = None
    sinrMIBServiceName = None
    myConfig = None
    __plugin__ = 'wifimac.draftn.LongTrainingFieldGenerator'

    

    def __init__(self,
                 name,
                 commandName,
                 phyUserName,
                 managerName,
                 protocolCalculatorName,
                 txDurationSetterName,
                 sinrMIBServiceName,
                 config,
                 parentLogger = None, **kw):
        super(LongTrainingFieldGenerator, self).__init__(functionalUnitName=name, commandName=commandName)
        self.phyUserName = phyUserName
        self.managerName = managerName
        self.protocolCalculatorName = protocolCalculatorName
        self.txDurationSetterName = txDurationSetterName
        self.sinrMIBServiceName = sinrMIBServiceName
        assert(isinstance(config, LongTrainingFieldGeneratorConfig))
        self.myConfig = config
        self.logger = wifimac.Logger.Logger("LongTrainingFieldGenerator", parent = parentLogger)
        openwns.pyconfig.attrsetter(self, kw)

