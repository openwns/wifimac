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

libname = 'wifimac'
srcFiles = [
    # root of the module
    'src/WiFiMAC.cpp',
    'src/Layer2.cpp',

    # convergence layer
    'src/convergence/PhyUser.cpp',
    'src/convergence/ErrorModelling.cpp',
    'src/convergence/PhyMode.cpp',
    'src/convergence/PhyModeProvider.cpp',
    'src/convergence/FrameSynchronization.cpp',
    'src/convergence/PreambleGenerator.cpp',
    'src/convergence/TxDurationSetter.cpp',
    'src/convergence/ChannelState.cpp',
    'src/convergence/NetworkStateProbe.cpp',
    'src/convergence/GuiWriter.cpp',

    # lower MAC
    'src/lowerMAC/Manager.cpp',
    'src/lowerMAC/RTSCTS.cpp',
    'src/lowerMAC/StopAndWaitARQ.cpp',
    'src/lowerMAC/RateAdaptation.cpp',
    'src/lowerMAC/TXOP.cpp',
    'src/lowerMAC/DuplicateFilter.cpp',
    'src/lowerMAC/Buffer.cpp',

    'src/lowerMAC/timing/DCF.cpp',
    'src/lowerMAC/timing/Backoff.cpp',

    'src/lowerMAC/rateAdaptationStrategies/Constant.cpp',
    'src/lowerMAC/rateAdaptationStrategies/SINR.cpp',
    'src/lowerMAC/rateAdaptationStrategies/Opportunistic.cpp',

    # additions for DraftN
    'src/draftn/DeAggregation.cpp',
    'src/draftn/BlockUntilReply.cpp',
    'src/draftn/Aggregation.cpp',
    'src/draftn/BlockACK.cpp',
    'src/draftn/TransmissionQueue.cpp',
    'src/draftn/ReceptionQueue.cpp',
    'src/draftn/RTSCTSwithFLA.cpp',
    'src/draftn/LongTrainingFieldGenerator.cpp',
    'src/draftn/rateAdaptationStrategies/SINRwithMIMO.cpp',
    'src/draftn/rateAdaptationStrategies/OpportunisticwithMIMO.cpp',
    'src/draftn/SINRwithMIMOInformationBase.cpp',

    # Management stuff
    'src/management/Beacon.cpp',
    'src/management/SINRInformationBase.cpp',
    'src/management/PERInformationBase.cpp',
    'src/management/VirtualCapabilityInformationBase.cpp',
    'src/management/ProtocolCalculator.cpp',
    'src/management/protocolCalculatorPlugins/ErrorProbability.cpp',
    'src/management/protocolCalculatorPlugins/FrameLength.cpp',
    'src/management/protocolCalculatorPlugins/Duration.cpp',

    # Pathselection
    'src/pathselection/VirtualPathSelection.cpp',
    'src/pathselection/MeshForwarding.cpp',
    'src/pathselection/StationForwarding.cpp',
    'src/pathselection/PathSelectionOverVPS.cpp',
    'src/pathselection/BeaconLinkQualityMeasurement.cpp',

    # Helper
    'src/helper/Keys.cpp',
    'src/helper/HopContextWindowProbe.cpp',
    'src/helper/FilterFrameType.cpp',
    'src/helper/FilterSize.cpp',

    # Tests
    #####'src/lowerMAC/timing/tests/BackoffTest.cpp',
]

