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

from PhyUser import *
from ChannelState import *
from TxDurationSetter import *
from PreambleGenerator import *
from FrameSynchronization import *
from ErrorModelling import *
from DeAggregation import *

import openwns.Multiplexer
import openwns.CRC

names = dict()
names['phyUser'] = 'PhyUser'
names['channelState'] = 'ChannelState'
names['frameSynchronization'] = 'FrameSynchronization'
names['crc'] = 'CRC'
names['deAggregation'] = 'DeAggregation'
names['txDuration'] = 'TxDuration'
names['errorModelling'] = 'ErrorModelling'

def getFUN(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs = __upperPart__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs)

    FUs.append(TxDurationSetter(name = names['txDuration'] + str(transceiverAddress),
                                commandName = names['txDuration'] + 'Command',
                                phyUserName = names['phyUser'] + str(transceiverAddress),
                                managerName = names['manager'] + str(transceiverAddress),
                                parentLogger = logger))

    FUs.extend(__lowerPart__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs))

    # add created FUs to FUN
    for fu in FUs:
        myFUN.add(fu)

    # connect FUs with each other
    for num in xrange(0, len(FUs)-1):
        FUs[num].connect(FUs[num+1])

    return([FUs[0], FUs[-1]])

def __upperPart__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs = []
    FUs.append(openwns.Multiplexer.Dispatcher(commandName = 'planeDispatcherCommand',
                                          functionalUnitName = 'planeDispatcher' + str(transceiverAddress),
                                          opcodeSize = 0,
                                          parentLogger = logger,
                                          logName = 'PhyDispatcher',
                                          moduleName = 'WiFiMAC'))
    FUs.append(PreambleGenerator(name = 'Preamble' + str(transceiverAddress),
                                 commandName = 'PreambleCommand',
                                 phyUserName = names['phyUser'] + str(transceiverAddress),
                                 managerName = names['manager'] + str(transceiverAddress),
                                 parentLogger = logger))
    return FUs

def __lowerPart__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs = []
    FUs.append(ChannelState(name = names['channelState'] + str(transceiverAddress),
                            commandName = names['channelState'] + 'Command',
                            managerName = names['manager'] + str(transceiverAddress),
                            phyUserCommandName = names['phyUser'] + 'Command',
                            rtsctsCommandName = names['rtscts'] + 'Command',
                            txStartEndName = names['phyUser'] + str(transceiverAddress),
                            rxStartEndName = names['frameSynchronization'] + str(transceiverAddress),
                            probePrefix = 'wifimac.channelState',
                            config = config.channelState,
                            parentLogger = logger,
                            localIDs = probeLocalIDs))
    FUs.append(FrameSynchronization(name = names['frameSynchronization'] + str(transceiverAddress),
                                    commandName = names['frameSynchronization'] + 'Command',
                                    managerName = names['manager'] + str(transceiverAddress),
                                    crcCommandName = names['crc'] + 'Command',
                                    phyUserCommandName = names['phyUser'] + 'Command',
                                    probePrefix = 'wifimac.linkQuality',
                                    sinrMIBServiceName = names['sinrMIB'] + str(transceiverAddress),
                                    config = config.frameSynchronization,
                                    parentLogger=logger,
                                    localIDs = probeLocalIDs))
    FUs.append(openwns.CRC.CRC(functionalUnitName = names['crc'] + str(transceiverAddress),
                           commandName = names['crc'] + 'Command',
                           PERProvider = names['errorModelling'] + str(transceiverAddress),
                           CRCsize = 0,
                           isDropping=False,
                           parentLogger = logger))
    FUs[-1].logger.moduleName = 'WiFiMAC'
    FUs.append(ErrorModelling(name= names['errorModelling'] + str(transceiverAddress),
                              commandName = names['errorModelling'] + 'Command',
                              phyUserCommandName = names['phyUser'] + 'Command',
                              managerCommandName = names['manager'] + 'Command',
                              parentLogger = logger))
    FUs.append(PhyUser(functionalUnitName = names['phyUser'] + str(transceiverAddress),
                       commandName = names['phyUser'] + 'Command',
                       managerName = names['manager'] + str(transceiverAddress),
                       txDurationProviderCommandName = names['txDuration'] + 'Command',
                       config = config.phyUser,
                       parentLogger = logger))
    return FUs
