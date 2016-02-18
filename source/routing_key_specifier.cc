/*
 * routing_key_specifier.cc
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#include "routing_key_specifier.hh"

namespace dripline
{

    routing_key_specifier::routing_key_specifier( const std::string& a_rk ) :
            queue()
    {
        add_next( a_rk );
    }

    routing_key_specifier::~routing_key_specifier()
    {
    }

    void routing_key_specifier::parse( const std::string& a_rk )
    {
        while( ! empty() ) pop();

        add_next( a_rk );
        return;
    }

    void routing_key_specifier::add_next( const std::string& a_addr )
    {
        size_t t_div_pos = a_addr.find( f_node_separator );
        if( t_div_pos == a_addr.npos )
        {
            push( a_addr );
            return;
        }
        push( a_addr.substr( 0, t_div_pos ) );
        add_next( a_addr.substr( t_div_pos + 1 ) );
        return;
    }


} /* namespace dripline */
