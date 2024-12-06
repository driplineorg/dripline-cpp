/*
 * specifier.cc
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#define DRIPLINE_API_EXPORTS

#include "specifier.hh"

#include "logger.hh"

LOGGER( dlog, "specifier" );

namespace dripline
{

    routing_key::routing_key( const std::string& a_rk ) :
            deque()
    {
        if( ! a_rk.empty() ) add_next( a_rk );
    }

    void routing_key::parse( const std::string& a_rk )
    {
        while( ! empty() ) pop_front();

        add_next( a_rk );
        return;
    }

    std::string routing_key::to_string() const
    {
        std::string t_return;
        for( container_type::const_iterator t_it = this->begin(); t_it != this->end(); ++t_it )
        {
            if( t_it != this->begin() ) t_return += f_node_separator;
            t_return += *t_it;
        }
        return t_return;
    }

    void routing_key::add_next( const std::string& a_addr )
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
            container_type(),
            f_unparsed( a_unparsed )
    {
        LTRACE( dlog, "Creating specifier <" << a_unparsed << ">" );
        if( ! a_unparsed.empty() ) add_next( a_unparsed );
    }

    specifier::specifier( const specifier& a_orig ) :
            container_type( a_orig ),
            f_unparsed( a_orig.f_unparsed )
    {}

    specifier::specifier( specifier&& a_orig ) :
            container_type( a_orig ),
            f_unparsed( std::move( a_orig.f_unparsed ) )
    {}

    specifier::~specifier()
    {
    }

    const specifier& specifier::operator=( const specifier& a_orig )
    {
        container_type::operator=( a_orig );
        f_unparsed = a_orig.f_unparsed;
        return *this;
    }

    const specifier& specifier::operator=( specifier&& a_orig )
    {
        container_type::operator=( a_orig );
        f_unparsed = std::move(a_orig.f_unparsed);
        return *this;
    }

    void specifier::parse( const std::string& a_unparsed )
    {
        LTRACE( dlog, "Parsing <" << a_unparsed << ">" );

        while( ! empty() ) pop_front();

        f_unparsed = a_unparsed;

        if( a_unparsed.empty() ) return;

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
