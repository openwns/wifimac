from PhyUser import *
from ChannelState import *
from TxDurationSetter import *
from PreambleGenerator import *
from FrameSynchronization import *
from ErrorModelling import *
from DeAggregation import *

import wns.Multiplexer
import wns.CRC

names = dict()
names['phyUser'] = 'PhyUser'
names['channelState'] = 'ChannelState'
names['frameSynchronization'] = 'FrameSynchronization'
names['crc'] = 'CRC'
names['deAggregation'] = 'DeAggregation'
names['txDuration'] = 'TxDuration'
names['errorModelling'] = 'ErrorModelling'

def getFUN(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs = []
    FUs.append(wns.Multiplexer.Dispatcher(commandName = 'planeDispatcherCommand',
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
    if(config.mode == 'basic'):
        FUs.append(TxDurationSetter(name = names['txDuration'] + str(transceiverAddress),
                                    commandName = names['txDuration'] + 'Command',
                                    phyUserName = names['phyUser'] + str(transceiverAddress),
                                    managerName = names['manager'] + str(transceiverAddress),
                                    parentLogger = logger))
    if(config.mode == 'DraftN'):
        FUs.append(DeAggregation(name = names['deAggregation'] + str(transceiverAddress),
                                 commandName = names['txDuration'] + 'Command',
                                 phyUserName = names['phyUser'] + str(transceiverAddress),
                                 managerName = names['manager'] + str(transceiverAddress),
                                 aggregationCommandName = 'AggregationCommand',
                                 parentLogger = logger))
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
                                    sinrMIBServiceName = 'wifimac.sinrMIB.' + str(transceiverAddress),
                                    config = config.frameSynchronization,
                                    parentLogger=logger,
                                    localIDs = probeLocalIDs))
    FUs.append(wns.CRC.CRC(functionalUnitName = names['crc'] + str(transceiverAddress),
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

    # add created FUs to FUN
    for fu in FUs:
        myFUN.add(fu)

    # connect FUs with each other
    for num in xrange(0, len(FUs)-1):
        FUs[num].connect(FUs[num+1])

    return([FUs[0], FUs[-1]])
