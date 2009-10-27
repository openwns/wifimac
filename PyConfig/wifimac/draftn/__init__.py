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

from BlockUntilReply import *
from BlockACK import *
from Aggregation import *
from DeAggregation import *
from PhyMode import *
from FastLinkFeedback import *

import wifimac.lowerMAC
import wifimac.convergence
import wifimac.FUNModes

import dll.CompoundSwitch
import openwns.Multiplexer
import openwns.FlowSeparator
import openwns.ldk

names = dict()
names['aggregation'] = 'Aggregation'
names['blockUntilReply'] = 'BlockUntilReply'
names['deAggregation'] = 'DeAggregation'
names['fastLinkFeedback'] = 'FastLinkFeedback'

class FUNTemplate(wifimac.FUNModes.Basic):

    def __init__(self, logger, transceiverAddress, upperConvergenceName):
        super(FUNTemplate, self).__init__(logger, transceiverAddress, upperConvergenceName)
        self.names.update(names)

    def createLowerMAC(self, config, myFUN):
        return(getLowerMACFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))

    def createConvergence(self, config, myFUN):
        return(getConvergenceFUN(self.transceiverAddress, self.names, config, myFUN, self.logger, self.probeLocalIDs))

def getLowerMACFUN(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs =  wifimac.lowerMAC.__getTopBlock__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs)

    FUs.append(wifimac.lowerMAC.Buffer(functionalUnitName = names['buffer'] + str(transceiverAddress),
                                       commandName = names['buffer'] + 'Command',
                                       sizeUnit = config.bufferSizeUnit,
                                       size = config.bufferSize,
                                       localIDs = probeLocalIDs,
                                       probingEnabled = False,
                                       raName = names['ra'] + str(transceiverAddress),
                                       managerName = names['manager'] + str(transceiverAddress),
                                       protocolCalculatorName = 'protocolCalculator' + str(transceiverAddress),
                                       parentLogger = logger))

    FUs.append(openwns.ldk.Probe.PacketProbeBus(name = names['holDelayProbe'] + str(transceiverAddress),
                                                prefix = 'wifimac.hol',
                                                commandName = names['holDelayProbe'] + 'Command',
                                                parentLogger = logger,
                                                moduleName = 'WiFiMAC'))

    FUs.append(BlockACK(functionalUnitName = names['arq'] + str(transceiverAddress),
                        commandName = names['arq'] + 'Command',
                        managerName = names['manager'] + str(transceiverAddress),
                        rxStartEndName = names['frameSynchronization'] + str(transceiverAddress),
                        txStartEndName = names['phyUser'] + str(transceiverAddress),
                        perMIBServiceName = names['perMIB'] + str(transceiverAddress),
                        sendBufferName = names['buffer'] + str(transceiverAddress),
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
    agg = Aggregation(functionalUnitName = names['aggregation'] + str(transceiverAddress),
                      commandName = names['aggregation'] + 'Command',
                      managerName = names['manager'] + str(transceiverAddress),
                      config = config.aggregation,
                      probePrefix = 'wifimac.aggregation',
                      parentLogger = logger,
                      localIDs = probeLocalIDs)

    ra = wifimac.lowerMAC.RateAdaptation(functionalUnitName = names['ra'] + str(transceiverAddress),
                                         commandName = names['ra'] + 'Command',
                                         phyUserName = names['phyUser'] + str(transceiverAddress),
                                         managerName = names['manager'] + str(transceiverAddress),
                                         arqName = names['arq'] + str(transceiverAddress),
                                         sinrMIBServiceName = names['sinrMIB'] + str(transceiverAddress),
                                         perMIBServiceName = names['perMIB'] + str(transceiverAddress),
                                         config = config.ra,
                                         parentLogger = logger)


    txop = wifimac.lowerMAC.TXOP(functionalUnitName = names['txop'] + str(transceiverAddress),
                                 commandName = names['txop'] + 'Command',
                                 managerName = names['manager'] +  str(transceiverAddress),
                                 protocolCalculatorName = 'protocolCalculator' + str(transceiverAddress),
                                 txopWindowName = names['buffer'] + str(transceiverAddress),
                                 raName = names['ra'] + str(transceiverAddress),
                                 probePrefix = 'wifimac.txop',
                                 localIDs = probeLocalIDs,
                                 config = config.txop,
                                 parentLogger = logger)

    rtscts = wifimac.lowerMAC.RTSCTS(functionalUnitName = names['rtscts'] + str(transceiverAddress),
                                     commandName = names['rtscts'] + 'Command',
                                     managerName = names['manager'] + str(transceiverAddress),
                                     phyUserName = names['phyUser'] + str(transceiverAddress),
                                     protocolCalculatorName = 'protocolCalculator' + str(transceiverAddress),
                                     arqName = names['arq'] + str(transceiverAddress),
                                     navName = names['channelState'] + str(transceiverAddress),
                                     rxStartName = names['frameSynchronization'] + str(transceiverAddress),
                                     txStartEndName = names['phyUser'] + str(transceiverAddress),
                                     config = config.rtscts,
                                     parentLogger = logger)

    raACK = wifimac.lowerMAC.RateAdaptation(functionalUnitName = names['ra'] + 'ACK'+ str(transceiverAddress),
                                            commandName = names['ra'] + 'ACK' +'Command',
                                            phyUserName = names['phyUser'] + str(transceiverAddress),
                                            managerName = names['manager'] + str(transceiverAddress),
                                            arqName = names['arq'] + str(transceiverAddress),
                                            sinrMIBServiceName = names['sinrMIB'] + str(transceiverAddress),
                                            perMIBServiceName = names['perMIB'] + str(transceiverAddress),
                                            config = config.ra,
                                            parentLogger = logger)

    ackMux = openwns.Multiplexer.Dispatcher(commandName = 'ackMuxCommand',
                                            functionalUnitName = 'ackMux' + str(transceiverAddress),
                                            opcodeSize = 0,
                                            parentLogger = logger,
                                            logName = 'ackMux',
                                            moduleName = 'WiFiMAC')

    fastLinkFeedback = FastLinkFeedback(fuName = names['fastLinkFeedback'] + str(transceiverAddress),
                                        commandName = names['fastLinkFeedback'] + 'Command',
                                        managerName = names['manager'] +  str(transceiverAddress),
                                        phyUserCommandName = names['phyUser'] + 'Command',
                                        sinrMIBServiceName = names['sinrMIB'] + str(transceiverAddress),
                                        config = config.fastLinkFeedback,
                                        parentLogger = logger)

    block = BlockUntilReply(fuName = names['blockUntilReply'] + str(transceiverAddress),
                            commandName = names['blockUntilReply'] + 'Command',
                            managerName = names['manager'] +  str(transceiverAddress),
                            rxStartEndName = names['frameSynchronization'] + str(transceiverAddress),
                            txStartEndName = names['deAggregation'] + str(transceiverAddress),
                            config = config.blockUntilReply,
                            parentLogger = logger)

    for fu in FUs:
            myFUN.add(fu)

    # connect FUs with each other
    for num in xrange(0, len(FUs)-1):
        FUs[num].connect(FUs[num+1])
    # DraftN requires special structure to send (Block)ACKs via a different way
    for fu in [ackSwitch, agg, ra, raACK, txop, rtscts, ackMux, fastLinkFeedback, block]:
        myFUN.add(fu)
    # DraftN requires special handling so that the acks are not send through the aggregation path
    ackSwitch.connectOnDataFU(FUs[-1], dll.CompoundSwitch.FilterAll('All'))
    ackSwitch.connectSendDataFU(raACK, wifimac.helper.Filter.FrameType('ACK', names['manager'] + 'Command'))
    ackSwitch.connectSendDataFU(agg, dll.CompoundSwitch.FilterAll('All'))
    agg.connect(ra)
    ra.connect(txop)
    txop.connect(rtscts)
    rtscts.connect(ackMux)
    raACK.connect(ackMux)
    if(config.useFastLinkFeedback):
        ackMux.connect(fastLinkFeedback)
        fastLinkFeedback.connect(block)
    else:
        ackMux.connect(block)

    bottomFU = None
    bottomFU = __appendDraftNTimingBlock__(myFUN, block, config, names, transceiverAddress, logger, probeLocalIDs)

    # Final FU: FlowGate -> Filter all frames not addressed to me
    gate = openwns.FlowSeparator.FlowGate(fuName = names['rxFilter'] + str(transceiverAddress),
                                      commandName = names['rxFilter'] + 'Command',
                                      keyBuilder = wifimac.helper.Keys.LinkByTransmitter,
                                      parentLogger = logger)
    gate.logger.moduleName = 'WiFiMAC'
    myFUN.add(gate)
    bottomFU.connect(gate)

    return([FUs[0], gate])

def __appendDraftNTimingBlock__(myFUN, bottomFU, config, names, transceiverAddress, logger, probeLocalIDs):
    unicastScheduler = wifimac.lowerMAC.DCF(fuName = names['unicastDCF'] + str(transceiverAddress),
                                            commandName = names['unicastDCF'] + 'Command',
                                            csName = names['channelState'] + str(transceiverAddress),
                                            arqCommandName = names['arq'] + 'Command',
                                            config = config.unicastDCF,
                                            parentLogger = logger)
    sifsDelay = openwns.ldk.Tools.ConstantDelay(delayDuration = config.sifsDuration,
                                               functionalUnitName = names['sifsDelay'] + str(transceiverAddress),
                                               commandName = names['sifsDelay'] + 'Command',
                                               logName = names['sifsDelay'],
                                               moduleName = 'WiFiMAC',
                                               parentLogger = logger)
    #############
    # Dispatcher
    # Multiplexing everything for the backoff
    backoffDispatcher = openwns.Multiplexer.Dispatcher(commandName = 'BODispatcherCommand',
                                                   functionalUnitName = 'BODispatcher' + str(transceiverAddress),
                                                   opcodeSize = 0,
                                                   parentLogger = logger,
                                                   logName = 'boDispatcher',
                                                   moduleName = 'WiFiMAC')

    # Multiplexing everything for the SIFS delay
    SIFSDispatcher = openwns.Multiplexer.Dispatcher(commandName = 'SIFSDispatcherCommand',
                                                functionalUnitName = 'SIFSDispatcher' + str(transceiverAddress),
                                                opcodeSize = 0,
                                                parentLogger = logger,
                                                logName = 'SIFSDispatcher',
                                                moduleName = 'WiFiMAC')
    # RxFilter multiplexer
    rxFilterDispatcher = openwns.Multiplexer.Dispatcher(commandName = 'RxFilterDispatcherCommand',
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
    for fu in [unicastScheduler, sifsDelay, backoffDispatcher, SIFSDispatcher, rxFilterDispatcher, frameSwitch]:
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
    frameSwitch.connectSendDataFU(backoffDispatcher, wifimac.helper.Filter.FrameType('DATA', names['manager'] + 'Command'))

    return(rxFilterDispatcher)

def getConvergenceFUN(transceiverAddress, names, config, myFUN, logger, probeLocalIDs):
    FUs = wifimac.convergence.__upperPart__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs)

    FUs.append(wifimac.draftn.DeAggregation(name = names['deAggregation'] + str(transceiverAddress),
                                            commandName = names['txDuration'] + 'Command',
                                            protocolCalculatorName = 'protocolCalculator' + str(transceiverAddress),
                                            managerName = names['manager'] + str(transceiverAddress),
                                            phyUserName = names['phyUser'] + str(transceiverAddress),
                                            aggregationCommandName = 'AggregationCommand',
                                            parentLogger = logger))
    FUs.extend(wifimac.convergence.__lowerPart__(transceiverAddress, names, config, myFUN, logger, probeLocalIDs))

    # add created FUs to FUN
    for fu in FUs:
        myFUN.add(fu)

    # connect FUs with each other
    for num in xrange(0, len(FUs)-1):
        FUs[num].connect(FUs[num+1])

    return([FUs[0], FUs[-1]])
