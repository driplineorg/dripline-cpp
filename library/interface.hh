/*
 * interface.hh
 *
 *  Created on: Aug 15, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_INTERFACE_HH_
#define DRIPLINE_INTERFACE_HH_

#include "core.hh"

namespace dripline
{

    class DRIPLINE_API interface : public core
    {
        public:
            interface( const scarab::param_node& a_config = scarab::param_node() );
            virtual ~interface();

            sent_msg_pkg_ptr get( const std::string& a_rk, const std::string& a_specifier="" );

            sent_msg_pkg_ptr set( const std::string& a_rk, scarab::param_value a_value, const std::string& a_specifier="", const std::string& a_lockout_key="" );

            sent_msg_pkg_ptr cmd( const std::string& a_rk, const std::string& a_specifier="", const std::string& a_lockout_key="" );
    };

} /* namespace dripline */

#endif /* DRIPLINE_INTERFACE_HH_ */
