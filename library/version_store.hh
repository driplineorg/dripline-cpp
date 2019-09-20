/*
 * version_store.hh
 *
 *  Created on: Sep 20, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_VERSIONSTORE_HH_
#define DRIPLINE_VERSIONSTORE_HH_

#include "singleton.hh"

#include "dripline_api.hh"

#include "member_variables.hh"
#include "scarab_version.hh"

#include <map>


namespace dripline
{

    class version_store : public scarab::singleton< version_store >
    {
        protected:
            friend class scarab::singleton< version_store >;
            friend class scarab::destroyer< version_store >;
            version_store();
            virtual ~version_store();

        public:
            template< typename x_version >
            void add_version( const std::string& a_name );
            void remove_version( const std::string& a_name );

            typedef std::map< std::string, scarab::version_semantic > version_map_t;
            mv_referrable_const( version_map_t, versions );
    };

    template< typename x_version >
    inline void version_store::add_version( const std::string& a_name )
    {
        f_versions.insert( std::make_pair( a_name, x_version() ) );
        return;
    }

    inline void version_store::remove_version( const std::string& a_name )
    {
        f_versions.erase( a_name );
        return;
    }

} /* namespace dripline */

#endif /*DRIPLINE_VERSIONSTORE_HH_ */
