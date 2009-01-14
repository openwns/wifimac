from DCF import *
from Manager import *
from NextFrameGetter import *
from RTSCTS import *
from RateAdaptation import *
from StopAndWaitARQ import *
from TXOP import *
from DuplicateFilter import *

import wifimac.helper.Keys
import wifimac.helper.Filter

import dll.CompoundSwitch

import wns.Buffer
import wns.FlowSeparator
import wns.Multiplexer
import wns.ldk
import wns.Probe

names = dict()
names['manager'] = 'Manager'
names['rxFilter'] = 'RxFilter'
names['overhead'] = 'Overhead'
names['p2pWindowProbe'] = 'p2pWindowProbe'
names['buffer'] = 'Buffer'
names['DuplicateFilter'] = 'DuplicateFilter'
names['arq'] = 'ARQ'
names['ra'] = 'RateAdaptation'
names['ackSwitch'] = 'ACKSwitch'
names['nextFrame'] = 'nextFrameGetter'
names['rtscts'] = 'RTSCTS'
names['unicastDCF'] = 'UnicastDCF'
names['sifsDelay'] = 'SIFSDelay'
names['txop']  = 'TXOP'

def getFUN(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs = __getTopBlock__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs)

    FUs.append(DuplicateFilter(functionalUnitName = names['DuplicateFilter'] + str(transceiverAddress),
                               commandName =  names['DuplicateFilter'] + 'Command',
                               managerName = names['manager'] + str(transceiverAddress),
                               arqCommandName = names['arq'] + 'Command',
                               parentLogger = logger))

    ra = RateAdaptation(functionalUnitName = names['ra'] + str(transceiverAddress),
                        commandName = names['ra'] + 'Command',
                        phyUserName = names['phyUser'] + str(transceiverAddress),
                        managerName = names['manager'] + str(transceiverAddress),
                        arqName = names['arq'] + str(transceiverAddress),
                        sinrMIBServiceName = 'wifimac.sinrMIB.' + str(transceiverAddress),
                        perMIBServiceName = 'wifimac.perMIB.' + str(transceiverAddress),
                        config = config.ra,
                        parentLogger = logger)

    nextFrame = NextFrameGetter(functionalUnitName = names['nextFrame'] + str(transceiverAddress),
                                                 commandName = names['nextFrame'] + 'Command')

    arq = StopAndWaitARQ(fuName = names['arq'] + str(transceiverAddress),
                         commandName = names['arq'] + 'Command',
                         managerName = names['manager'] + str(transceiverAddress),
                         csName = names['channelState'] + str(transceiverAddress),
                         rxStartName = names['frameSynchronization'] + str(transceiverAddress),
                         txStartEndName = names['phyUser'] + str(transceiverAddress),
                         perMIBServiceName = 'wifimac.perMIB.' + str(transceiverAddress),
                         probePrefix = 'wifimac.linkQuality',
                         config = config.arq,
                         parentLogger = logger,
                         localIDs = probeLocalIDs)

    txop = TXOP(functionalUnitName = names['txop'] + str(transceiverAddress),
                                 commandName = names['txop'] + 'Command',
                                 managerName = names['manager'] +  str(transceiverAddress),
                                 phyUserName =  names['phyUser'] + str(transceiverAddress),
                                 nextFrameHolderName = names['nextFrame'] + str(transceiverAddress),
                                 config = config.txop,
                                 parentLogger = logger)

    ## connect the common part for all FUs
    # add created FUs to FUN
    FUs.extend([arq, ra, nextFrame, txop])

    for fu in FUs:
            myFUN.add(fu)

    # connect FUs with each other
    for num in xrange(0, len(FUs)-1):
            FUs[num].connect(FUs[num+1])

    bottomFU = None
    bottomFU = __appendBasicTimingBlock__(myFUN, txop, config, names, transceiverAddress, logger, probeLocalIDs)

    # Final FU: FlowGate -> Filter all frames not addressed to me
    gate = wns.FlowSeparator.FlowGate(fuName = names['rxFilter'] + str(transceiverAddress),
                                      commandName = names['rxFilter'] + 'Command',
                                      keyBuilder = wifimac.helper.Keys.LinkByTransmitter,
                                      parentLogger = logger)
    gate.logger.moduleName = 'WiFiMAC'
    myFUN.add(gate)
    bottomFU.connect(gate)

    return([FUs[0], gate])


def __getTopBlock__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs = []
    FUs.append(Manager(functionalUnitName = names['manager'] + str(transceiverAddress),
                       commandName = names['manager'] + 'Command',
                       phyUserName = names['phyUser'] + str(transceiverAddress),
                       channelStateName = names['channelState'] + str(transceiverAddress),
                       receiveFilterName = names['rxFilter'] + str(transceiverAddress),
                       upperConvergenceCommandName = names['upperConvergence'],
                       config = config.manager,
                       parentLogger = logger))
    # overhead for regular msdu
    FUs.append(wns.ldk.Tools.Overhead(functionalUnitName = names['overhead'] + str(transceiverAddress),
                                      commandName = names['overhead'] + 'Command',
                                      overhead = config.headerBits + config.crcBits))

    FUs.append(wns.Probe.WindowProbeBus(name = "wifimac.tx" + str(transceiverAddress) + ".upper",
                                        prefix = "wifimac.txUpper",
                                        commandName = 'wifimac.txUpperProbeCommand',
                                        parentLogger = logger,
                                        moduleName = 'WiFiMAC',
                                        localIDs = probeLocalIDs))

    FUs.append(wns.Buffer.Dropping(functionalUnitName = names['buffer'] + str(transceiverAddress),
                                   commandName = names['buffer'] + 'Command',
                                   sizeUnit = config.bufferSizeUnit,
                                   size = config.bufferSize,
                                   localIDs = probeLocalIDs,
                                   probingEnabled = False))
    return FUs
    

