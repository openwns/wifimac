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

#ifndef WIFIMAC_CONVERGENCE_ITXSTARTEND_HPP
#define WIFIMAC_CONVERGENCE_ITXSTARTEND_HPP

#include <WNS/Subject.hpp>
#include <WNS/ldk/Compound.hpp>

namespace wifimac { namespace convergence {

    enum TxStartEndState {
        start,
        end
    };

	class ITxStartEnd
	{
	public:
		virtual ~ITxStartEnd()
			{}

		virtual void onTxStart(const wns::ldk::CompoundPtr& compound) = 0;
        virtual void onTxEnd(const wns::ldk::CompoundPtr& compound) = 0;
	};

	class TxStartEndNotification :
		virtual public wns::Subject<ITxStartEnd>
	{
	public:

		struct OnTxStartEnd
		{
			OnTxStartEnd(const wns::ldk::CompoundPtr& _compound, const TxStartEndState _state):
				compound(_compound),
                state(_state)
				{}

			void operator()(ITxStartEnd* tx)
				{
                    assure(compound, "compound is NULL");

                    if(state == start)
                    {
                        // The functor calls the txStartIndication implemented by the
                        // Observer
                        tx->onTxStart(compound);
                    }
                    else
                    {
                        // The functor calls the txEndIndication implemented by the
                        // Observer
                        tx->onTxEnd(compound);
                    }
				}
        private:
            wns::ldk::CompoundPtr compound;
            TxStartEndState state;
		};
	};
}
}

#endif // not defined WIFIMAC_SCHEDULER_TXSTARTENDINTERFACE_HPP
