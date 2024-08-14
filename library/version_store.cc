/*
 * version_store.cc
 *
 *  Created on: Sep 20, 2019
 *      Author: N.S. Oblath
 */

#define DRIPLINE_API_EXPORTS

#include "version_store.hh"


namespace dripline
{

    version_store::version_store() :
            f_versions()
    {}

    version_store::~version_store()
    {}

    DRIPLINE_API void add_version( const std::string& a_name, scarab::version_semantic_ptr_t a_version_ptr )
    {
        version_store::get_instance()->add_version( a_name, a_version_ptr );
    }

    DRIPLINE_API scarab::version_semantic_ptr_t get_version( const std::string& a_name )
    {
        return version_store::get_instance()->versions().at( a_name );
    }


} /* namespace dripline */
