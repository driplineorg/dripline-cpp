/*
 * specifier.cc
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "specifier.hh"

namespace dripline
{

    routing_key_specifier::routing_key_specifier( const std::string& a_rk ) :
            deque()
    {
        add_next( a_rk );
    }

    routing_key_specifier::~routing_key_specifier()
    {
    }

    void routing_key_specifier::parse( const std::string& a_rk )
    {
        while( ! empty() ) pop_front();

        add_next( a_rk );
        return;
    }

    std::string routing_key_specifier::to_string() const
    {
        std::string t_return;
        for( container_type::const_iterator t_it = this->begin(); t_it != this->end(); ++t_it )
        {
            if( t_it != this->begin() ) t_return += f_node_separator;
            t_return += *t_it;
        }
        return t_return;
    }

    void routing_key_specifier::add_next( const std::string& a_addr )
    {
        size_t t_div_pos = a_addr.find( f_node_separator );
        if( t_div_pos == a_addr.npos )
        {
            push_back( a_addr );
            return;
        }
        push_back( a_addr.substr( 0, t_div_pos ) );
        add_next( a_addr.substr( t_div_pos + 1 ) );
        return;
    }


    specifier::specifier( const std::string& a_unparsed ) :
            deque(),
            f_unparsed( a_unparsed )
    {
        add_next( a_unparsed );
    }

    specifier::specifier( const specifier& a_orig ) :
            deque( a_orig ),
            f_unparsed( a_orig.f_unparsed )
    {}

    specifier::~specifier()
    {
    }

    void specifier::parse( const std::string& a_unparsed )
    {
        while( ! empty() ) pop_front();

        f_unparsed = a_unparsed;

        add_next( a_unparsed );
        return;
    }

    std::string specifier::to_string() const
    {
        std::string t_return;
        for( container_type::const_iterator t_it = this->begin(); t_it != this->end(); ++t_it )
        {
            if( t_it != this->begin() ) t_return += f_node_separator;
            t_return += *t_it;
        }
        return t_return;
    }

    void specifier::add_next( const std::string& a_addr )
    {
        size_t t_div_pos = a_addr.find( f_node_separator );
        if( t_div_pos == a_addr.npos )
        {
            push_back( a_addr );
            return;
        }
        push_back( a_addr.substr( 0, t_div_pos ) );
        add_next( a_addr.substr( t_div_pos + 1 ) );
        return;
    }


} /* namespace dripline */
