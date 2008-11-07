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

#ifndef WIFIMAC_PATHSELECTION_NOVELCHECK_HPP
#define WIFIMAC_PATHSELECTION_NOVELCHECK_HPP

#include <map>

namespace wifimac { namespace pathselection {

	class NovelCheck :
	{
	public:

		NovelCheck();

		virtual
		~NovelCheck();

		virtual bool
		isNovel(wns::service::dll::UnicastAddress source,
				wns::SequenceNumber sn,
				pathSelectionMetric metric)
			{
				map<wns::service::dll::UnicastAddress, wns::SequenceNumber>::iterator itr = snMap.find(source);
				if(!itr)
				{
					// seen source for the first time
					snMap[source] = sn;
					metricMap[source] = metric;
					return (true);
				}
				else
				{
					if(itr->second < sn)
					{
						// Old message
						return (false);
					}
					if(itr->second == sn)
					{
						map<wns::service::dll::UnicastAddress, pathSelectionMetric>::iterator itrMetric = metricMap.find(source);
						assureNotNull(itrMetric, "sn found but not metric");
						if(itrMetric < metric)
						{
							// Current message, metric is less -> better
							metricMap[source] = metric;
							return true;
						}
						else
						{
							// Current message, metric is not so good
							return false;
						}
					}
					if(itr->second > sn)
					{
						// new message
						snMap[source] = sn;
						metricMap[source] = metric;
						return true;
					}
				}
			}
	private:
		wns::logger::Logger logger;
		map<wns::service::dll::UnicastAddress, wns::SequenceNumber> snMap;
		map<wns::service::dll::UnicastAddress, pathSelectionMetric> metricMap;
	};


} // pathselection
} // wifimac

#endif // WIFIMAC_SCHEDULER_HPP
