from ConstantWait import *
from DCF import *
from Manager import *
from NextFrameGetter import *
from RTSCTS import *
from RateAdaptation import *
from StopAndWaitARQ import *
from TXOP import *
from DuplicateFilter import *

import wifimac.draftn
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
names['constantWait'] = 'ConstantWait'
names['txop']  = 'TXOP'

def getFUN(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
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

    FUs.append(wns.Buffer.Dropping( functionalUnitName = names['buffer'] + str(transceiverAddress),
                                    commandName = names['buffer'] + 'Command',
                                    sizeUnit = config.bufferSizeUnit,
                                    size = config.bufferSize,
                                    localIDs = probeLocalIDs,
                                    probingEnabled = False))

    if(config.mode == 'basic'):
            FUs.append(DuplicateFilter(functionalUnitName = names['DuplicateFilter'] + str(transceiverAddress),
                                                        commandName =  names['DuplicateFilter'] + 'Command',
                                                        managerName = names['manager'] + str(transceiverAddress),
                                                        arqCommandName = names['arq'] + 'Command',
                                                        parentLogger = logger))
    if(config.mode == 'DraftN'):
            FUs.append(wifimac.draftn.BlockACK(functionalUnitName = names['arq'] + str(transceiverAddress),
                                               commandName = names['arq'] + 'Command',
                                               managerName = names['manager'] + str(transceiverAddress),
                                               rxStartEndName = names['frameSynchronization'] + str(transceiverAddress),
                                               txStartEndName = names['phyUser'] + str(transceiverAddress),
                                               perMIBServiceName = 'wifimac.perMIB.' + str(transceiverAddress),
                                               probePrefix = 'wifimac.linkQuality',
                                               config = config.blockACK,
                                               parentLogger = logger,
                                               localIDs = probeLocalIDs))
            ackSwitch = dll.CompoundSwitch.CompoundSwitch(functionalUnitName = names['ackSwitch'] + str(transceiverAddress),
                                                          commandName = names['ackSwitch'] + 'Command',
                                                          logName = 'ACKSwitch',
                                                          moduleName = 'WiFiMAC',
                                                          parentLogger = logger,
                                                          mustAccept = False)
            agg = wifimac.draftn.Aggregation(functionalUnitName = names['aggregation'] + str(transceiverAddress),
                                             commandName = names['aggregation'] + 'Command',
                                             managerName = names['manager'] + str(transceiverAddress),
                                             config = config.aggregation,
                                             parentLogger = logger)

    ra = RateAdaptation(functionalUnitName = names['ra'] + str(transceiverAddress),
                                         commandName = names['ra'] + 'Command',
                                         phyUserName = names['phyUser'] + str(transceiverAddress),
                                         managerName = names['manager'] + str(transceiverAddress),
                                         sinrMIBServiceName = 'wifimac.sinrMIB.' + str(transceiverAddress),
                                         perMIBServiceName = 'wifimac.perMIB.' + str(transceiverAddress),
                                         config = config.ra,
                                         parentLogger = logger)

    nextFrame = NextFrameGetter(functionalUnitName = names['nextFrame'] + str(transceiverAddress),
                                                 commandName = names['nextFrame'] + 'Command')

    if(config.mode == 'basic'):
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

    if(config.mode == 'DraftN'):
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
            raACK = RateAdaptation(functionalUnitName = names['ra'] + 'ACK'+ str(transceiverAddress),
                                                    commandName = names['ra'] + 'ACK' +'Command',
                                                    phyUserName = names['phyUser'] + str(transceiverAddress),
                                                    managerName = names['manager'] + str(transceiverAddress),
                                                    sinrMIBServiceName = 'wifimac.sinrMIB.' + str(transceiverAddress),
                                                    perMIBServiceName = 'wifimac.perMIB.' + str(transceiverAddress),
                                                    config = config.ra,
                                                    parentLogger = logger)

            ackMux = wns.Multiplexer.Dispatcher(commandName = 'ackMuxCommand',
                                                functionalUnitName = 'ackMux' + str(transceiverAddress),
                                                opcodeSize = 0,
                                                parentLogger = logger,
                                                logName = 'ackMux',
                                                moduleName = 'WiFiMAC')

            block = wifimac.draftn.BlockUntilReply(fuName = names['blockUntilReply'] + str(transceiverAddress),
                                                   commandName = names['blockUntilReply'] + 'Command',
                                                   managerName = names['manager'] +  str(transceiverAddress),
                                                   rxStartEndName = names['frameSynchronization'] + str(transceiverAddress),
                                                   txStartEndName = names['deAggregation'] + str(transceiverAddress),
                                                   config = config.blockUntilReply,
                                                   parentLogger = logger)

    ## connect the common part for all FUs
    # add created FUs to FUN
    if(config.mode == 'basic'):
            FUs.extend([ra, nextFrame, arq, txop])
    for fu in FUs:
            myFUN.add(fu)

    if(config.mode == 'DraftN'):
            for fu in [ackSwitch, agg, ra, raACK, nextFrame, txop, rtscts, ackMux, block]:
                    myFUN.add(fu)

    # connect FUs with each other
    for num in xrange(0, len(FUs)-1):
            FUs[num].connect(FUs[num+1])

    # DraftN requires special handling so that the acks are not send through the aggregation path
    if(config.mode == 'DraftN'):
            ackSwitch.connectOnDataFU(FUs[-1], dll.CompoundSwitch.FilterAll('All'))
            ackSwitch.connectSendDataFU(raACK, wifimac.helper.Filter.FrameType('ACK', names['manager'] + 'Command'))
            ackSwitch.connectSendDataFU(agg, dll.CompoundSwitch.FilterAll('All'))
            agg.connect(ra)
            ra.connect(nextFrame)
            nextFrame.connect(txop)
            txop.connect(rtscts)
            rtscts.connect(ackMux)
            raACK.connect(ackMux)
            ackMux.connect(block)

    bottomFU = None
    if(config.mode == 'basic'):
            bottomFU = __appendBasicTimingBlock__(myFUN, txop, config, names, transceiverAddress, logger, probeLocalIDs)
    if(config.mode == 'DraftN'):
            bottomFU = __appendDraftNTimingBlock__(myFUN, block, config, names, transceiverAddress, logger, probeLocalIDs)

    # Final FU: FlowGate -> Filter all frames not addressed to me
    gate = wns.FlowSeparator.FlowGate(fuName = names['rxFilter'] + str(transceiverAddress),
                                      commandName = names['rxFilter'] + 'Command',
                                      keyBuilder = wifimac.helper.Keys.LinkByTransmitter,
                                      parentLogger = logger)
    gate.logger.moduleName = 'WiFiMAC'
    myFUN.add(gate)
    bottomFU.connect(gate)

    return([FUs[0], gate])

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
    constantWait = ConstantWait(functionalUnitName = names['constantWait'] + str(transceiverAddress),
                                                 commandName = names['constantWait'] + 'Command',
                                                 managerName = names['manager'] + str(transceiverAddress),
                                                 config = config.constantWait,
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
    ## for fu in [rtscts, unicastScheduler, constantWait, backoffDispatcher, SIFSDispatcher, rxFilterDispatcher, rtsSwitch, frameSwitch]:
    for fu in [rtscts, unicastScheduler, constantWait, backoffDispatcher, SIFSDispatcher, rxFilterDispatcher, rtsSwitch, frameSwitch]:
            myFUN.add(fu)

    # connect simple FU as far as possible
    backoffDispatcher.connect(unicastScheduler)
    SIFSDispatcher.connect(constantWait)
    unicastScheduler.connect(rxFilterDispatcher)
    constantWait.connect(rxFilterDispatcher)


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

def __appendDraftNTimingBlock__(myFUN, bottomFU, config, names, transceiverAddress, logger, probeLocalIDs):
    unicastScheduler = DCF(fuName = names['unicastDCF'] + str(transceiverAddress),
                                            commandName = names['unicastDCF'] + 'Command',
                                            csName = names['channelState'] + str(transceiverAddress),
                                            arqCommandName = names['arq'] + 'Command',
                                            config = config.unicastDCF,
                                            parentLogger = logger)
    constantWait = ConstantWait(functionalUnitName = names['constantWait'] + str(transceiverAddress),
                                                 commandName = names['constantWait'] + 'Command',
                                                 managerName = names['manager'] + str(transceiverAddress),
                                                 config = config.constantWait,
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
    # add FUs
    for fu in [unicastScheduler, constantWait, backoffDispatcher, SIFSDispatcher, rxFilterDispatcher, frameSwitch]:
            myFUN.add(fu)

    # connect simple FU as far as possible
    backoffDispatcher.connect(unicastScheduler)
    SIFSDispatcher.connect(constantWait)
    unicastScheduler.connect(rxFilterDispatcher)
    constantWait.connect(rxFilterDispatcher)


    # connect switches
    frameSwitch.connectOnDataFU(bottomFU, dll.CompoundSwitch.FilterAll('All'))
    frameSwitch.connectSendDataFU(SIFSDispatcher, wifimac.helper.Filter.FrameType('ACK', names['manager'] + 'Command'))
    frameSwitch.connectSendDataFU(SIFSDispatcher, wifimac.helper.Filter.FrameType('DATA_TXOP', names['manager'] + 'Command'))
    frameSwitch.connectSendDataFU(backoffDispatcher, wifimac.helper.Filter.FrameType('DATA', names['manager'] + 'Command'))

    return(rxFilterDispatcher)
