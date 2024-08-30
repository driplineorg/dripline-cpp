/*
 * dripline_config.hh
 *
 *  Created on: June 26, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_DRIPLINE_CONFIG_HH_
#define DRIPLINE_DRIPLINE_CONFIG_HH_

#include "dripline_api.hh"

#include "param.hh"

namespace scarab
{
    class main_app;
}

namespace dripline
{

    /*!
     @class dripline_config
     @author N.S. Oblath

     @brief Sets the default configuration used by `core`.  These parameters pertain to the dripline mesh that will be used.
    */
    class DRIPLINE_API dripline_config : public scarab::param_node
    {
        public:
            dripline_config();
            dripline_config( const dripline_config& ) = default;
            dripline_config( dripline_config&& ) = default;
            virtual ~dripline_config() = default;

            dripline_config& operator=( const dripline_config& ) = default;
            dripline_config& operator=( dripline_config&& ) = default;
    };

    /// Add dripline mesh CL options to an app object
    void DRIPLINE_API add_dripline_options( scarab::main_app& an_app );

    /// Create a param_node with the default dripline authentication specification
    scarab::param_node DRIPLINE_API create_dripline_auth_spec();

    /// Add default authentication specification
    /// This can either be done with an authentication specification group or with an auth file, as determined by the a_use_auth_file flag.
    /// The use of an auth file is being maintained for backwards compatibility, but is not preferred.
    /// For these defaults, it will either specify an auth file or an auth-spec group, but not both.
    /// If an auth-spec group is the default, but the user provides an auth file, the latter will override the former.
    void DRIPLINE_API add_dripline_auth_spec( scarab::main_app& an_app, bool a_use_auth_file=false );

} /* namespace dripline */
#endif /* DRIPLINE_DRIPLINE_CONFIG_HH_ */
