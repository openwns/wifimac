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
    'src/convergence/OFDMAAccessFunc.cpp',
    'src/convergence/ErrorModelling.cpp',
    'src/convergence/PhyMode.cpp',
    'src/convergence/PhyModeProvider.cpp',
    'src/convergence/FrameSynchronization.cpp',
    'src/convergence/PreambleGenerator.cpp',
    'src/convergence/TxDurationSetter.cpp',
    'src/convergence/ChannelState.cpp',

    # lower MAC
    'src/lowerMAC/Manager.cpp',
    'src/lowerMAC/RTSCTS.cpp',
    'src/lowerMAC/StopAndWaitARQ.cpp',
    'src/lowerMAC/RateAdaptation.cpp',
    'src/lowerMAC/TXOP.cpp',
    'src/lowerMAC/NextFrameGetter.cpp',
    'src/lowerMAC/DuplicateFilter.cpp',

    'src/lowerMAC/timing/DCF.cpp',
    'src/lowerMAC/timing/Backoff.cpp',

    'src/lowerMAC/rateAdaptationStrategies/ConstantLow.cpp',
    'src/lowerMAC/rateAdaptationStrategies/SINR.cpp',
    'src/lowerMAC/rateAdaptationStrategies/Opportunistic.cpp',

    # additions for DraftN
    'src/convergence/DeAggregation.cpp',
    'src/lowerMAC/BlockUntilReply.cpp',
    'src/lowerMAC/Aggregation.cpp',
    'src/lowerMAC/BlockACK.cpp',
    'src/lowerMAC/rateAdaptationStrategies/SINRwithMIMO.cpp',

    # (Virtual) Management stuff
    'src/management/Beacon.cpp',
    'src/management/SINRInformationBase.cpp',
    'src/management/PERInformationBase.cpp',
    'src/management/VirtualCapabilityInformationBase.cpp',

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
    'src/lowerMAC/timing/tests/BackoffTest.cpp',
]

hppFiles = [
    'src/FrameType.hpp',
    'src/Layer2.hpp',
    'src/WiFiMAC.hpp',
    'src/convergence/ChannelState.hpp',
    'src/convergence/DeAggregation.hpp',
    'src/convergence/ErrorModelling.hpp',
    'src/convergence/FrameSynchronization.hpp',
    'src/convergence/IChannelState.hpp',
    'src/convergence/INetworkAllocationVector.hpp',
    'src/convergence/IRxStartEnd.hpp',
    'src/convergence/ITxStartEnd.hpp',
    'src/convergence/OFDMAAccessFunc.hpp',
    'src/convergence/PhyMode.hpp',
    'src/convergence/PhyModeProvider.hpp',
    'src/convergence/PhyUser.hpp',
    'src/convergence/PhyUserCommand.hpp',
    'src/convergence/PreambleGenerator.hpp',
    'src/convergence/TxDurationSetter.hpp',
    'src/helper/FilterFrameType.hpp',
    'src/helper/FilterSize.hpp',
    'src/helper/HopContextWindowProbe.hpp',
    'src/helper/Keys.hpp',
    'src/helper/ThroughputProbe.hpp',
    'src/helper/contextprovider/CommandInformation.hpp',
    'src/lowerMAC/Aggregation.hpp',
    'src/lowerMAC/BlockACK.hpp',
    'src/lowerMAC/BlockUntilReply.hpp',
    'src/lowerMAC/DuplicateFilter.hpp',
    'src/lowerMAC/ITransmissionCounter.hpp',
    'src/lowerMAC/Manager.hpp',
    'src/lowerMAC/NextFrameGetter.hpp',
    'src/lowerMAC/RTSCTS.hpp',
    'src/lowerMAC/RateAdaptation.hpp',
    'src/lowerMAC/StopAndWaitARQ.hpp',
    'src/lowerMAC/TXOP.hpp',
    'src/lowerMAC/rateAdaptationStrategies/ConstantLow.hpp',
    'src/lowerMAC/rateAdaptationStrategies/IRateAdaptationStrategy.hpp',
    'src/lowerMAC/rateAdaptationStrategies/Opportunistic.hpp',
    'src/lowerMAC/rateAdaptationStrategies/SINR.hpp',
    'src/lowerMAC/rateAdaptationStrategies/SINRwithMIMO.hpp',
    'src/lowerMAC/timing/Backoff.hpp',
    'src/lowerMAC/timing/DCF.hpp',
    'src/lowerMAC/timing/tests/BackoffTest.hpp',
    'src/management/Beacon.hpp',
    'src/management/ILinkNotification.hpp',
    'src/management/PERInformationBase.hpp',
    'src/management/SINRInformationBase.hpp',
    'src/management/VirtualCapabilityInformationBase.hpp',
    'src/pathselection/BeaconLinkQualityMeasurement.hpp',
    'src/pathselection/ForwardingCommand.hpp',
    'src/pathselection/IPathSelection.hpp',
    'src/pathselection/LinkQualityMeasurement.hpp',
    'src/pathselection/MeshForwarding.hpp',
    'src/pathselection/Metric.hpp',
    'src/pathselection/NovelCheck.hpp',
    'src/pathselection/PathSelectionOverVPS.hpp',
    'src/pathselection/StationForwarding.hpp',
    'src/pathselection/VirtualPathSelection.hpp',
]
pyconfigs = [
    'wifimac/FUNModes.py',
    'wifimac/Layer2.py',
    'wifimac/Logger.py',
    'wifimac/WiFiMac.py',
    'wifimac/__init__.py',
    'wifimac/convergence/ChannelState.py',
    'wifimac/convergence/DeAggregation.py',
    'wifimac/convergence/ErrorModelling.py',
    'wifimac/convergence/FrameSynchronization.py',
    'wifimac/convergence/PhyMode.py',
    'wifimac/convergence/PhyUser.py',
    'wifimac/convergence/PreambleGenerator.py',
    'wifimac/convergence/TxDurationSetter.py',
    'wifimac/convergence/__init__.py',
    'wifimac/draftn/Aggregation.py',
    'wifimac/draftn/BlockACK.py',
    'wifimac/draftn/BlockUntilReply.py',
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
    'wifimac/lowerMAC/NextFrameGetter.py',
    'wifimac/lowerMAC/RTSCTS.py',
    'wifimac/lowerMAC/RateAdaptation.py',
    'wifimac/lowerMAC/StopAndWaitARQ.py',
    'wifimac/lowerMAC/TXOP.py',
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
    'wifimac/support/__init__.py',
]
dependencies = []
Return('libname srcFiles hppFiles pyconfigs dependencies')


