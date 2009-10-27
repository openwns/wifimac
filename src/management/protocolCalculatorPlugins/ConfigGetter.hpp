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

#ifndef WIFIMAC_MANAGEMENT_PROTOCOLCALCULATORPLUGINS_CONFIGGETTER_HPP
#define WIFIMAC_MANAGEMENT_PROTOCOLCALCULATORPLUGINS_CONFIGGETTER_HPP

// must be the first include!
#include <WNS/Python.hpp>
#include <WNS/simulator/Bit.hpp>

namespace wifimac { namespace management { namespace protocolCalculatorPlugins {

    class ConfigGetter
    {
    public:
        ConfigGetter(PyObject* config_) :
            config(config_)
            {
                Py_XINCREF(config);
            };

        ~ConfigGetter()
            {
                Py_DECREF(config);
            }

        template <class T>
        T
        get(const char* varName, const char* format) const
            {
                T t;
                if(not PyArg_Parse(PyObject_GetAttrString(this->config, varName), format, &t))
                {
                    return 0;
                }
                return t;
            };

        ConfigGetter
        get(const char* varName) const
            {
                PyObject* o;
                if(not PyArg_Parse(PyObject_GetAttrString(this->config, varName), "O", o))
                {
                    return 0;
                }
                return(ConfigGetter(o));
            }

    private:
        PyObject* config;
    };
} // protocolCalculatorPlugins
} // management
} // wifimac

#endif
