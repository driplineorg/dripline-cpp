/*
 * labview_service.cc
 *
 *  Created on: Aug 20, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "labview_service.hh"

#include "param.hh"
#include "param_json.hh"


namespace dripline
{
    std::string labview_sys_interface::perform_get( const std::string& a_thing )
    {
        scarab::param_node t_node;
        t_node.add( "value", 5 );
        std::string t_node_string;
        scarab::param_output_json t_codec;
        t_codec.write_string( t_node, t_node_string );
        return t_node_string;
    }


    labview_service::labview_service( const scarab::param_node& a_config ) :
            service( a_config )
    {
    }

    labview_service::~labview_service()
    {
    }

    reply_ptr_t labview_service::do_get_request( const request_ptr_t a_request )
    {
        std::string t_reply_payload_string = labview_sys_interface::perform_get( a_request->parsed_specifier().unparsed() );
        scarab::param_input_json t_codec;
        scarab::param_ptr_t t_payload_ptr = t_codec.read_string( t_reply_payload_string );
        reply_ptr_t t_reply = a_request->reply( dl_success(), "Get operation succeeded", std::move(t_payload_ptr) );
        return t_reply;
    }

    /*
    reply_ptr_t do_set_request( const request_ptr_t a_request )
    {

    }

    reply_ptr_t do_cmd_request( const request_ptr_t a_request )
    {

    }
*/

} /* namespace dripline */
