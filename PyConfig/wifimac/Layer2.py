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
import wifimac.convergence
import wifimac.draftn
import wifimac.lowerMAC
import wifimac.pathselection
import wifimac.management
import wifimac.protocolCalculator
import wifimac.FUNModes
import wifimac.helper.Probes

import openwns.FUN
import openwns.ldk
from openwns.pyconfig import Sealed

import dll.Layer2
import dll.UpperConvergence
import dll.Association
import dll.Services
import dll.CompoundSwitch

class dllSTA(dll.Layer2.Layer2):
    manager = None

    def __init__(self, node, name, config, parentLogger):
        super(dllSTA, self).__init__(node, name, parentLogger)

        self.nameInComponentFactory = "wifimac.Layer2"
        self.associations = []
        self.stationType = "UT"
        self.ring = 3
        self.fun = openwns.FUN.FUN()

        ################
        # Upper MAC part
        e2eThroughputProbe =  wifimac.helper.Probes.DestinationSortedWindowProbeBus(name = "wifimac.e2eWindowProbe",
                                                                                    prefix = "wifimac.e2e",
                                                                                    commandName = 'e2eWindowProbeCommand',
                                                                                    ucCommandName = self.upperConvergenceName,
                                                                                    parentLogger = self.logger,
                                                                                    windowSize = config.e2eProbeWindowSize,
                                                                                    moduleName = 'WiFiMAC')

        e2ePacketProbe = openwns.ldk.Probe.PacketProbeBus(name = "wifimac.e2eDelayProbe",
                                                          prefix = "wifimac.e2e",
                                                          commandName = "e2eDelayProbeCommand",
                                                          parentLogger = self.logger,
                                                          moduleName = 'WiFiMAC')


        upperConvergence = dll.UpperConvergence.UT(parent = self.logger,
                                                   commandName = self.upperConvergenceName)
        upperConvergence.logger.moduleName = 'WiFiMAC'

        forwarding = wifimac.pathselection.StationForwarding(functionalUnitName = "STAForwarding",
                                                             commandName = "ForwardingCommand",
                                                             upperConvergenceName = self.upperConvergenceName,
                                                             parentLogger = self.logger)

        throughputProbe =  wifimac.helper.Probes.DestinationSortedWindowProbeBus(name = "wifimac.hopWindowProbe",
                                                                                 prefix = "wifimac.hop",
                                                                                 commandName = 'hopWindowProbeCommand',
                                                                                 ucCommandName = self.upperConvergenceName,
                                                                                 parentLogger = self.logger,
                                                                                 windowSize = config.probeWindowSize,
                                                                                 moduleName = 'WiFiMAC')

        packetProbe = openwns.ldk.Probe.PacketProbeBus(name = "wifimac.hopDelayProbe",
                                                       prefix = "wifimac.hop",
                                                       commandName = "hopDelayProbeCommand",
                                                       parentLogger = self.logger,
                                                       moduleName = 'WiFiMAC')

        self.fun = openwns.FUN.FUN()
        for fu in ([e2eThroughputProbe, e2ePacketProbe, upperConvergence, forwarding, packetProbe, throughputProbe]):
            self.fun.add(fu)

        ################
        # Lower MAC part
        funTemplate = config.funTemplate(logger = self.logger,
                                         transceiverAddress = node.id,
                                         upperConvergenceName = self.upperConvergenceName)

        [managementTop, managementBottom] = funTemplate.createManagement(config, self.fun)
        [convergenceTop, convergenceBottom] = funTemplate.createConvergence(config, self.fun)
        [lowerMACTop, lowerMACBottom] = funTemplate.createLowerMAC(config, self.fun)
        self.manager = lowerMACTop

        ###################################
        # connect FUs with each other
        upperConvergence.connect(e2eThroughputProbe)
        e2eThroughputProbe.connect(e2ePacketProbe)
        e2ePacketProbe.connect(forwarding)
        forwarding.connect(throughputProbe)
        throughputProbe.connect(packetProbe)
        packetProbe.connect(lowerMACTop)

        lowerMACBottom.connect(convergenceTop)

        managementBottom.connect(convergenceTop)

        ##########
        # Services
        self.controlServices.append(dll.Services.Association(parent = self.logger))

        self.managementServices.extend(funTemplate.createManagementServices(config))

