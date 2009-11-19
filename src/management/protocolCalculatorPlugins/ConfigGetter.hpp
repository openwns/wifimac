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
#include <WNS/Assure.hpp>

namespace wifimac { namespace management { namespace protocolCalculatorPlugins {

    class ConfigGetter
    {
    public:
        ConfigGetter(PyObject* config_) :
            config(config_)
            {
                //Py_XINCREF(config);
            };

        ~ConfigGetter()
            {
                //Py_XDECREF(config);
            }

        template <class T>
        T
        get(const char* varName, const char* format) const
            {
                T t;
                assure(PyObject_HasAttrString(this->config, varName),
                       "Attribute " << varName << " not found");
                PyObject* obj = PyObject_GetAttrString(this->config, varName);
                if(not PyArg_Parse(obj, format, &t))
                {
                    Py_DECREF(obj);
                    return 0;
                }
                Py_DECREF(obj);
                return t;
            };

        ConfigGetter
        get(const char* varName) const
            {
                assure(PyObject_HasAttrString(this->config, varName),
                       "Attribute " << varName << " not found");
                PyObject* obj = PyObject_GetAttrString(this->config, varName);
                ConfigGetter c(obj);
                Py_DECREF(obj);
                return c;
            };

        ConfigGetter
        get(const char* varName, int i) const
            {
                assure(PyObject_HasAttrString(this->config, varName),
                       "Attribute " << varName << " not found");
                PyObject* obj = PyObject_GetAttrString(this->config, varName);
                int n = PyList_Size(obj);
                assure(i < n, "List has size " << n << ", cannot get element " << i);
                PyObject* item = PyList_GetItem(obj, i);
                ConfigGetter c(item);
                Py_DECREF(obj);
                return c;
            };

        int
        length(const char* varName) const
            {
                assure(PyObject_HasAttrString(this->config, varName),
                       "Attribute " << varName << " not found");
                PyObject* obj = PyObject_GetAttrString(this->config, varName);
                int n = PyList_Size(obj);
                Py_DECREF(obj);
                return n;
            }

    private:
        PyObject* config;
    };
} // protocolCalculatorPlugins
} // management
} // wifimac

#endif
