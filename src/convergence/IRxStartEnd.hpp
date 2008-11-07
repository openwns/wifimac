/******************************************************************************
 * WIFIMAC                                                                    *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 1999-2007                                                    *
 * Chair of Communication Networks (ComNets)                                  *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: wns@comnets.rwth-aachen.de                                         *
 * www: http://rise.comnets.rwth-aachen.de                                    *
 ******************************************************************************/

#ifndef WIFIMAC_CONVERGENCE_IRXSTARTEND_HPP
#define WIFIMAC_CONVERGENCE_IRXSTARTEND_HPP

#include <WNS/Subject.hpp>
#include <WNS/simulator/Time.hpp>

namespace wifimac { namespace convergence {

        class IRxStartEnd
        {
        public:
            virtual ~IRxStartEnd()
                {}

            /*** @brief called by ChannelState, implemented by observers ***/
            virtual void onRxStart(const wns::simulator::Time expRxDuration) = 0;
            virtual void onRxEnd() = 0;
            virtual void onRxError() = 0;
        };

        class RxStartEndNotification :
        virtual public wns::Subject<IRxStartEnd>
        {
        public:
            // @brief functor for IRxStartEnd::rxStartIndicationon calls
            struct OnRxStartEnd
            {
                OnRxStartEnd(const wns::simulator::Time _expRxDuration, const bool _isStart, const bool _rxError):
                    expRxDuration(_expRxDuration),
                    isStart(_isStart),
                    rxError(_rxError)
                    {}

                void operator()(IRxStartEnd* rx)
                    {
                        if(isStart)
                        {
                            rx->onRxStart(expRxDuration);
                        }
                        else
                        {
                            if(rxError)
                            {
                                rx->onRxError();
                            }
                            else
                            {
                                rx->onRxEnd();
                            }
                        }
                    }
            private:
                wns::simulator::Time expRxDuration;
                bool isStart;
                bool rxError;
            };
        };
    }
}

#endif // not defined WIFIMAC_CONVERGENCE_IRXSTARTEND_HPP
