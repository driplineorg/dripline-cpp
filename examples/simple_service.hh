/*
 * run_simple_service.hh
 *
 *  Created on: Aug 23, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINECPP_EXAMPLES_RUN_SIMPLE_SERVICE_HH_
#define DRIPLINECPP_EXAMPLES_RUN_SIMPLE_SERVICE_HH_

#include "service.hh"

#include "logger.hh"

LOGGER( rsslog, "run_simple_service" );

namespace dripline
{

    class DRIPLINE_EXAMPLES_API run_simple_service : public service
    {
        public:
            run_simple_service( const scarab::param_node& a_config = scarab::param_node() );
            virtual ~run_simple_service();

            void execute();

            virtual reply_ptr_t do_run_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_get_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_set_request( const request_ptr_t a_request );
            virtual reply_ptr_t do_cmd_request( const request_ptr_t a_request );

            mv_accessible( int, return );
    };

    inline reply_ptr_t run_simple_service::do_run_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_success(), "Congrats, you performed an OP_RUN" );
    }

    inline reply_ptr_t run_simple_service::do_get_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_success(), "Congrats, you performed an OP_GET" );

    }

    inline reply_ptr_t run_simple_service::do_set_request( const request_ptr_t a_request )
    {
        return a_request->reply( dl_success(), "Congrats, you performed an OP_SET" );
    }

    inline reply_ptr_t run_simple_service::do_cmd_request( const request_ptr_t a_request )
    {
        // Example of doing something based on the message specifier
        unsigned t_spec_level = 0;
        while( ! a_request->parsed_specifier().empty() )
        {
            ++t_spec_level;
            LINFO( rsslog, "Specifier level " << t_spec_level << ": " << a_request->parsed_specifier().front() );
            a_request->parsed_specifier().pop_front();
        }
        return a_request->reply( dl_success(), "Congrats, you performed an OP_CMD that had " + std::to_string(t_spec_level) + " levels" );
    }



} /* namespace dripline */

#endif /* DRIPLINECPP_EXAMPLES_RUN_SIMPLE_SERVICE_HH_ */