class MeshLayer2(dll.Layer2.Layer2):
    pathSelectionServiceName = None
    manager = None
    switch = None
    topFU = None
    addresses = None

    def __init__(self, node, name, config, parentLogger):
        super(MeshLayer2, self).__init__(node, name, parentLogger)

        self.nameInComponentFactory = "wifimac.Layer2"
        self.associations = []
        self.manager = []
        self.addresses = []

        self.fun = openwns.FUN.FUN()

        ################
        # Upper MAC part
        e2eThroughputProbe = wifimac.helper.Probes.DestinationSortedWindowProbeBus(name = "wifimac.e2eWindowProbe",
                                                                                   prefix = "wifimac.e2e",
                                                                                   commandName = 'e2eWindowProbeCommand',
                                                                                   ucCommandName = self.upperConvergenceName,
                                                                                   parentLogger = self.logger,
                                                                                   windowSize = config.e2eProbeWindowSize,
                                                                                   moduleName = 'WiFiMAC')


        throughputProbe =  wifimac.helper.Probes.DestinationSortedWindowProbeBus(name = "wifimac.hopWindowProbe",
                                                                                 prefix = "wifimac.hop",
                                                                                 commandName = 'hopWindowProbeCommand',
                                                                                 ucCommandName = self.upperConvergenceName,
                                                                                 parentLogger = self.logger,
                                                                                 windowSize = config.probeWindowSize,
                                                                                 moduleName = 'WiFiMAC')

        e2ePacketProbe = openwns.ldk.Probe.PacketProbeBus(name = "wifimac.e2eDelayProbe",
                                                       prefix = "wifimac.e2e",
                                                       commandName = "e2eDelayProbeCommand",
                                                       parentLogger = self.logger,
                                                       moduleName = 'WiFiMAC')

        packetProbe = openwns.ldk.Probe.PacketProbeBus(name = "wifimac.hopDelayProbe",
                                                       prefix = "wifimac.hop",
                                                       commandName = "hopDelayProbeCommand",
                                                       parentLogger = self.logger,
                                                       moduleName = 'WiFiMAC')

        forwarding = wifimac.pathselection.MeshForwarding(functionalUnitName = "MeshForwarding",
                                                          commandName = "ForwardingCommand",
                                                          upperConvergenceName = self.upperConvergenceName,
                                                          parentLogger = self.logger)


        self.switch = dll.CompoundSwitch.CompoundSwitch(logName = 'AdrSwitch', moduleName = 'WiFiMAC', parentLogger=self.logger)

        for fu in [e2eThroughputProbe, e2ePacketProbe, forwarding, throughputProbe, packetProbe, self.switch]:
            self.fun.add(fu)

        self.topFU = e2eThroughputProbe

        e2eThroughputProbe.connect(e2ePacketProbe)
        e2ePacketProbe.connect(forwarding)
        forwarding.connect(throughputProbe)
        throughputProbe.connect(packetProbe)

        self.switch.connectOnDataFU(packetProbe, dll.CompoundSwitch.FilterAll('All'))

        self.controlServices.append(dll.Services.Association(parent = self.logger))

    def addTransceiver(self, address, phyDataTransmission, phyNotification, phyCarrierSense, config):
        logger = wifimac.Logger.Logger(name = str(address), parent = self.logger)

        ################
        # Lower MAC Part
        funTemplate = config.funTemplate(logger = logger,
                         transceiverAddress = address,
                         upperConvergenceName = self.upperConvergenceName)

        [managementTop, managementBottom] = funTemplate.createManagement(config, self.fun)
        [lowerMACTop, lowerMACBottom] = funTemplate.createLowerMAC(config, self.fun)
        self.manager.append(lowerMACTop)
        [convergenceTop, convergenceBottom] = funTemplate.createConvergence(config, self.fun)

        # connect FU chains with each other
        lowerMACBottom.connect(convergenceTop)
        managementBottom.connect(convergenceTop)

        self.switch.connectSendDataFU(lowerMACTop, dll.CompoundSwitch.FilterMACAddress(name = 'Adr'+str(address),
                                                   filterMACAddress = address,
                                                   filterTarget = False,
                                                   upperConvergenceCommandName = self.upperConvergenceName))

        self.manager[-1].setMACAddress(address)
        self.manager[-1].setPhyDataTransmission(phyDataTransmission)
        self.manager[-1].setPhyNotification(phyNotification)
        self.manager[-1].setPhyCarrierSense(phyCarrierSense)
        self.addresses.append(address)

        self.managementServices.extend(funTemplate.createManagementServices(config))

    def setPathSelectionService(self, name):
        self.pathSelectionServiceName = name
        self.managementServices.append(
            wifimac.pathselection.PathSelection.PathSelectionOverVPS(self.pathSelectionServiceName,
                                         upperConvergenceName = self.upperConvergenceName,
                                         parentLogger = self.logger))
