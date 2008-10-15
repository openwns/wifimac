/******************************************************************************
 * WIFIMAC                                                                    *
 * __________________________________________________________________________ *
 *                                                                            *
 * Copyright (C) 1999-2007                                                    *
 * Chair of Communication Networks (ComNets)                                  *
 * Kopernikusstr. 16, D-52074 Aachen, Germany                                 *
 * phone: ++49-241-80-27910 (phone), fax: ++49-241-80-22242                   *
 * email: rise@comnets.rwth-aachen.de                                         *
 * www: http://rise.comnets.rwth-aachen.de                                    *
 ******************************************************************************/

#ifndef WIFIMAC_CONVERGENCE_ICHANNELSTATE_HPP
#define WIFIMAC_CONVERGENCE_ICHANNELSTATE_HPP

#include <WNS/Subject.hpp>

namespace wifimac { namespace convergence {

    // The indicator for the channel state
    enum CS {
        idle,
        busy
    };


    class IChannelState
    {
    public:
        virtual ~IChannelState(){};

        /*** @brief called by ChannelState, implemented by observers on cs ***/
        virtual void onChannelBusy() = 0;
        virtual void onChannelIdle() = 0;
    };

    class ChannelStateNotification :
        virtual public wns::Subject<IChannelState>
    {
    public:
        // @brief functor for IChannelState::onChangedCS calls
        struct OnChangedCS
        {
            OnChangedCS(const CS _newCS):
                newCS(_newCS)
                {}

            void operator()(IChannelState* cs)
                {
                    // The functor calls the onChannelBusy/onChannelIdle implemented by the
                    // Observer
                    if (newCS == idle)
                        cs->onChannelIdle();
                    else
                        cs->onChannelBusy();
                }
        private:
            CS newCS;
        };

    };


}
}

#endif // not defined WIFIMAC_CONVERGENCE_ICHANNELSTATE_HPP
