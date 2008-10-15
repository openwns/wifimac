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

#ifndef WIFIMAC_CONVERGENCE_ERRORMODELLING
#define WIFIMAC_CONVERGENCE_ERRORMODELLING

#include <WIFIMAC/convergence/PhyUser.hpp>

#include <WNS/ldk/fu/Plain.hpp>
#include <WNS/ldk/Processor.hpp>
#include <WNS/ldk/ErrorRateProviderInterface.hpp>

#include <WNS/pyconfig/View.hpp>
#include <WNS/logger/Logger.hpp>

namespace wifimac { namespace convergence {

	/**
	 * @brief ErrorModellingCommand, which stores the per/cir
	 */
	class ErrorModellingCommand:
		public wns::ldk::Command,
		public wns::ldk::ErrorRateProviderInterface
	{
	public:
		ErrorModellingCommand()
			{
				local.per = 1;
				local.destructorCalled = NULL;
				local.cir.set_dB(0);
			}

		~ErrorModellingCommand()
			{
				if(local.destructorCalled != NULL)
					*local.destructorCalled = true;
			}
		virtual double getErrorRate() const
			{
				return local.per;
			}

		struct {
			double per;
			long *destructorCalled;
			wns::Ratio cir;
		} local;
		struct {} peer;
		struct {} magic;
	};

	/**
	 * @brief ErrorModelling implementation of the FU.
	 *
	 * It maps the Carry Interference Ratio (CIR) for a PhyMode
	 * to the Packet Error Rate (PER).
	 */
	class ErrorModelling:
		public wns::ldk::fu::Plain<ErrorModelling, ErrorModellingCommand>,
        public wns::ldk::Processor<ErrorModelling>
	{
	public:
		// FUNConfigCreator interface realisation
		ErrorModelling(wns::ldk::fun::FUN* fun, const wns::pyconfig::View& config);

        /**
         * @brief Processor Interface Implementation
         */
        void processIncoming(const wns::ldk::CompoundPtr& compound);
        void processOutgoing(const wns::ldk::CompoundPtr& compound);

	private:
		wns::pyconfig::View config;
		wns::logger::Logger logger;
		const std::string phyUserCommandName;
        const std::string managerCommandName;
	}; // ErrorModelling
} // convergence
} // wifimac

#endif