class dllAP(MeshLayer2):

    def __init__(self, node, name, config, parentLogger):
        super(dllAP, self).__init__(node, name, config, parentLogger)

        self.stationType = "AP"
        self.ring = 1

        upperConvergence = dll.UpperConvergence.AP(parent = self.logger, commandName = self.upperConvergenceName)
        upperConvergence.logger.moduleName = 'WiFiMAC'
        self.fun.add(upperConvergence)
        upperConvergence.connect(self.topFU)

class dllMP(MeshLayer2):

    def __init__(self, node, name, config, parentLogger):
        super(dllMP, self).__init__(node, name, config, parentLogger)

        self.stationType = "FRS"
        self.ring = 2

        upperConvergence = dll.UpperConvergence.UT(parent = self.logger,
                               commandName = self.upperConvergenceName)		
        upperConvergence.logger.moduleName = 'WiFiMAC'
        self.fun.add(upperConvergence)
        upperConvergence.connect(self.topFU)

# begin example "wifimac.pyconfig.layer2.config.start"
class Config(Sealed):
    beacon = None
    channelState = None
    phyUser = None
    unicastDCF = None
    broadcastDCF = None
    arq = None
    perMIB = None
    protocolCalculator = None
    ra = None
    rtscts = None
    manager = None
    txop = None
    aggregation = None
    frameSynchronization = None
    beaconLQM = None
    # end example
    # begin example "wifimac.pyconfig.layer2.config.variables"
    bufferSize = 10
    bufferSizeUnit = 'PDU'

    headerBits = 32*8
    crcBits = 4*8

    maxFrameSize = 65538*8

    useFastLinkFeedback = False

    probeWindowSize = 1.0
    e2eProbeWindowSize = 1.0

    funTemplate = None
    # end example

    # begin example "wifimac.pyconfig.layer2.config.multiusevariables"
    # variables which are used multiple times
    rtsctsThreshold = None
    """ This threshold (in bits) decides if (a) a rts/cts handshake
        precedes a frame transmission and (b) if the frame is
        considered as large or small (for the retransmission limit)
    """

    sifsDuration = None
    """ Duration of the Short Interframe Space (SIFS) in seconds
    """

    maximumACKDuration = None
    """ The maximum duration of a ACK frame [seconds]. This value is used
        to set the frame exchange duration field in data (and rts/cts)
        frames so that other STAs can set their NAV accordingly.
    """

    maximumCTSDuration = None
    """ Similar to maximumACKDuration but for the CTS frame - used in the
        RTS frame [seconds].
    """

    eifsDuration = None

    ackTimeout = None

    ctsTimeout = None

    slotDuration = None
    """ The duration of a backoff slot [seconds]."""

    preambleProcessingDelay = None
    """ Delay from the start of the preamble (which is small+long training
        sequence plus PLCP Header) until the receiver has identified the
        preamble as valid OFDM transmission and signals RxStartIndication -->
        duration of preamble plus short delay
    """
    # end example

    multipleUsedVariables = None

    # begin example "wifimac.pyconfig.layer2.config.init"
    def __init__(self, initFrequency):
        self.beacon = wifimac.management.BeaconConfig()
        self.channelState = wifimac.convergence.ChannelStateConfig()
        self.phyUser = wifimac.convergence.PhyUserConfig(initFrequency)
        self.unicastDCF = wifimac.lowerMAC.DCFConfig()
        self.broadcastDCF = wifimac.lowerMAC.DCFConfig(cwMin = 7, cwMax = 7)
        self.arq = wifimac.lowerMAC.StopAndWaitARQConfig()
        self.perMIB = wifimac.management.InformationBases.PERConfig()
        self.protocolCalculator = wifimac.protocolCalculator.Config()
        self.ra = wifimac.lowerMAC.RateAdaptationConfig()
        self.rtscts = wifimac.lowerMAC.RTSCTSConfig()
        self.manager = wifimac.lowerMAC.ManagerConfig()
        self.txop = wifimac.lowerMAC.TXOPConfig()
        self.aggregation = wifimac.draftn.AggregationConfig()
        #self.multiBuffer = wifimac.draftn.MultiBufferConfig()
        #self.blockACK = wifimac.draftn.BlockACKConfig()
        self.blockUntilReply = wifimac.draftn.BlockUntilReplyConfig()
        self.frameSynchronization = wifimac.convergence.FrameSynchronizationConfig()
        self.beaconLQM = wifimac.pathselection.BeaconLinkQualityMeasurementConfig()
        # end example

        self.funTemplate = wifimac.FUNModes.Basic

        self.multipleUsedVariables = dict()
        self.multipleUsedVariables['rtsctsThreshold'] = ['self', 'self.arq', 'self.rtscts']
        self.multipleUsedVariables['sifsDuration'] = ['self', 'self.manager', 'self.rtscts', 'self.arq', 'self.txop', 'self.blockUntilReply', 'self.channelState', 'self.beaconLQM']
        self.multipleUsedVariables['maximumACKDuration'] = ['self', 'self.manager', 'self.rtscts', 'self.arq', 'self.txop', 'self.beaconLQM']
        self.multipleUsedVariables['maximumCTSDuration'] = ['self', 'self.rtscts']
        self.multipleUsedVariables['slotDuration'] =  ['self', 'self.unicastDCF', 'self.broadcastDCF', 'self.beaconLQM']
        self.multipleUsedVariables['preambleProcessingDelay'] = ['self', 'self.rtscts']
        self.multipleUsedVariables['ctsTimeout'] = ['self', 'self.rtscts']
        self.multipleUsedVariables['ackTimeout'] = ['self', 'self.arq']
        self.multipleUsedVariables['eifsDuration'] = ['self', 'self.unicastDCF', 'self.broadcastDCF']

        # this ensures consistency, no matter on what the variables are set by default
        self.rtsctsThreshold = 800*8
        self.sifsDuration = 16E-6
        self.preambleProcessingDelay = 21E-6
        self.maximumCTSDuration = 44E-6
        self.slotDuration = 9E-6
        # value for normal ACK -- change e.g. for BlockACK to 68E-6
        self.maximumACKDuration = 44E-6
        # EIFS = SIFS + DIFS + ACK
        self.eifsDuration = self.sifsDuration + 34E-6 + self.maximumACKDuration
        self.ackTimeout = self.sifsDuration + self.slotDuration + self.preambleProcessingDelay
        self.ctsTimeout = self.sifsDuration + self.slotDuration + self.preambleProcessingDelay

    def __setattr__(self, name, val):
        # special setattr for multiple used variables: Enable propagation of single setting to required FUs
        if (isinstance(self.multipleUsedVariables, dict)) and (name in self.multipleUsedVariables.keys()):
            for target in self.multipleUsedVariables[name]:
                exec(target + '.__dict__[\'' + name + '\']=' + str(val))
        else:
            self.__dict__[name] = val
            # self.multipleUsedVariables must be set at the beginning
            if(isinstance(self.multipleUsedVariables, dict)):
                for muv in self.multipleUsedVariables.keys():
                    for target in self.multipleUsedVariables[muv]:
                        exec(target + '.__dict__[\'' + muv + '\']=self.__dict__[\''+muv+'\']')

class configUpperLayer2(Sealed):
    probeWindowSize = 1.0
    e2eProbeWindowSize = 1.0
