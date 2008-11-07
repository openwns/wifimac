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

#ifndef WIFIMAC_CONVERGENCE_INETWORKALLOCATIONVECTOR_HPP
#define WIFIMAC_CONVERGENCE_INETWORKALLOCATIONVECTOR_HPP

#include <WNS/Subject.hpp>
#include <WNS/service/dll/Address.hpp>

namespace wifimac { namespace convergence {

	class INetworkAllocationVector
	{
	public:
		virtual ~INetworkAllocationVector(){};

		/*** @brief called by ChannelState, implemented by observers on NAV ***/
		virtual void onNAVBusy(const wns::service::dll::UnicastAddress setter) = 0;
		virtual void onNAVIdle() = 0;
	};

    class NAVNotification :
        virtual public wns::Subject<INetworkAllocationVector>
    {
    public:
        // @brief functor for INetworkAllocationVector::onChangedNAV calls
        struct OnChangedNAV
        {
            OnChangedNAV(const bool _newNAV, const wns::service::dll::UnicastAddress _setter):
                newNAV(_newNAV),
                setter(_setter)
                {}

            void operator()(INetworkAllocationVector* nav)
				{
					// The functor calls the onChannelBusy/onChannelIdle implemented by the
					// Observer
					if (newNAV)
						nav->onNAVBusy(setter);
					else
						nav->onNAVIdle();
				}
		private:
			bool newNAV;
            wns::service::dll::UnicastAddress setter;
		};

	};


}
}

#endif // not defined WIFIMAC_CONVERGENCE_INETWORKALLOCATIONVECTOR_HPP
