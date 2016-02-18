/*
 * routing_key_specifier.hh
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_ROUTING_KEY_SPECIFIER_HH_
#define DRIPLINE_ROUTING_KEY_SPECIFIER_HH_

#include <queue>
#include <string>

namespace dripline
{

    class routing_key_specifier : public std::queue< std::string >
    {
        public:
            routing_key_specifier( const std::string& a_rk = "" );
            virtual ~routing_key_specifier();

            void parse( const std::string& a_rk );

        private:
            void add_next( const std::string& a_addr );

        public:
            static const char f_node_separator = '.';

    };

} /* namespace dripline */

#endif /* DRIPLINE_ROUTING_KEY_SPECIFIER_HH_ */
