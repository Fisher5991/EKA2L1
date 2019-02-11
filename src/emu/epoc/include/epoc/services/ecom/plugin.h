/*
 * Copyright (c) 2019 EKA2L1 Team.
 * 
 * This file is part of EKA2L1 project 
 * (see bentokun.github.com/EKA2L1).
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <epoc/loader/rsc.h>
#include <cstdint>
#include <vector>

namespace eka2l1 {
    struct ecom_implementation_info {
        std::uint32_t uid;
        std::uint8_t version;
        std::uint8_t format;

        std::u16string display_name;
        std::string default_data;
        std::string opaque_data;

        bool rom {false};

        // A implementation may covers other interface ?
        std::vector<std::uint32_t> extended_interfaces;
    };

    struct ecom_interface_info {
        std::uint32_t uid;
        std::vector<ecom_implementation_info> implementations;
    };

    struct ecom_plugin {
        std::uint32_t type;
        std::uint32_t uid;

        std::vector<ecom_interface_info> interfaces;
    };

    enum ecom_type {
        ecom_plugin_type_2 = 0x101FB0B9,
        ecom_plugin_type_3 = 0x10009E47
    };
    
    /*! \brief Load a plugin from described resource file
     *
     * \returns True if success.
    */
    bool load_plugin(loader::rsc_file &rsc, ecom_plugin &plugin);    
};