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

#ifndef WIFIMAC_LOWERMAC_TIMING_TESTS_BACKOFF_HPP
#define WIFIMAC_LOWERMAC_TIMING_TESTS_BACKOFF_HPP

#include <WIFIMAC/lowerMAC/timing/Backoff.hpp>

#include <WNS/events/MemberFunction.hpp>
#include <WNS/CppUnit.hpp>

#include <WNS/pyconfig/Parser.hpp>


namespace wifimac { namespace lowerMAC { namespace timing { namespace tests {

    class BackoffTest:
        public wns::TestFixture
    {
        CPPUNIT_TEST_SUITE( BackoffTest );
        CPPUNIT_TEST ( onTXrequestDuringBusy );
        CPPUNIT_TEST ( onTXrequestDuringIdle );
        CPPUNIT_TEST ( averageBackoff        );
        CPPUNIT_TEST_SUITE_END();

        class BackoffObserverMock :
            public virtual BackoffObserver
        {
        public:
            BackoffObserverMock():
                cBackoffExpired(0),
                txWaiting(false)
                {
                }

            virtual void backoffExpired()
                {
                    ++cBackoffExpired;
                }
            virtual bool hasTransmissionWaiting() const
                {
                    return txWaiting;
                }
            int cBackoffExpired;
            bool txWaiting;
        };

        /**
		 * @brief Enable deterministic backoff
		 **/
        class BackoffFixed:
            public Backoff
        {
        public:
            BackoffFixed(BackoffObserver* backoffObserver,
                         const wns::pyconfig::View& config):
                Backoff(backoffObserver, config)
                {
                }

            // Fix counter to have deterministic tests
            virtual void fixBackoffCounter(int slots)
                {
                    this->counter = slots;
                }
        };

        // members
        BackoffFixed* bo;
        BackoffObserverMock* boObserver;

        const wns::simulator::Time sifsDuration;
        const wns::simulator::Time slotDuration;
        const wns::simulator::Time ackDuration;
        const wns::simulator::Time aifsDuration;
        const wns::simulator::Time eifsDuration;

        const int cwMin;
        const int cwMax;

        // used to send events to backoff entity
        typedef wns::events::MemberFunction<Backoff> BackoffEvent;

    public:
        BackoffTest();

    private:
        virtual void prepare();
        virtual void cleanup();

        // the tests
        void onTXrequestDuringBusy();
        void onTXrequestDuringIdle();
        void averageBackoff();

        // support functions to generate random samples
        double generateBackoffSample();
    };

} // tests
} // timing
} // lowerMAC
} // wifimac

#endif
