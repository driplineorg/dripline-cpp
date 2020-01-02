/*
 * dripline_exceptions.hh
 *
 *  Created on: Aug 14, 2018
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_EXCEPTIONS_HH_
#define DRIPLINE_EXCEPTIONS_HH_

#include "return_codes.hh"

#include "member_variables.hh"
#include "param.hh"

#include <exception>
#include <memory>
#include <sstream>


namespace dripline
{
    /*!
     @class base_exception
     @author N.S. Oblath
     @brief Base class for dripline exceptions

     @details
     This class provides streaming operators for building up the `what()` message.
     It's meant to be the base class for all dripline-cpp exceptions.

     This class uses the Curiously Recurring Template Pattern (CRTP) to get the 
     derived class type to appear in the base class.
     In particular, we need to return x_derived& from the various operator<<() 
     so that those functions can be used in a throw statement and the user can 
     still catch the derived type.
    */
   template< typename x_derived >
    class DRIPLINE_API base_exception : public ::std::exception
    {
        public:
            base_exception() noexcept;
            base_exception( const base_exception< x_derived >& a_orig ) noexcept;
            virtual ~base_exception() noexcept;

            base_exception< x_derived >& operator=( const base_exception< x_derived >& a_orig ) noexcept;

            template< class x_streamable >
            x_derived& operator<<( x_streamable a_fragment ) noexcept;
            x_derived& operator<<( const std::string& a_fragment ) noexcept;
            x_derived& operator<<( const char* a_fragment ) noexcept;

            virtual const char* what() const noexcept;

        protected:
            mutable std::string f_error;
    };

    /*!
     @class dripline_error
     @author N.S. Oblath
     @brief Dripline-specific errors

     @details
     This is meant to be thrown for Dripline-specific errors.

     It should be used in the manner of any typical C++ exception, whenever the current
     function doesn't know how to handle a problem it encounters.
    */
    class DRIPLINE_API dripline_error : public base_exception< dripline_error >
    {
        public:
            dripline_error() noexcept;
            dripline_error( const dripline_error& ) noexcept;
            virtual ~dripline_error() noexcept;
    };

    /*!
     @class throw_reply
     @author N.S. Oblath
     @brief Object that can be thrown while processing a request to send a reply.

     @details
     The throw_reply is intended to be thrown during message processing. 
     It's caught in endpoint::on_request_message() to translate the information into a reply message.

     Three pieces of information can be transmitted:
        1. (required) The return code is provided by an object derived from return_code. 
          It's passed to the throw_reply in the constructor.
        2. (optional) The return message explains the reply in human-readable terms.  It's passed 
          to the throw_reply using operator<<().
        3. (optional) The payload can contain further information to reply to the requstor.  
           It's passed to the throw_reply in the constructor.  The default is a null (scarab::param) object. 
    */
    class DRIPLINE_API throw_reply : public base_exception< throw_reply >
    {
        public:
            throw_reply();
            throw_reply( const return_code& a_code, scarab::param_ptr_t&& a_payload_ptr = scarab::param_ptr_t(new scarab::param()) );
            throw_reply( const throw_reply& a_orig );
            virtual ~throw_reply() noexcept;

            throw_reply& operator=( const throw_reply& a_orig );

            virtual const char* what() const noexcept;

            const return_code& ret_code() const;
            void set_return_code( const return_code& a_code );

            const scarab::param& payload() const;
            scarab::param& payload();
            void set_payload( scarab::param_ptr_t&& a_payload );
            const scarab::param_ptr_t& get_payload_ptr() const;

#ifdef DL_PYTHON
            mv_referrable( std::string, py_throw_reply_keyword );
#endif

        protected:
            std::shared_ptr< return_code > f_return_code;
            scarab::param_ptr_t f_payload;
    };


    template< typename x_derived >
    base_exception< x_derived >::base_exception()  noexcept :
            ::std::exception(),
            f_error()
    {}

    template< typename x_derived >
    base_exception< x_derived >::base_exception( const base_exception< x_derived >& a_orig )  noexcept :
            std::exception( a_orig ),
            f_error( a_orig.f_error )
    {}

    template< typename x_derived >
    base_exception< x_derived >::~base_exception() noexcept
    {}

    template< typename x_derived >
    base_exception< x_derived >& base_exception< x_derived >::operator=( const base_exception< x_derived >& a_orig ) noexcept
    {
        f_error = a_orig.f_error;
        return *this;
    }

    template< typename x_derived >
    const char* base_exception< x_derived >::what() const noexcept
    {
        return f_error.c_str();
    }

    template< typename x_derived >
    template< class x_streamable >
    x_derived& base_exception< x_derived >::operator<<( x_streamable a_fragment ) noexcept
    {
        try
        {
            std::stringstream stream;
            stream << a_fragment;
            stream >> f_error;
        }
        catch(...)
        {}

        return *static_cast< x_derived* >(this);
    }

    template< typename x_derived >
    x_derived& base_exception< x_derived >::operator<<( const std::string& a_fragment ) noexcept
    {
        try
        {
            f_error += a_fragment;
        }
        catch(...)
        {}

        return *static_cast< x_derived* >(this);
    }

    template< typename x_derived >
    x_derived& base_exception< x_derived >::operator<<( const char* a_fragment ) noexcept
    {
        try
        {
            f_error += std::string( a_fragment );
        }
        catch(...)
        {}
        return *static_cast< x_derived* >(this);
    }

    inline const return_code& throw_reply::ret_code() const
    {
        return *f_return_code;
    }

    inline void throw_reply::set_return_code( const return_code& a_code )
    {
        f_return_code.reset( new copy_code( a_code ) );
        return;
    }

    inline const scarab::param& throw_reply::payload() const
    {
        return *f_payload;
    }

    inline scarab::param& throw_reply::payload()
    {
        return *f_payload;
    }

    inline void throw_reply::set_payload( scarab::param_ptr_t&& a_payload )
    {
        f_payload = std::move( a_payload );
        return;
    }

    inline const scarab::param_ptr_t& throw_reply::get_payload_ptr() const
    {
        return f_payload;
    }

}

#endif /* DRIPLINE_EXCEPTIONS_HH_ */
