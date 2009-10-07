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

#include <WIFIMAC/lowerMAC/timing/tests/BackoffTest.hpp>

#include <WNS/events/NoOp.hpp>

using namespace wifimac::lowerMAC::timing::tests;

CPPUNIT_TEST_SUITE_REGISTRATION( BackoffTest );

BackoffTest::BackoffTest():
    wns::TestFixture(),
    bo(NULL),
    boObserver(NULL),
    sifsDuration(9e-6),
    slotDuration(16e-9),
    ackDuration(44e-9),
    aifsDuration(sifsDuration+2*slotDuration),
    eifsDuration(aifsDuration+ackDuration+slotDuration),
    cwMin(15),
    cwMax(1023)
{
}

void BackoffTest::prepare()
{
	assure(this->boObserver == NULL, "not properly deleted");
	assure(this->bo == NULL, "not properly deleted");

	// setup configuration for backoff
	std::stringstream ss;
	ss << "from wifimac.Logger import Logger\n"
	   << "\n"
	   << "backoffLogger = Logger(\"BO\")\n"
	   << "class myConfig:\n"
	   << "  slotDuration = " << slotDuration << "\n"
	   << "  aifsDuration = " << aifsDuration << "\n"
	   << "  cwMin = " << cwMin << "\n"
	   << "  cwMax = " << cwMax << "\n"
	   << "\n";
	wns::pyconfig::Parser parser;
	parser.loadString(ss.str());

	this->boObserver = new BackoffObserverMock();
	this->bo = new BackoffFixed(this->boObserver, parser);
}

void BackoffTest::cleanup()
{
    assure(this->bo != NULL, "not properly created");
    delete this->bo;
    this->bo = NULL;

    assure(this->boObserver != NULL, "not properly created");
    delete this->boObserver;
    this->boObserver = NULL;
}

void BackoffTest::onTXrequestDuringBusy()
{
    int boSlots = 3;
    wns::events::scheduler::Interface* es = wns::simulator::getEventScheduler();

    // wait some time for the initial backoff with an idle channel
    // and then block the channel
    simTimeType nextEventAt = 100*aifsDuration;
    es->schedule(BackoffEvent(this->bo, &Backoff::onChannelBusy), nextEventAt);
    es->start();

    // send tx request
    bool answer = this->bo->transmissionRequest(1);
    // Check that immediate transmission is not allowed
    CPPUNIT_ASSERT_EQUAL(answer, false);
    // Fix counter
    this->bo->fixBackoffCounter(boSlots);

    // free channel after some time
    nextEventAt += 20*aifsDuration;
    es->schedule(BackoffEvent(this->bo, &BackoffFixed::onChannelIdle), nextEventAt);
    es->start();

    simTimeType expected = nextEventAt + aifsDuration + boSlots*slotDuration;

    WNS_ASSERT_MAX_REL_ERROR(expected, es->getTime(), 1E-12);
    CPPUNIT_ASSERT_EQUAL(1, this->boObserver->cBackoffExpired);
}

void BackoffTest::onTXrequestDuringIdle()
{
    wns::events::scheduler::Interface* es = wns::simulator::getEventScheduler();

    // wait some time for the initial backoff with an idle channel
    // and then issue the tx request
    simTimeType nextEventAt = 100*aifsDuration;
    es->schedule(wns::events::NoOp(), nextEventAt);
    es->start();

    bool answer = this->bo->transmissionRequest(1);
    // Check that immediate transmission is allowed
    CPPUNIT_ASSERT_EQUAL(answer, true);
}

void BackoffTest::averageBackoff()
{
    wns::TFunctor<wifimac::lowerMAC::timing::tests::BackoffTest, double> boSample(this, &BackoffTest::generateBackoffSample);

    WNS_ASSERT_MEAN_VALUE(aifsDuration + (cwMin/2)*slotDuration, boSample, 100000);
}

double BackoffTest::generateBackoffSample()
{
    wns::events::scheduler::Interface* es = wns::simulator::getEventScheduler();

    // wait some time for the initial backoff with an idle channel
    // and then block the channel
    int expired = this->boObserver->cBackoffExpired;
    simTimeType nextEventAt = es->getTime() + 100*aifsDuration;
    es->schedule(BackoffEvent(this->bo, &Backoff::onChannelBusy), nextEventAt);
    es->start();

    // send tx request & do NOT fix the bo
    bool answer = this->bo->transmissionRequest(1);
    // Check that immediate transmission is not allowed
    CPPUNIT_ASSERT_EQUAL(answer, false);

    // free channel after some time
    nextEventAt += 20*aifsDuration;
    es->schedule(BackoffEvent(this->bo, &Backoff::onChannelIdle), nextEventAt);
    es->start();

    CPPUNIT_ASSERT_EQUAL(expired+1, this->boObserver->cBackoffExpired);

    return(es->getTime() - nextEventAt);
}

