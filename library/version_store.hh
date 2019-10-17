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
#include "version_semantic.hh"

#include <map>
#include <memory>


namespace dripline
{

    class DRIPLINE_API version_store : public scarab::singleton< version_store >
    {
        protected:
            friend class scarab::singleton< version_store >;
            friend class scarab::destroyer< version_store >;
            version_store();
            virtual ~version_store();

        public:
            template< typename x_version >
            void add_version( const std::string& a_name );
            void add_version( const std::string& a_name, scarab::version_semantic_ptr_t a_version_ptr );
            void remove_version( const std::string& a_name );

            typedef std::map< std::string, scarab::version_semantic_ptr_t > version_map_t;
            mv_referrable_const( version_map_t, versions );
    };


    // version adder object to enable adding version classes at static initialization
    template< typename x_version >
    struct DRIPLINE_API version_store_adder
    {
        version_store_adder( const std::string& a_name )
        {
            version_store::get_instance()->add_version< x_version >( a_name );
        }
    };

    // function to use to add a version (whenever; adder is created on the heap)
    template< typename x_version >
    DRIPLINE_API std::shared_ptr< version_store_adder< x_version > > add_version( const std::string& a_name )
    {
        return std::make_shared< version_store_adder<x_version> >( a_name );
    }

    // function to use to add a version via a base-class pointer to a derived object
    DRIPLINE_API void add_version( const std::string& a_name, scarab::version_semantic_ptr_t a_version_ptr );

    // macro to use to add a version (usually at static initialization)
    #define ADD_VERSION( version_name, version_type ) \
        static ::dripline::version_store_adder< version_type > s_version_adder_##version_name( TOSTRING(version_name) );


    template< typename x_version >
    inline void version_store::add_version( const std::string& a_name )
    {
        f_versions.insert( std::make_pair( a_name, scarab::version_semantic_ptr_t( new x_version() ) ) );
        return;
    }

    inline void version_store::add_version( const std::string& a_name, scarab::version_semantic_ptr_t a_version_ptr )
    {
        f_versions.insert( std::make_pair( a_name, a_version_ptr ) );
        return;
    }

    inline void version_store::remove_version( const std::string& a_name )
    {
        f_versions.erase( a_name );
        return;
    }

} /* namespace dripline */

#endif /*DRIPLINE_VERSIONSTORE_HH_ */
