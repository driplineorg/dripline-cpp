/*
 * routing_key_specifier.hh
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_ROUTING_KEY_SPECIFIER_HH_
#define DRIPLINE_ROUTING_KEY_SPECIFIER_HH_

#include "dripline_api.hh"

#include <deque>
#include <string>

namespace dripline
{

    class DRIPLINE_API routing_key_specifier : public std::deque< std::string >
    {
        public:
            typedef std::deque< std::string > container_type;

        public:
            routing_key_specifier( const std::string& a_rk = "" );
            virtual ~routing_key_specifier();

            void parse( const std::string& a_rk );
            std::string to_string() const;

        private:
            void add_next( const std::string& a_addr );

        public:
            static const char f_node_separator = '.';

    };

} /* namespace dripline */

#endif /* DRIPLINE_ROUTING_KEY_SPECIFIER_HH_ */
