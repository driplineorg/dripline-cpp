/*
 * run_simple_service.hh
 *
 *  Created on: Aug 23, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINECPP_EXAMPLES_RUN_SIMPLE_SERVICE_HH_
#define DRIPLINECPP_EXAMPLES_RUN_SIMPLE_SERVICE_HH_

#include "service.hh"

namespace dripline
{

    class run_simple_service : public service
    {
        public:
            run_simple_service( const scarab::param_node& a_config = scarab::param_node() );
            virtual ~run_simple_service();

            void execute();

            virtual reply_info do_run_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual reply_info do_get_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual reply_info do_set_request( const request_ptr_t a_request, reply_package& a_reply_pkg );
            virtual reply_info do_cmd_request( const request_ptr_t a_request, reply_package& a_reply_pkg );

            mv_accessible( int, return );
    };

    inline reply_info run_simple_service::do_run_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::success, "Congrats, you performed an OP_RUN" );;
    }

    inline reply_info run_simple_service::do_get_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::success, "Congrats, you performed an OP_GET" );;
    }

    inline reply_info run_simple_service::do_set_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::success, "Congrats, you performed an OP_SET" );;
    }

    inline reply_info run_simple_service::do_cmd_request( const request_ptr_t, reply_package& a_reply_pkg )
    {
        return a_reply_pkg.send_reply( retcode_t::success, "Congrats, you performed an OP_CMD" );;
    }



} /* namespace dripline */

#endif /* DRIPLINECPP_EXAMPLES_RUN_SIMPLE_SERVICE_HH_ */
