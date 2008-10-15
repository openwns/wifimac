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
    if(config.forwardingEnabled):
        FUs.append(BeaconLinkQualityMeasurement(fuName = names['beaconLQM'] + str(transceiverAddress),
                                                commandName = names['beaconLQM'] + 'Command',
                                                managerName = names['manager'] + str(transceiverAddress),
                                                beaconInterval = FUs[-1].myConfig.period,
                                                phyUserCommandName = names['phyUser'] + 'Command',
                                                probePrefix = 'wifimac.linkQuality',
                                                sinrMIBServiceName = 'wifimac.sinrMIB.' + str(transceiverAddress),
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
