/******************************************************************************
 * WiFiMac                                                                    *
 * This file is part of openWNS (open Wireless Network Simulator)
 * _____________________________________________________________________________
 *
 * Copyright (C) 2004-2007
 * Chair of Communication Networks (ComNets)
 * Kopernikusstr. 16, D-52074 Aachen, Germany
 * phone: ++49-241-80-27910,
 * fax: ++49-241-80-22242
 * email: info@openwns.org
 * www: http://www.openwns.org
 * _____________________________________________________________________________
 *
 * openWNS is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 2 as published by the
 * Free Software Foundation;
 *
 * openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <WIFIMAC/pathselection/MeshForwarding.hpp>

#include <WNS/service/dll/StationTypes.hpp>
#include <WNS/ldk/CommandPool.hpp>
#include <WNS/Exception.hpp>

using namespace wifimac::pathselection;

STATIC_FACTORY_REGISTER_WITH_CREATOR(
    wifimac::pathselection::MeshForwarding,
    wns::ldk::FunctionalUnit,
    "wifimac.pathselection.MeshForwarding",
    wns::ldk::FUNConfigCreator);

MeshForwarding::MeshForwarding(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config_) :
    wns::ldk::fu::Plain<MeshForwarding, ForwardingCommand>(fun),
    config(config_),
    logger(config.get("logger")),
    ucName(config.get<std::string>("upperConvergenceName")),
    dot11MeshTTL(config.get<int>("dot11MeshTTL"))
{
    MESSAGE_SINGLE(NORMAL, this->logger, "created");
}

MeshForwarding::~MeshForwarding()
{

}

void
MeshForwarding::onFUNCreated()
{

    if(getFUN()->getLayer<wifimac::Layer2*>()->getStationType() == wns::service::dll::StationTypes::UT())
    {
        throw wns::Exception("MeshForwarding is only allowed for StationType != UT");
    }

    std::string psName = config.get<std::string>("pathSelectionServiceName");
    layer2 = getFUN()->getLayer<wifimac::Layer2*>();
    ps = layer2->getManagementService<wifimac::pathselection::IPathSelection>(psName);
    assure(ps, "Could not get VirtualPathSelection Service");
}

bool
MeshForwarding::doIsAccepting(const wns::ldk::CompoundPtr& _compound) const
{
    // First make a copy of the compound and use this, as we manipulate the
    // target address
    wns::ldk::CompoundPtr compound = _compound->copy();

    dll::UpperCommand* uc = getFUN()->getProxy()->getCommand<dll::UpperCommand>(compound->getCommandPool(), ucName);

    if (layer2->getControlService<dll::services::control::Association>("ASSOCIATION")
        ->hasAssociated(uc->peer.targetMACAddress))
    {
        uc->peer.sourceMACAddress = ps->getProxyFor(uc->peer.targetMACAddress);
    }
    else
    {
        wns::service::dll::UnicastAddress mpDst = uc->peer.targetMACAddress;
        if (!mpDst.isValid())
        {
            mpDst = ps->getPortalFor(uc->peer.sourceMACAddress);
        }
        uc->peer.targetMACAddress = ps->getNextHop(uc->peer.sourceMACAddress, mpDst);
        if(layer2->isTransceiverMAC(uc->peer.targetMACAddress))
        {
            uc->peer.sourceMACAddress = uc->peer.targetMACAddress;
            uc->peer.targetMACAddress = ps->getNextHop(uc->peer.sourceMACAddress, mpDst);
        }
    }

    // Forward the manipulated compound
    return getConnector()->hasAcceptor(compound);
}

void
MeshForwarding::doWakeup()
{
    // simply forward the wakeup call
    getReceptor()->wakeup();
}

void
MeshForwarding::doOnData(const wns::ldk::CompoundPtr& _compound)
{
    // Received compound from one of my transceivers

    // First: copy the compound
    wns::ldk::CompoundPtr compound = _compound->copy();

    ForwardingCommand* fc = getCommand(compound);
    dll::UpperCommand* uc = getFUN()->getProxy()->getCommand<dll::UpperCommand>(compound->getCommandPool(), ucName);

    ++fc->magic.hopCount;
    fc->magic.path.push_back(uc->peer.targetMACAddress);
    if(uc->peer.targetMACAddress != layer2->getDLLAddress())
    {
        fc->magic.path.push_back(layer2->getDLLAddress());
    }

#ifndef WNS_NO_LOGGING
    printForwardingInformation(fc, uc);
#endif

    assure(layer2->isTransceiverMAC(uc->peer.targetMACAddress),
           "Recieved compound with targetMACAddress " << uc->peer.targetMACAddress << " which is not a transceiver of me");

    if (layer2->getStationType() == wns::service::dll::StationTypes::AP())
    {
        this->doOnDataAP(compound, fc, uc);
        return;
    }
    if (layer2->getStationType() == wns::service::dll::StationTypes::FRS())
    {
        this->doOnDataFRS(compound, fc, uc);
        return;
    }

    throw wns::Exception("doOnData not implemented for this station type");
}

void MeshForwarding::doOnDataAP(const wns::ldk::CompoundPtr& compound, ForwardingCommand*& fc, dll::UpperCommand*& uc)
{
    assure(layer2->getStationType() == wns::service::dll::StationTypes::AP(), "Forwarding only valid for AP!");
    assure(fc->peer.toDS, "received frame NOT to DS");

    // special handling if one of the hops on the selected path goes over the
    // wired backbone, i.e. the RANG
    if((fc->peer.fromDS) &&
       (fc->peer.finalDestination != layer2->getDLLAddress()) &&
       (ps->isPortal(ps->getNextHop(layer2->getDLLAddress(), fc->peer.finalDestination))))
    {
        // the initial increase of the hc was incorrect
        --fc->magic.hopCount;
        // give it to the RANG
        MESSAGE_SINGLE(NORMAL, this->logger, "frame destination is another portal, give it to the RANG");
        // the RANG assumes the finalDestination in the UC command
        uc->peer.targetMACAddress = fc->peer.finalDestination;
        getDeliverer()->getAcceptor(compound)->onData(compound);
        return;
    }

    if(this->doOnDataFRS(compound, fc, uc))
    {
        //  Frame is either on its next wireless hop or dropped
        return;
    }
    else
    {
        assure(fc->peer.addressExtension == true, "delivered to root MP (AP), but peer.addressExtension is false");

        // frames which end up here are send by the meshSource without real
        // knowledge where to send them, i.e. using the tree-routing approach:
        // Send everything to the root, it should know where to put it

        if(ps->isMeshPoint(fc->peer.finalDestination))
        {
            // The finalDestination is known to be a MP -> convert into
            // 4-address and deliver

            // make a partial copy, i.e. only including the commands from
            // 'upper' FUs, including the payload
            dll::UpperCommand* copyUC;
            ForwardingCommand* copyFC;
            wns::ldk::CompoundPtr partialCompoundCopy = createForwardingCopy(compound, fc, copyFC, copyUC);

            sendFrameToMP(partialCompoundCopy, copyFC, copyUC,
                          fc->peer.originalSource,
                          fc->peer.finalDestination);
        } // end destination is MP
        else
        {
            // The finalDestination is non-MP, hopefully we know the proxy

            dll::UpperCommand* copyUC;
            ForwardingCommand* copyFC;
            wns::ldk::CompoundPtr partialCompoundCopy = createForwardingCopy(compound, fc, copyFC, copyUC);

            if(!sendFrameToOtherBSS(partialCompoundCopy, copyFC, copyUC,
                                    fc->peer.originalSource,
                                    layer2->getDLLAddress(),
                                    fc->peer.finalDestination))

            {
                // Could not deliver the frame, give it to the RANG
                MESSAGE_SINGLE(NORMAL, this->logger, "frame destination unknown, give to RANG");
                uc->peer.targetMACAddress = fc->peer.finalDestination;
                getDeliverer()->getAcceptor(compound)->onData(compound);
            } // end destination is unknown
        } // end destination is NOT MP
    } // end delivery by root MP
} // end doOnDataAP


bool MeshForwarding::doOnDataFRS(const wns::ldk::CompoundPtr& compound, ForwardingCommand*& fc, dll::UpperCommand*& uc)
{
    assure(fc->peer.toDS, "received frame NOT to DS");

    if(!fc->peer.fromDS)
    {
        // The compound should be received from an associated STA
        assure(layer2->isTransceiverMAC(ps->getProxyFor(uc->peer.sourceMACAddress)),
               "Recieved compound from " << uc->peer.sourceMACAddress << " with fromDS=false, but source is not proxied by me");

        assure(this->layer2->
               getControlService<dll::services::control::Association>("ASSOCIATION")->hasAssociated(uc->peer.sourceMACAddress),
               "Received compound from " << uc->peer.sourceMACAddress << " with fromDS=false, but source is not associated to any of my transceivers");

        assure(!ps->isMeshPoint(uc->peer.sourceMACAddress),
               "Recieved compound from " << uc->peer.sourceMACAddress << " with fromDS=false, but source is registered as MP");

        MESSAGE_SINGLE(NORMAL, this->logger, "received frame from proxied client "
                       << uc->peer.sourceMACAddress << " to " << fc->peer.finalDestination);

        if(!fc->peer.finalDestination.isValid())
        {
            // The STA does not know the MAC-Address of its portal; hence it
            // sends it to -1
            fc->peer.finalDestination = ps->getPortalFor(uc->peer.sourceMACAddress);
            if(fc->peer.finalDestination == layer2->getDLLAddress())
            {
                // I am the portal -> no FRS service required
                MESSAGE_SINGLE(NORMAL, this->logger, "STA is associated directly to the portal -> delivery to RANG");
                uc->peer.targetMACAddress = fc->peer.finalDestination;
                getDeliverer()->getAcceptor(compound)->onData(compound);
                return true;
            }
            if(!fc->peer.finalDestination.isValid())
            {
                MESSAGE_SINGLE(NORMAL, this->logger, "proxy has no portal -> no delivery ");
                return false;
            }
            MESSAGE_SINGLE(NORMAL, this->logger, "changing finalDestination to portal " << fc->peer.finalDestination);
        }

        dll::UpperCommand* copyUC;
        ForwardingCommand* copyFC;
        wns::ldk::CompoundPtr partialCompoundCopy = createForwardingCopy(compound, fc, copyFC, copyUC);

        if (layer2->getControlService<dll::services::control::Association>("ASSOCIATION")->hasAssociated(fc->peer.finalDestination))
        {
            // AP forwarding from the DS
            sendFrameToOwnBSS(partialCompoundCopy, copyFC, copyUC, uc->peer.sourceMACAddress, fc->peer.finalDestination);
            return true;
        }
        else
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "doOnDataFRS: Destination " << fc->peer.finalDestination << " is not directly associated -> try forwarding");

            if(ps->isMeshPoint(fc->peer.finalDestination))
            {
                MESSAGE_SINGLE(NORMAL, this->logger, "doOnDataFRS: Destination " << fc->peer.finalDestination << " is known MP -> forward");
                sendFrameToMP(partialCompoundCopy, copyFC, copyUC, uc->peer.sourceMACAddress, fc->peer.finalDestination);
                return true;
            }
            else
            {
                sendFrameToOtherBSS(partialCompoundCopy, copyFC, copyUC,
                                    uc->peer.sourceMACAddress,                                          // src
                                    layer2->getDLLAddress(),  // MeshSrc
                                    fc->peer.finalDestination);                                         // finalDst
                return true;
            }
        }
    }

    // Compound is fromDS

    wns::service::dll::UnicastAddress finalMeshDst;
    if(fc->peer.addressExtension)
    {
        finalMeshDst = fc->peer.meshDestination;
    }
    else
    {
        finalMeshDst = fc->peer.finalDestination;
        assure((layer2->isTransceiverMAC(finalMeshDst) == false) ||
               (finalMeshDst == layer2->getDLLAddress()),
               "Final Destination is Transceiver MAC " << finalMeshDst);
    }

    if((finalMeshDst == layer2->getDLLAddress()) ||
       (layer2->isTransceiverMAC(finalMeshDst)))
    {
        // I am the final mesh destination, so it goes
        //  a) to me directly
        //  b) to one of my clients
        //  c) to one of my transceivers and there to the client
        //  [d) to somewhere else, if I am a portal]
        MESSAGE_BEGIN(NORMAL, this->logger, m, "doOnData received frame from ");
        m << uc->peer.sourceMACAddress;
        m << " which has reached its final mesh destination towards ";
        m << fc->peer.finalDestination;
        MESSAGE_END();

        if(!fc->peer.addressExtension)
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "Final destination reached, delivering to upper layer");
            getDeliverer()->getAcceptor(compound)->onData(compound);
            return true;
        } // end finalDestination is me
        if((layer2->getDLLAddress() == ps->getProxyFor(fc->peer.finalDestination)) ||
           (layer2->isTransceiverMAC(ps->getProxyFor(fc->peer.finalDestination))))
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "finalDestination is proxied by me, delivering");

            dll::UpperCommand* copyUC;
            ForwardingCommand* copyFC;
            wns::ldk::CompoundPtr partialCompoundCopy = createForwardingCopy(compound, fc, copyFC, copyUC);

            sendFrameToOwnBSS(partialCompoundCopy, copyFC, copyUC, fc->peer.originalSource, fc->peer.finalDestination);
            return true;
        } // end finalDestination is proxied by me

        MESSAGE_SINGLE(NORMAL, this->logger, "finalDestination is not proxied by me -> unknown");
        return false;
    } // end meshDestination is me
    else
    {
        // I am not the meshDestination -> forward inside the mesh to
        // meshDestination if known
        MESSAGE_BEGIN(NORMAL, this->logger, m, "doOnData received frame from ");
        m << uc->peer.sourceMACAddress;
        m << " to be forwarded to mesh destination ";
        m << finalMeshDst;
        MESSAGE_END();

        --fc->peer.TTL;
        if(fc->peer.TTL == 0)
        {
            MESSAGE_SINGLE(NORMAL, this->logger, "TTL has reached zero, dropping frame");
            return true;
        } // end fc->TTL == 0

        dll::UpperCommand* copyUC;
        ForwardingCommand* copyFC;
        wns::ldk::CompoundPtr partialCompoundCopy = createForwardingCopy(compound, fc, copyFC, copyUC);

        copyFC->peer = fc->peer;
        copyUC->peer.sourceMACAddress = layer2->getDLLAddress();
        copyUC->peer.targetMACAddress = ps->getNextHop(copyUC->peer.sourceMACAddress, finalMeshDst);
        if(layer2->isTransceiverMAC(copyUC->peer.targetMACAddress))
        {
            // the next hop is simply to the own transceiver -> do one more hop
            MESSAGE_BEGIN(NORMAL, this->logger, m, "doOnDataFRS:");
            m << "next hop is own transceiver " << copyUC->peer.targetMACAddress;
            m << " -> one more hop to " << ps->getNextHop(copyUC->peer.targetMACAddress, finalMeshDst);
            MESSAGE_END();

            copyUC->peer.sourceMACAddress = copyUC->peer.targetMACAddress;
            copyUC->peer.targetMACAddress = ps->getNextHop(copyUC->peer.sourceMACAddress, finalMeshDst);
            copyFC->magic.path.push_back(copyUC->peer.sourceMACAddress);
        }

        if(!copyUC->peer.targetMACAddress.isValid())
        {
            MESSAGE_BEGIN(NORMAL, this->logger, m, "Path to meshDestination ");
            m << finalMeshDst;
            m << " is unknown -> drop frame";
            MESSAGE_END();
            return true;
        } // end no path found, drop
        else
        {
            MESSAGE_BEGIN(NORMAL, this->logger, m, "NextHop to meshDestination ");
            m << finalMeshDst;
            m << " is ";
            m << copyUC->peer.sourceMACAddress;
            m << " -> " << copyUC->peer.targetMACAddress;
            MESSAGE_END();
            getConnector()->getAcceptor(partialCompoundCopy)->sendData(partialCompoundCopy);
            return true;
        } // end path found, delivering
    } // end meshDestination is not me

} // end doOnDataFRS

void
MeshForwarding::doSendData(const wns::ldk::CompoundPtr& compound)
{
    // Received fresh packet from upper layer -> activate command
    ForwardingCommand* fc = activateCommand(compound->getCommandPool());
    fc->magic.path.push_back(layer2->getDLLAddress());
    // Get the upperCommand, which contains the final source and destination
    dll::UpperCommand* uc = getFUN()->getProxy()->getCommand<dll::UpperCommand>(compound->getCommandPool(), ucName);

    // this frame comes from the upperConvergence
    assure(uc->peer.sourceMACAddress.isValid(), "uc->peer.sourceMACAddress is not a valid MAC address");

    // locally generated traffic, destination not known
    if (not uc->peer.targetMACAddress.isValid())
    {
        uc->peer.targetMACAddress = ps->getPortalFor(uc->peer.sourceMACAddress);
    }

    fc->magic.isUplink = false;

    if (layer2->getControlService<dll::services::control::Association>("ASSOCIATION")
        ->hasAssociated(uc->peer.targetMACAddress))
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "Destination " << uc->peer.targetMACAddress << " is directly associated -> deliver");
        sendFrameToOwnBSS(compound, fc, uc, uc->peer.sourceMACAddress, uc->peer.targetMACAddress);
        return;
    }
    else
    {
        if(ps->isMeshPoint(uc->peer.targetMACAddress))
        {
            assure(layer2->isTransceiverMAC(uc->peer.targetMACAddress) == false,
                   "Final destination is own transceiver MAC");

            MESSAGE_SINGLE(NORMAL, this->logger, "doSendDataAP: Destination " << uc->peer.targetMACAddress << " is known MP -> forward");

            assure((layer2->getStationType() != wns::service::dll::StationTypes::AP()) or
                   (ps->isPortal(ps->getNextHop(layer2->getDLLAddress(), uc->peer.targetMACAddress)) == false),
                   "AP cannot forward to an other portal --> RANG has used outdated configuration!");

            sendFrameToMP(compound, fc, uc, layer2->getDLLAddress(), uc->peer.targetMACAddress);
        } // end isMeshPoint
        else
        {
            sendFrameToOtherBSS(compound, fc, uc,
                                layer2->getDLLAddress(),       // src
                                layer2->getDLLAddress(),       // MeshSrc
                                uc->peer.targetMACAddress);                                              // finalDst
            return;
        }
    } // end hasAssociated
}

void MeshForwarding::sendFrameToOwnBSS(const wns::ldk::CompoundPtr& compound,
                                       ForwardingCommand*& fc,
                                       dll::UpperCommand*& uc,
                                       wns::service::dll::UnicastAddress src,
                                       wns::service::dll::UnicastAddress dst)
{
    assure(layer2->getControlService<dll::services::control::Association>("ASSOCIATION")->hasAssociated(dst),
           dst << " is not associated to me");
    assure(ps->getProxyFor(dst).isValid(), "Proxy for dst " << dst << " is unknown");
    assure(layer2->isTransceiverMAC(ps->getProxyFor(dst)),
           "Proxy " << ps->getProxyFor(dst) << " for dst " << dst << " is not a transceiver MAC");

    fc->peer.toDS = false;
    fc->peer.fromDS = true;
    fc->peer.originalSource = src;
    uc->peer.sourceMACAddress = ps->getProxyFor(dst);
    uc->peer.targetMACAddress = dst;

    getConnector()->getAcceptor(compound)->sendData(compound);
}

bool MeshForwarding::sendFrameToMP(const wns::ldk::CompoundPtr& compound,
                                   ForwardingCommand*& fc,
                                   dll::UpperCommand*& uc,
                                   wns::service::dll::UnicastAddress src,
                                   wns::service::dll::UnicastAddress mpDst)
{
    assure(ps->isMeshPoint(mpDst), mpDst << " is not a known MP");

    fc->peer.TTL = dot11MeshTTL;
    fc->peer.toDS = true;
    fc->peer.fromDS = true;
    fc->peer.addressExtension = false;
    fc->peer.originalSource = src;
    fc->peer.finalDestination = mpDst;
    uc->peer.sourceMACAddress = layer2->getDLLAddress();
    uc->peer.targetMACAddress = ps->getNextHop(uc->peer.sourceMACAddress, mpDst);

    if(layer2->isTransceiverMAC(uc->peer.targetMACAddress))
    {
        // the next hop is simply to the own transceiver -> do one more hop
        MESSAGE_BEGIN(NORMAL, this->logger, m, "sendFrameToMP:");
        m << "next hop is own transceiver " << uc->peer.targetMACAddress;
        m << " -> one more hop to MP " << ps->getNextHop(uc->peer.targetMACAddress, mpDst);
        MESSAGE_END();

        uc->peer.sourceMACAddress = uc->peer.targetMACAddress;
        uc->peer.targetMACAddress = ps->getNextHop(uc->peer.sourceMACAddress, mpDst);

        fc->magic.path.push_back(uc->peer.sourceMACAddress);
    }
    if(!uc->peer.targetMACAddress.isValid())
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "sendFrameToMP: finalDestination address " << mpDst << "unknown -> drop");
        return false;
    }

    MESSAGE_SINGLE(NORMAL, this->logger, "sendFrameToMP: Forward to " << uc->peer.targetMACAddress);
	
    getConnector()->getAcceptor(compound)->sendData(compound);
    return true;
}

bool MeshForwarding::sendFrameToOtherBSS(const wns::ldk::CompoundPtr& compound,
                                         ForwardingCommand*& fc,
                                         dll::UpperCommand*& uc,
                                         wns::service::dll::UnicastAddress src,
                                         wns::service::dll::UnicastAddress meshSrc,
                                         wns::service::dll::UnicastAddress dst)
{
    fc->peer.TTL = dot11MeshTTL;
    fc->peer.toDS = true;
    fc->peer.fromDS = true;
    fc->peer.addressExtension = true;
    fc->peer.originalSource = src;
    fc->peer.meshSource = meshSrc;
    fc->peer.meshDestination = ps->getProxyFor(dst);

    if(!fc->peer.meshDestination.isValid())
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "sendFrameToOtherBSS: proxy for " << dst << "unknown -> drop");
        return false;
    }

    fc->peer.finalDestination = dst;

    uc->peer.sourceMACAddress = layer2->getDLLAddress();
    uc->peer.targetMACAddress = ps->getNextHop(uc->peer.sourceMACAddress, dst);

    if(layer2->isTransceiverMAC(uc->peer.targetMACAddress))
    {
        // the next hop is simply to the own transceiver -> do one more hop
        MESSAGE_BEGIN(NORMAL, this->logger, m, "sendFrameToOtherBSS:");
        m << "next hop is own transceiver " << uc->peer.targetMACAddress;
        m << " -> one more hop to MP " << ps->getNextHop(uc->peer.targetMACAddress, dst);
        MESSAGE_END();

        uc->peer.sourceMACAddress = uc->peer.targetMACAddress;
        uc->peer.targetMACAddress = ps->getNextHop(uc->peer.sourceMACAddress, dst);

        fc->magic.path.push_back(uc->peer.sourceMACAddress);
    }
    if(!uc->peer.targetMACAddress.isValid())
    {
        MESSAGE_SINGLE(NORMAL, this->logger, "sendFrameToMP: finalDestination address " << dst << "unknown -> drop");
        return false;
    }

    MESSAGE_SINGLE(NORMAL, this->logger, "daSendDataAP: Forward to " << uc->peer.targetMACAddress);
    getConnector()->getAcceptor(compound)->sendData(compound);
    return true;
}

wns::ldk::CompoundPtr
MeshForwarding::createForwardingCopy(const wns::ldk::CompoundPtr& compound,
                                     ForwardingCommand* originalFC,
                                     ForwardingCommand*& copyFC,
                                     dll::UpperCommand*& copyUC)
{
    wns::ldk::CommandPool* copyCommandPool = getFUN()->getProxy()->createCommandPool();
    getFUN()->getProxy()->partialCopy(this, copyCommandPool, compound->getCommandPool());
    wns::ldk::CompoundPtr partialCompoundCopy(new wns::ldk::Compound(copyCommandPool, compound->getData()));

    copyUC = getFUN()->getProxy()->getCommand<dll::UpperCommand>(partialCompoundCopy->getCommandPool(), ucName);
    copyFC = activateCommand(partialCompoundCopy->getCommandPool());
    copyFC->magic = originalFC->magic;

    return(partialCompoundCopy);
}

void
MeshForwarding::printForwardingInformation(ForwardingCommand* fc, dll::UpperCommand* uc) const
{
    MESSAGE_BEGIN(NORMAL, this->logger, m, "Compound with fromDS " << fc->peer.fromDS << " toDS " << fc->peer.toDS << ":");
    if(fc->peer.fromDS == true)
    {
        m << " " << fc->peer.originalSource << " ->";
    }
    if(fc->peer.addressExtension == true)
    {
        m << " " << fc->peer.meshSource << " ->";
    }
    m << " " << uc->peer.sourceMACAddress << " -> " << uc->peer.targetMACAddress << " -> ";
    if(fc->peer.addressExtension == true)
    {
        m << " " << fc->peer.meshDestination << " ->";
    }
    if(fc->peer.toDS == true)
    {
        m << fc->peer.finalDestination;
    }
    m << "; hc: " << fc->magic.hopCount;

    m << "\nFull path:";
    for (unsigned int hop = 0; hop < fc->magic.path.size(); hop++)
    {
        m << " " << fc->magic.path[hop];
    }

    if(((fc->peer.fromDS == true) &&
        (fc->peer.originalSource == layer2->getDLLAddress()))
       ||
       ((fc->peer.addressExtension == true) &&
        (fc->peer.meshSource == layer2->getDLLAddress())))
    {
        // We have a loop in the path, which may occur e.g. due to
        // pathselection-table changes during the transmission process.
        m << "\nThere is a loop in the path!";
    }

    MESSAGE_END();
}
