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

import openwns.node
import openwns.pyconfig
from openwns import dBm, dB

import ofdmaphy.Station

import wifimac.Layer2
import wifimac.support.Rang
import wifimac.Logger

import ip.VirtualARP
import ip.VirtualDNS
import wifimac.pathselection

import ip.Component
import ip
from ip.AddressResolver import FixedAddressResolver, VirtualDHCPResolver

import constanze.node
import rise.Mobility

class Station(openwns.node.Node):
    load = None      # Load Generator
    tl = None        # Transport Layer
    nl = None        # Network Layer
    dll = None       # Data Link Layer
    phy = None       # Physical Layer
    mobility = None  # Mobility Component

    id = None        # Identifier

    def __init__(self, name, id):
        super(Station, self).__init__(name)
        self.id = id
        self.logger = wifimac.Logger.Logger(name, parent = None)

    def setPosition(self, position):
        self.mobility.mobility.setCoords(position)      

    def getPosition(self):
        return self.mobility.mobility.getCoords()

    def setAntenna(self, antenna):
        assert False, "Not implemented!!!"

    def getAntenna(self):
        assert False, "Not implemented!!!"

class NodeCreator(object):
    __slots__ = (
    'propagation',  # rise propagation object (pathloss, shadowing, fading... for node type pairs)
    'txPower'
    )

    def __init__(self, propagationConfig, **kw):
        self.propagation = rise.scenario.Propagation.Propagation()
        self.propagation.configurePair("STA", "STA", propagationConfig)
        self.propagation.configurePair("STA", "MP", propagationConfig)
        self.propagation.configurePair("STA", "AP", propagationConfig)

        self.propagation.configurePair("MP", "STA", propagationConfig)
        self.propagation.configurePair("MP", "MP", propagationConfig)
        self.propagation.configurePair("MP", "AP", propagationConfig)

        self.propagation.configurePair("AP", "STA", propagationConfig)
        self.propagation.configurePair("AP", "MP", propagationConfig)
        self.propagation.configurePair("AP", "AP", propagationConfig)

        self.txPower = dBm(30)

        openwns.pyconfig.attrsetter(self, kw)

    def createPhyLayer(self, managerName, propagationName, config, parentLogger):
        receiver = ofdmaphy.Receiver.OFDMAReceiver(propagation = self.propagation,
                                                   propagationCharacteristicName = propagationName,
                                                   receiverNoiseFigure = config.receiverNoiseFigure,
                                                   parentLogger = parentLogger )
        receiver.mimoProcessing = config.mimoProcessing
        transmitter = rise.Transmitter.Transmitter(propagation = self.propagation,
                                                   propagationCharacteristicName = propagationName,
                                                   parentLogger = parentLogger )

        phy = ofdmaphy.Station.OFDMAStation([receiver], [transmitter], parentLogger = parentLogger)
        phy.txFrequency = config.frequency
        phy.rxFrequency = config.frequency
        phy.txPower = config.txPower
        phy.numberOfSubCarrier = 1
        phy.bandwidth = config.bandwidth
        phy.systemManagerName = managerName
        phy.numAntennas = config.numAntennas

        return(phy)

    def createTransceiver(self, node, name, MACAddress, managerName, config):
        # create PHY
        phyLayerConfig = self.createPhyLayer(managerName = managerName,
                                             propagationName = name,
                                             config = config.layer1,
                                             parentLogger = node.logger)
        ofdma = ofdmaphy.Station.OFDMAComponent(node, name+' PHY' + str(MACAddress), phyLayerConfig, parentLogger = node.logger)
        node.phy.append(ofdma)

        # create lower MAC
        node.dll.addTransceiver(address = MACAddress,
                                phyDataTransmission = ofdma.dataTransmission,
                                phyNotification = ofdma.notification,
                                phyCarrierSense = ofdma.notification,
                                config = config.layer2)


    def createAP(self, idGen, managerPool, config):
        # One id for the node and the first transceiver
        id = idGen.next()

        newAP = Station("AP" + str(id), id)
        newAP.setProperty("Type", "AP")
        # Create tranceivers (phy+lower mac), for each given frequency one
        # Prepare the L1/L2

        newAP.phy = []
        newAP.dll = wifimac.Layer2.dllAP(newAP, "MAC AP"+str(id), config=config.configUpperLayer2, parentLogger = newAP.logger)
        newAP.dll.setStationID(id)
        newAP.dll.setPathSelectionService("PATHSELECTIONOVERVPS")

        # The first frequency is the bss frequency (important for the manager)
        self.createTransceiver(node = newAP,
                               name = 'AP',
                               MACAddress = id,
                               managerName = managerPool.getBSSManager().name,
                               config = config.transceivers[0])

        # Create the mesh transceivers
        for i in xrange(len(config.transceivers)-1):
            self.createTransceiver(node = newAP,
                                   name = 'AP',
                                   MACAddress = idGen.next(),
                                   managerName = managerPool.getManager(config.transceivers[i+1].layer1.frequency, id).name,
                                   config = config.transceivers[i+1])
        # create Mobility component
        newAP.mobility = rise.Mobility.Component(node = newAP,
                                                     name = "Mobility AP"+str(id),
                                                     mobility = rise.Mobility.No(openwns.geometry.position.Position()))
        newAP.mobility.mobility.setCoords(config.position)

        return newAP

    def createMP(self, idGen, managerPool, config):
        # One id for the node and the first transceiver
        id = idGen.next()

        newMP = Station("MP" + str(id), id)
        newMP.setProperty("Type", "MP")

        # Create tranceivers (phy+lower mac), for each given frequency one
        # Prepare the L1/L2

        newMP.phy = []
        newMP.dll = wifimac.Layer2.dllMP(newMP, "MAC MP"+str(id), config=config.configUpperLayer2, parentLogger = newMP.logger)
        newMP.dll.setStationID(id)
        newMP.dll.setPathSelectionService("PATHSELECTIONOVERVPS")

        # The first frequency is the bss frequency (important for the manager)
        self.createTransceiver(node = newMP,
                               name = 'MP',
                               MACAddress = id,
                               managerName = managerPool.getBSSManager().name,
                               config = config.transceivers[0])

        # Create the mesh transceivers
        for i in xrange(len(config.transceivers)-1):
            self.createTransceiver(node = newMP,
                                   name = 'MP',
                                   MACAddress = idGen.next(),
                                   managerName = managerPool.getManager(config.transceivers[i+1].layer1.frequency, id).name,
                                   config = config.transceivers[i+1])

        newMP.nl = ip.Component.IPv4Component ( newMP, "192.168.1."+str ( id ),"192.168.1."+str ( id ), probeWindow = config.transceivers[0].probeWindow )
        newMP.nl.addDLL ( "wifi",
                # Where to get IP Adresses
                _addressResolver = FixedAddressResolver ( "192.168.1."+str ( id ), "255.255.255.0" ),
                # Name of ARP zone
                _arpZone = "theOnlyZone",
                # We can deliver locally without going to the gateway
                _pointToPoint = True,
                # Service names of DLL
                _dllDataTransmission = newMP.dll.dataTransmission,
                _dllNotification = newMP.dll.notification )

        # create load generator
        newMP.load = constanze.node.ConstanzeComponent ( newMP, "constanze" )

        newMP.mobility = rise.Mobility.Component ( node = newMP,
                                                   name = "Mobility MP"+str ( id ),
                                                   mobility = rise.Mobility.No ( openwns.geometry.position.Position() ))
        newMP.mobility.mobility.setCoords ( config.position )

        return newMP

    def createSTA ( self, idGen, managerPool, rang, config, loggerLevel, dllLoggerLevel) :
        id = idGen.next()
        newSTA = Station("STA" + str(id), id)
        newSTA.setProperty("Type", "STA")

        # create Physical Layer
        newSTA.phy = ofdmaphy.Station.OFDMAComponent(newSTA, "PHY STA"+str(id),
                                                     self.createPhyLayer(managerName = managerPool.getBSSManager().name,
                                                                         propagationName="STA",
                                                                         config = config.layer1,
                                                                         parentLogger = newSTA.logger),
                                                     parentLogger = newSTA.logger)

        # create data link layer
        newSTA.dll = wifimac.Layer2.dllSTA ( newSTA, "MAC STA"+str ( id ), config.layer2, parentLogger = newSTA.logger )
        newSTA.dll.setStationID ( id )
        newSTA.dll.manager.setMACAddress(id)
        newSTA.dll.manager.setPhyDataTransmission(newSTA.phy.dataTransmission)
        newSTA.dll.manager.setPhyNotification(newSTA.phy.notification)
        newSTA.dll.manager.setPhyCarrierSense(newSTA.phy.notification)

        # create network layer
        newSTA.nl = ip.Component.IPv4Component ( newSTA, "192.168.1."+str ( id ),"192.168.1."+str ( id ), probeWindow = config.probeWindow )
        newSTA.nl.addDLL ( "wifi",
                # Where to get IP Adresses
                _addressResolver = FixedAddressResolver ( "192.168.1."+str ( id ), "255.255.255.0" ),
                # Name of ARP zone
                _arpZone = "theOnlyZone",
                # We can deliver locally without going to the gateway
                _pointToPoint = True,
                # Service names of DLL
                _dllDataTransmission = newSTA.dll.dataTransmission,
                _dllNotification = newSTA.dll.notification )

        # IP Route Table
        newSTA.nl.addRoute("192.168.1.0", "255.255.255.0", "0.0.0.0", "wifi")
        newSTA.nl.addRoute(rang.nl.dataLinkLayers[0].addressResolver.address,
                "255.255.255.255",
                rang.nl.dataLinkLayers[0].addressResolver.address,
                "wifi")
        rang.nl.addRoute(newSTA.nl.dataLinkLayers[0].addressResolver.address,
                 "255.255.255.255",
                 newSTA.nl.dataLinkLayers[0].addressResolver.address,
                 "wifi")

        # create load generator
        newSTA.load = constanze.node.ConstanzeComponent ( newSTA, "constanze" )

        # create mobility component
        newSTA.mobility = rise.Mobility.Component ( node = newSTA,
                                name = "Mobility STA"+str ( id ),
                                mobility = rise.Mobility.No ( openwns.geometry.position.Position() ))
        newSTA.mobility.mobility.setCoords ( config.position )

        newSTA.logger.level = loggerLevel
        newSTA.dll.logger.level = dllLoggerLevel


        return newSTA

    def createRANG(self, listener, loggerLevel,
                   listenerWindowSize = 1.0, listenerSampleInterval = 0.5):
        rang = Station('RANG', (256*255)-1)
        rang.setProperty("Type", "RANG")

        # create dll
        rang.dll = wifimac.support.Rang.RANG(rang, parentLogger = rang.logger)
        rang.dll.setStationID((256*255)-1)

        # create network layer
        rang.nl = ip.Component.IPv4Component(rang, "192.168.255.254", "192.168.255.254", probeWindow=listenerWindowSize)
        rang.nl.addDLL("wifi",
            # Where to get IP Adresses
            _addressResolver = FixedAddressResolver("192.168.255.254", "255.255.0.0"),
            # Name of ARP zone
            _arpZone = "theOnlyZone",
            # We can deliver locally without going to the gateway
            _pointToPoint = True,
            # Service names of DLL
            _dllDataTransmission = rang.dll.dataTransmission,
            _dllNotification = rang.dll.notification)
        rang.nl.forwarding.config.isForwarding = True

        # create load generator
        rang.load = constanze.node.ConstanzeComponent(rang, "constanze")

        rang.logger.level = loggerLevel

        if(listener):
            ipListenerBinding = constanze.node.IPListenerBinding(rang.nl.domainName, parentLogger=rang.logger)
            listener = constanze.node.Listener(rang.nl.domainName + ".listener", probeWindow = listenerWindowSize, parentLogger=rang.logger)
            rang.load.addListener(ipListenerBinding, listener)
            rang.nl.windowedEndToEndProbe.config.windowSize = listenerWindowSize
            rang.nl.windowedEndToEndProbe.config.sampleInterval = listenerSampleInterval

        return rang

    def createVARP(self, loggerLevel, name = "VARP", zone = "theOnlyZone"):
        varp = ip.VirtualARP.VirtualARPServer(name, zone)
        varp.logger.level = loggerLevel
        return varp

    def createVDNS(self, loggerLevel, name = "VDNS", root = "ip.DEFAULT.GLOBAL"):
        vdns = ip.VirtualDNS.VirtualDNSServer(name, root)
        vdns.logger.level = loggerLevel
        return vdns

    def createVPS(self, numNodes, loggerLevel, name = "VPS"):
        vps = wifimac.pathselection.VirtualPSServer("VPS", numNodes = numNodes)
        vps.logger.level = loggerLevel
        return vps

    def createVCIB(self, loggerLevel, name = "VCIB"):
        vcibs = wifimac.management.InformationBases.VirtualCababilityInformationService(name)
        vcibs.logger.level = loggerLevel
        return vcibs