hppFiles = [
    'src/FrameType.hpp',
    'src/Layer2.hpp',
    'src/WiFiMAC.hpp',
    'src/convergence/ChannelState.hpp',
    'src/draftn/DeAggregation.hpp',
    'src/convergence/ErrorModelling.hpp',
    'src/convergence/FrameSynchronization.hpp',
    'src/convergence/IChannelState.hpp',
    'src/convergence/INetworkAllocationVector.hpp',
    'src/convergence/IRxStartEnd.hpp',
    'src/convergence/ITxStartEnd.hpp',
    'src/convergence/PhyMode.hpp',
    'src/convergence/PhyModeProvider.hpp',
    'src/convergence/PhyUser.hpp',
    'src/convergence/PreambleGenerator.hpp',
    'src/convergence/TxDurationSetter.hpp',
    'src/convergence/NetworkStateProbe.hpp',
    'src/convergence/GuiWriter.hpp',
    'src/helper/FilterFrameType.hpp',
    'src/helper/FilterSize.hpp',
    'src/helper/HopContextWindowProbe.hpp',
    'src/helper/Keys.hpp',
    'src/helper/ThroughputProbe.hpp',
    'src/helper/CholeskyDecomposition.hpp',
    'src/helper/contextprovider/CommandInformation.hpp',
    'src/helper/contextprovider/CompoundSize.hpp',
    'src/draftn/Aggregation.hpp',
    'src/lowerMAC/ITXOPWindow.hpp',
    'src/lowerMAC/Buffer.hpp',
    'src/draftn/BlockACK.hpp',
    'src/draftn/BlockACKCommand.hpp',
    'src/draftn/TransmissionQueue.hpp',
    'src/draftn/ReceptionQueue.hpp',
    'src/draftn/IBlockACKObserver.hpp',
    'src/draftn/BlockUntilReply.hpp',
    'src/draftn/RTSCTSwithFLA.hpp',
    'src/draftn/LongTrainingFieldGenerator.hpp',
    'src/draftn/SINRwithMIMOInformationBase.hpp',
    'src/lowerMAC/DuplicateFilter.hpp',
    'src/lowerMAC/ITransmissionCounter.hpp',
    'src/lowerMAC/Manager.hpp',
    'src/lowerMAC/RTSCTS.hpp',
    'src/lowerMAC/RateAdaptation.hpp',
    'src/lowerMAC/StopAndWaitARQ.hpp',
    'src/lowerMAC/TXOP.hpp',
    'src/lowerMAC/ITXOPObserver.hpp',
    'src/lowerMAC/rateAdaptationStrategies/Constant.hpp',
    'src/lowerMAC/rateAdaptationStrategies/IRateAdaptationStrategy.hpp',
    'src/lowerMAC/rateAdaptationStrategies/Opportunistic.hpp',
    'src/lowerMAC/rateAdaptationStrategies/SINR.hpp',
    'src/draftn/rateAdaptationStrategies/SINRwithMIMO.hpp',
    'src/draftn/rateAdaptationStrategies/OpportunisticwithMIMO.hpp',
    'src/lowerMAC/timing/Backoff.hpp',
    'src/lowerMAC/timing/DCF.hpp',
    'src/lowerMAC/timing/tests/BackoffTest.hpp',
    'src/management/Beacon.hpp',
    'src/management/ILinkNotification.hpp',
    'src/management/PERInformationBase.hpp',
    'src/management/SINRInformationBase.hpp',
    'src/management/VirtualCapabilityInformationBase.hpp',
    'src/management/ProtocolCalculator.hpp',
    'src/management/protocolCalculatorPlugins/ErrorProbability.hpp',
    'src/management/protocolCalculatorPlugins/FrameLength.hpp',
    'src/management/protocolCalculatorPlugins/Duration.hpp',
    'src/management/protocolCalculatorPlugins/ConfigGetter.hpp',
    'src/pathselection/BeaconLinkQualityMeasurement.hpp',
    'src/pathselection/ForwardingCommand.hpp',
    'src/pathselection/IPathSelection.hpp',
    'src/pathselection/LinkQualityMeasurement.hpp',
    'src/pathselection/MeshForwarding.hpp',
    'src/pathselection/Metric.hpp',
    'src/pathselection/PathSelectionOverVPS.hpp',
    'src/pathselection/StationForwarding.hpp',
    'src/pathselection/VirtualPathSelection.hpp'
]

pyconfigs = [
    'wifimac/FUNModes.py',
    'wifimac/Layer2.py',
    'wifimac/Logger.py',
    'wifimac/WiFiMac.py',
    'wifimac/__init__.py',
    'wifimac/convergence/ChannelState.py',
    'wifimac/draftn/DeAggregation.py',
    'wifimac/convergence/ErrorModelling.py',
    'wifimac/convergence/FrameSynchronization.py',
    'wifimac/convergence/PhyMode.py',
    'wifimac/convergence/PhyUser.py',
    'wifimac/convergence/PreambleGenerator.py',
    'wifimac/convergence/TxDurationSetter.py',
    'wifimac/convergence/__init__.py',
    'wifimac/convergence/NetworkStateProbe.py',
    'wifimac/draftn/Aggregation.py',
    'wifimac/draftn/BlockACK.py',
    'wifimac/draftn/BlockUntilReply.py',
    'wifimac/draftn/PhyMode.py',
    'wifimac/draftn/RTSCTSwithFLA.py',
    'wifimac/draftn/LongTrainingFieldGenerator.py',
    'wifimac/draftn/__init__.py',
    'wifimac/evaluation/__init__.py',
    'wifimac/evaluation/default.py',
    'wifimac/evaluation/ip.py',
    'wifimac/helper/Filter.py',
    'wifimac/helper/Keys.py',
    'wifimac/helper/Probes.py',
    'wifimac/helper/__init__.py',
    'wifimac/lowerMAC/DCF.py',
    'wifimac/lowerMAC/DuplicateFilter.py',
    'wifimac/lowerMAC/Manager.py',
    'wifimac/lowerMAC/RTSCTS.py',
    'wifimac/lowerMAC/RateAdaptation.py',
    'wifimac/lowerMAC/StopAndWaitARQ.py',
    'wifimac/lowerMAC/TXOP.py',
    'wifimac/lowerMAC/Buffer.py',
    'wifimac/lowerMAC/__init__.py',
    'wifimac/management/Beacon.py',
    'wifimac/management/InformationBases.py',
    'wifimac/management/__init__.py',
    'wifimac/pathselection/BeaconLinkQualityMeasurement.py',
    'wifimac/pathselection/Forwarding.py',
    'wifimac/pathselection/PathSelection.py',
    'wifimac/pathselection/__init__.py',
    'wifimac/support/ChannelManagerPool.py',
    'wifimac/support/NodeCreator.py',
    'wifimac/support/Rang.py',
    'wifimac/support/Transceiver.py',
    'wifimac/support/Layer1Config.py',
    'wifimac/support/__init__.py',

    'wifimac/protocolCalculator/__init__.py',
    'wifimac/protocolCalculator/Duration.py',
    'wifimac/protocolCalculator/FrameLength.py',
]
dependencies = []
Return('libname srcFiles hppFiles pyconfigs dependencies')


