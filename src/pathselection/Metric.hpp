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

#ifndef WIFIMAC_PATHSELECTION_METRIC_HPP
#define WIFIMAC_PATHSELECTION_METRIC_HPP

#include <iostream>
#include <cassert>

namespace wifimac { namespace pathselection {

	/**
	 * @brief Metric for the path-selection algorithms
	 *
	 * Valid values are either a double > 0 or inifinity
	 */
	class Metric
	{
	public:

		inline Metric();
		inline Metric(const Metric& m);
		inline Metric(const double& d);

		inline Metric operator - (const Metric& m) const;
		inline void   operator -=(const Metric& m);
		inline Metric operator + (const Metric& m) const;
		inline void   operator +=(const Metric& m);
		inline bool   operator < (const Metric& m) const;
		inline bool   operator > (const Metric& m) const;
		inline bool   operator >=(const Metric& m) const;
		inline bool   operator <=(const Metric& m) const;
		inline bool   operator ==(const Metric& m) const;
		inline bool   operator !=(const Metric& m) const;
		inline void   operator = (const Metric& m);
        inline Metric operator * (const double& d) const;

		inline bool   isInf() const;
		inline bool   isNotInf() const;

		/**
		 * @brief Compares if two metrics are similar to each other by computing their relative deviation
		 */
		inline bool   isNear(const Metric &m, const double limit = 0.1) const;

		/**
		 * @brief Back-conversion to double if not inf
		 */
		inline double toDouble() const;

		friend std::ostream& operator <<(std::ostream &str, const wifimac::pathselection::Metric& m)
		{
			if(m.inf)
			{
				str << "Inf";
			}
			else
			{
				str << m.value;
			}
			return str;
		}
	protected:
		double value;
		bool inf;
	};


	inline Metric::Metric() :
		value(0),
		inf(true)
	{}

	inline Metric::Metric(const Metric& m) :
		value(m.value),
		inf(m.inf)
	{
		assert(m.value >= 0 || inf);
	}

	inline Metric::Metric(const double& d) :
		value(d),
		inf(false)
	{
		assert(d>=0);
	}

	inline Metric Metric::operator - (const Metric &m) const
	{
	    assert(m.value >= 0 || m.inf);
		assert(value >= 0 || inf);
		if (m.inf || inf)
		{
			return(Metric());
		}
		else
		{
			return(Metric(value - m.value));
		}
	}

	inline Metric Metric::operator + (const Metric &m) const
	{
	    assert(m.value >= 0 || m.inf);
		assert(value >= 0 || inf);
		if (m.inf || inf)
		{
			return(Metric());
		}
		else
		{
			return(Metric(value + m.value));
		}
	}

    inline Metric Metric::operator * (const double& d) const
    {
        assert(!inf);
        assert((d >= 0.0) and (d <= 1.0));
        return(Metric(value*d));
    }

	inline bool   Metric::operator < (const Metric &m) const
	{
		// cannot compare two inf's with each other
		assert((!inf) || (!m.inf));

		if(inf)
		{
			return false;
		}
		if(m.inf)
		{
			return true;
		}
		return(value < m.value);
	}

	inline bool   Metric::operator > (const Metric &m) const
	{
		// cannot compare two inf's with each other
		assert((!inf) || (!m.inf));

		if(inf)
		{
			return true;
		}
		if(m.inf)
		{
			return false;
		}
		return(value > m.value);
	}

	inline bool   Metric::operator >=(const Metric &m) const
	{
		return((*this > m) || (*this == m));
	}

	inline bool   Metric::operator <=(const Metric &m) const
	{
		return((*this < m) || (*this == m));
	}

	inline bool   Metric::operator ==(const Metric &m) const
	{
		// cannot compare two inf's with each other
		assert((!inf) || (!m.inf));
		if (inf || m.inf)
		{
			return false;
		}
		else
		{
			return(value == m.value);
		}
	}

	inline bool   Metric::operator !=(const Metric &m) const
	{
		assert((!inf) || (!m.inf));
		if (inf || m.inf)
		{
			return false;
		}
		else
		{
			return(value != m.value);
		}
	}

	inline void   Metric::operator = (const Metric &m)
	{
		assert(m.value >= 0);
		value = m.value;
		inf = m.inf;
	}

	inline bool   Metric::isInf() const
	{
		return (inf);
	}

	inline bool   Metric::isNotInf() const
	{
		return (!inf);
	}
	
	inline bool   Metric::isNear(const Metric &m, const double /*limit*/) const
	{
		if((inf) || (m.inf))
			return(false);
		if(m.value == 0)
			return (value < 0.1 ? true : false);
		if (m.value < value)
			return((value - m.value) < 0.1*value ? true : false);
		else
			return((m.value - value) < 0.1*m.value ? true : false);
	}
	
	inline double Metric::toDouble() const
	{
		assert(!inf);
		return(value);
	}
			
} // pathselection
} // wifimac

#endif // WIFIMAC_SCHEDULER_HPP
