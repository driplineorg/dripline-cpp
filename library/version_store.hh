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

    /*!
     @class version_store
     @author N.S. Oblath

     @brief Singleton class to store all version information relevant in any particular context.

     @details
     This class is used to provide all of the relevant version information to a dripline message object.
     A library/executable will add the version information to this singleton object, and when 
     a message is created, it automatically access that information.
    */
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


    /*!
     @struct version_store_adder
     @author N.S. Oblath

     @brief Version adder struct to enable adding version classes at static initialization.  See ADD_VERSION.

     @note This method for adding a version object requires a derived version class that has been 
     registered with the factory already.  It's intended to be used at static initialization time, 
     but could be used during runtime as well.
    */
    template< typename x_version >
    struct DRIPLINE_API version_store_adder
    {
        version_store_adder( const std::string& a_name )
        {
            version_store::get_instance()->add_version< x_version >( a_name );
        }
    };

    /*!
     @fn add_version
     @author N.S. Oblath

     @brief Templated function for adding a version object.

     @note This method for adding a version object requires a derived version class that has been 
     registered with the factory already.  This could be used at static initialization, though 
     it's mainly intended for use during runtime.  The adder object is created on the heap using a 
     shared_ptr, so memory management should be simple.
    */
    template< typename x_version >
    DRIPLINE_API std::shared_ptr< version_store_adder< x_version > > add_version( const std::string& a_name )
    {
        return std::make_shared< version_store_adder<x_version> >( a_name );
    }

    // function to use to add a version via a base-class pointer to a derived object
    /*!
     @fn add_version
     @author N.S. Oblath

     @brief Function for adding a version object using polymorphism.

     @note This method for adding a version object requires an instance of a version_semantic class.  
     This is intended for use at runtime, and in particular for the Python binding.  The instance could 
     be a base-class instance or a generic derived object containing unique version information.
    */
    DRIPLINE_API void add_version( const std::string& a_name, scarab::version_semantic_ptr_t a_version_ptr );

    /*!
     @def ADD_VERSION( version_name, version_type )
     @author N.S. Oblath

     @brief Macro for adding version classes at static initialization.
    */
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
