//
// Copyright 2013-2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/transport/nirio/niriok_proxy.h>
#include <uhd/transport/nirio/niriok_proxy_impl_v1.h>
#include <uhd/transport/nirio/niriok_proxy_impl_v2.h>
#include <cstring>

// "push" and "pop" introduced in GCC 4.6; works with all clang
#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic push
#endif
#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace uhd { namespace niusrprio
{
    // initialization of static members
    boost::shared_mutex niriok_proxy::_synchronization;

    //-------------------------------------------------------
    // niriok_proxy
    //-------------------------------------------------------
    niriok_proxy::niriok_proxy(): _device_handle(nirio_driver_iface::INVALID_RIO_HANDLE)
    {
    }

    niriok_proxy::~niriok_proxy()
    {
    }
  
   niriok_proxy::sptr niriok_proxy::make_and_open(const std::string& interface_path)
   {
      nirio_status status;
     
      /*
         niriok_proxy_impl_v1 supports NI-RIO 13.0
         niriok_proxy_impl_v2 supports NI-RIO 14.0 and later

         We must dynamically determine which version of the RIO kernel we are 
         interfacing to.  Opening the interface will fail if there is a version
         incompatibility, so we try to open successively newer interface 
         proxies until it succeeds.
      */

      sptr proxy_v1(new niriok_proxy_impl_v1);
      status = proxy_v1->open(interface_path);

      if (nirio_status_not_fatal(status))
         return proxy_v1;

      sptr proxy_v2(new niriok_proxy_impl_v2);
      status = proxy_v2->open(interface_path);

      if (nirio_status_not_fatal(status))
         return proxy_v2;

      throw uhd::runtime_error("Unable to detect a supported version of the NI-RIO kernel interface.");
      
   }
}}

#if defined(__clang__) || defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC_MINOR__ > 5)
    #pragma GCC diagnostic pop
#endif
