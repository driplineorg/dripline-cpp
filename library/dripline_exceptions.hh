/*
 * dripline_exceptions.hh
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_EXCEPTIONS_HH_
#define DRIPLINE_EXCEPTIONS_HH_

#include "dripline_api.hh"

#include "base_exception.hh"


namespace dripline
{

    /*!
     @class dripline_error
     @author N.S. Oblath
     @brief Dripline-specific errors

     @details
     This is meant to be thrown for Dripline-specific errors.

     It should be used in the manner of any typical C++ exception, whenever the current
     function doesn't know how to handle a problem it encounters.
    */
    class DRIPLINE_API dripline_error : public scarab::typed_exception< dripline_error >
    {
        public:
            dripline_error() = default;
            dripline_error( const dripline_error& ) = default;
            dripline_error( dripline_error&& ) = default;
            virtual ~dripline_error() noexcept = default;

            dripline_error& operator=( const dripline_error& ) = default;
            dripline_error& operator=( dripline_error&& ) = default;
    };

    /*!
     @class connection_error
     @author N.S. Oblath
     @brief Error indicating a problem with the connection to the broker
    */
    class DRIPLINE_API connection_error : public scarab::typed_exception< connection_error >
    {
        public:
            connection_error() = default;
            connection_error( const connection_error& ) = default;
            connection_error( connection_error&& ) = default;
            virtual ~connection_error() noexcept = default;

            connection_error& operator=( const connection_error& ) = default;
            connection_error& operator=( connection_error&& ) = default;
    };

}

#endif /* DRIPLINE_EXCEPTIONS_HH_ */
