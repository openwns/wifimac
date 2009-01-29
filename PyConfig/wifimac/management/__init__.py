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

from Beacon import *
from InformationBases import *

from wifimac.pathselection import BeaconLinkQualityMeasurement
from wifimac.lowerMAC import DCF

names = dict()
names['beacon'] = 'Beacon'
names['broadcastDCF'] = 'BroadcastDCF'

def getFUN(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs = []
    FUs.append(Beacon(functionalUnitName = names['beacon'] + str(transceiverAddress),
                      commandName = names['beacon'] + 'Command',
                      managerName = names['manager'] + str(transceiverAddress),
                      phyUserCommandName = names['phyUser'] + 'Command',
                      config = config.beacon,
                      parentLogger = logger))
    FUs.append(BeaconLinkQualityMeasurement(fuName = names['beaconLQM'] + str(transceiverAddress),
                                            commandName = names['beaconLQM'] + 'Command',
                                            managerName = names['manager'] + str(transceiverAddress),
                                            beaconInterval = FUs[-1].myConfig.period,
                                            phyUserCommandName = names['phyUser'] + 'Command',
                                            probePrefix = 'wifimac.linkQuality',
                                            sinrMIBServiceName = names['sinrMIB'] + str(transceiverAddress),
                                            config = config.beaconLQM,
                                            parentLogger = logger,
                                            localIDs = probeLocalIDs))

    FUs.append(DCF(fuName = names['broadcastDCF'] + str(transceiverAddress),
                   commandName = names['broadcastDCF'] + 'Command',
                   csName = names['channelState'] + str(transceiverAddress),
                   arqCommandName = names['arq'] + 'Command',
                   config = config.broadcastDCF,
                   parentLogger = logger))
    # add created FUs to FUN
    for fu in FUs:
        myFUN.add(fu)

    for num in xrange(0, len(FUs)-1):
        FUs[num].connect(FUs[num+1])

    return([FUs[0], FUs[-1]])