def __appendBasicTimingBlock__(myFUN, bottomFU, config, names, transceiverAddress, logger, probeLocalIDs):
    ########################################
    # Timing of DATA, RTS/CTS and ACK frames
    #############
    # Working FUs
    rtscts = RTSCTS(functionalUnitName = names['rtscts'] + str(transceiverAddress),
                    commandName = names['rtscts'] + 'Command',
                    managerName = names['manager'] + str(transceiverAddress),
                    phyUserName = names['phyUser'] + str(transceiverAddress),
                    arqName = names['arq'] + str(transceiverAddress),
                    navName = names['channelState'] + str(transceiverAddress),
                    rxStartName = names['frameSynchronization'] + str(transceiverAddress),
                    txStartEndName = names['phyUser'] + str(transceiverAddress),
                    config = config.rtscts,
                    parentLogger = logger)
    unicastScheduler = DCF(fuName = names['unicastDCF'] + str(transceiverAddress),
                           commandName = names['unicastDCF'] + 'Command',
                           csName = names['channelState'] + str(transceiverAddress),
                           arqCommandName = names['arq'] + 'Command',
                           config = config.unicastDCF,
                           parentLogger = logger)
    sifsDelay = wns.ldk.Tools.ConstantDelay(delayDuration = config.sifsDuration,
                                               functionalUnitName = names['sifsDelay'] + str(transceiverAddress),
                                               commandName = names['sifsDelay'] + 'Command',
                                               logName = names['sifsDelay'],
                                               moduleName = 'WiFiMAC',
                                               parentLogger = logger)
    #############
    # Dispatcher
    # Multiplexing everything for the backoff
    backoffDispatcher = wns.Multiplexer.Dispatcher(commandName = 'BODispatcherCommand',
                                                   functionalUnitName = 'BODispatcher' + str(transceiverAddress),
                                                   opcodeSize = 0,
                                                   parentLogger = logger,
                                                   logName = 'boDispatcher',
                                                   moduleName = 'WiFiMAC')

    # Multiplexing everything for the SIFS delay
    SIFSDispatcher = wns.Multiplexer.Dispatcher(commandName = 'SIFSDispatcherCommand',
                                                functionalUnitName = 'SIFSDispatcher' + str(transceiverAddress),
                                                opcodeSize = 0,
                                                parentLogger = logger,
                                                logName = 'SIFSDispatcher',
                                                moduleName = 'WiFiMAC')
    # RxFilter multiplexer
    rxFilterDispatcher = wns.Multiplexer.Dispatcher(commandName = 'RxFilterDispatcherCommand',
                                                    functionalUnitName = 'RxDispatcher' + str(transceiverAddress),
                                                    opcodeSize = 0,
                                                    parentLogger = logger,
                                                    logName = 'RxFilterDispatcher',
                                                    moduleName = 'WiFiMAC')

    ##########
    # Switches
    frameSwitch = dll.CompoundSwitch.CompoundSwitch(functionalUnitName = 'FrameSwitch' + str(transceiverAddress),
                                                    commandName = 'FrameSwitch' + 'Command',
                                                    logName = 'FrameSwitch',
                                                    moduleName = 'WiFiMAC',
                                                    parentLogger = logger,
                                                    mustAccept = False)
    rtsSwitch = dll.CompoundSwitch.CompoundSwitch(functionalUnitName = 'RTSSwitch' + str(transceiverAddress),
                                                  commandName = 'RTSSwitch' + 'Command',
                                                  logName = 'RTSSwitch',
                                                  moduleName = 'WiFiMAC',
                                                  parentLogger = logger,
                                                  mustAccept = False)
                    # add FUs
    ## for fu in [rtscts, unicastScheduler, sifsDelay, backoffDispatcher, SIFSDispatcher, rxFilterDispatcher, rtsSwitch, frameSwitch]:
    for fu in [rtscts, unicastScheduler, sifsDelay, backoffDispatcher, SIFSDispatcher, rxFilterDispatcher, rtsSwitch, frameSwitch]:
            myFUN.add(fu)

    # connect simple FU as far as possible
    backoffDispatcher.connect(unicastScheduler)
    SIFSDispatcher.connect(sifsDelay)
    unicastScheduler.connect(rxFilterDispatcher)
    sifsDelay.connect(rxFilterDispatcher)


    # connect switches
    frameSwitch.connectOnDataFU(bottomFU, dll.CompoundSwitch.FilterAll('All'))
    frameSwitch.connectSendDataFU(SIFSDispatcher, wifimac.helper.Filter.FrameType('ACK', names['manager'] + 'Command'))
    frameSwitch.connectSendDataFU(SIFSDispatcher, wifimac.helper.Filter.FrameType('DATA_TXOP', names['manager'] + 'Command'))
    frameSwitch.connectSendDataFU(backoffDispatcher, wifimac.helper.Filter.Size(0, config.rtsctsThreshold-1))
    frameSwitch.connectSendDataFU(rtscts, wifimac.helper.Filter.Size(config.rtsctsThreshold, config.maxFrameSize))

    rtsSwitch.connectOnDataFU(rtscts, dll.CompoundSwitch.FilterAll('All'))
    rtsSwitch.connectSendDataFU(backoffDispatcher, wifimac.helper.Filter.FrameType('DATA', names['manager'] + 'Command'))
    rtsSwitch.connectSendDataFU(SIFSDispatcher, dll.CompoundSwitch.FilterAll('All'))

    return(rxFilterDispatcher)

