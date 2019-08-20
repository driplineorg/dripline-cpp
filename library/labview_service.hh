/*
 * labview_service.hh
 *
 *  Created on: Aug 20, 2019
 *      Author: N.S. Oblath
 */

#ifndef LIBRARY_LABVIEW_SERVICE_HH_
#define LIBRARY_LABVIEW_SERVICE_HH_

#include "service.hh"

namespace dripline
{
    class DRIPLINE_API labview_sys_interface
    {
        protected:
            labview_sys_interface() {}
            ~labview_sys_interface() {}

        public:
            // get interface:
            //    parameters:
            //      - a_thing: specifies what is being retrieved
            //    return: JSON-formatted string with the returned information
            static std::string perform_get( const std::string& a_thing );
    };

    class DRIPLINE_API labview_service : public service
    {
        public:
            labview_service( const scarab::param_node& a_config = scarab::param_node() );
            virtual ~labview_service();

        protected:
            // uses labview_sys_interface::perform_get
            // submits unparsed specifier as the `a_thing` argument
            // takes returned JSON string, converts to param, and adds as the payload of the reply
            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            //virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
            //virtual reply_ptr_t do_cmd_request( const request_ptr_t a_request );

    };

} /* namespace dripline */

#endif /* LIBRARY_LABVIEW_SERVICE_HH_ */
