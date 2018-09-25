/*
 * specifier.hh
 *
 *  Created on: Feb 18, 2016
 *      Author: nsoblath
 */

#ifndef DRIPLINE_SPECIFIER_HH_
#define DRIPLINE_SPECIFIER_HH_

#include "dripline_api.hh"

#include "member_variables.hh"

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

    class DRIPLINE_API specifier : public std::deque< std::string >
    {
        public:
            typedef std::deque< std::string > container_type;

        public:
            specifier( const std::string& a_unparsed = "" );
            specifier( const specifier& a_orig );
            specifier( specifier&& a_orig );
            virtual ~specifier();

            const specifier& operator=( const specifier& a_orig );
            const specifier& operator=( specifier&& a_orig );

            void parse( const std::string& a_unparsed );
            void reparse();
            std::string to_string() const;

            mv_referrable( std::string, unparsed );

        private:
            void add_next( const std::string& a_addr );

        public:
            static const char f_node_separator = '.';

    };

    inline void specifier::reparse()
    {
        parse( f_unparsed );
        return;
    }


} /* namespace dripline */

#endif /* DRIPLINE_SPECIFIER_HH_ */
