/******************************************************************************
 * WiFiMac                                                                    *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 2005-2007                                                    *
 * Lehrstuhl fuer Kommunikationsnetze (ComNets)                               *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                          *
 * www: http://wns.comnets.rwth-aachen.de                                     *
 ******************************************************************************/

#ifndef WIFIMAC_LOWERMAC_ITRANSMISSIONCOUNTER_HPP
#define WIFIMAC_LOWERMAC_ITRANSMISSIONCOUNTER_HPP

namespace wifimac { namespace lowerMAC {

    class ITransmissionCounter
    {
    public:
        virtual ~ITransmissionCounter()
            {}

        virtual size_t
        getTransmissionCounter(const wns::ldk::CompoundPtr& compound) const = 0;

        virtual void
        copyTransmissionCounter(const wns::ldk::CompoundPtr& src, const wns::ldk::CompoundPtr& dst) = 0;

        // indication from outside that the transmission has failed
        virtual void
        transmissionHasFailed(const wns::ldk::CompoundPtr& compound) = 0;

    };
}
}

#endif // WIFIMAC_LOWERMAC_ITRANSMISSIONCOUNTER_HPP
