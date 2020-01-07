/*
 * throw_reply.hh
 *
 *  Created on: Jan 2, 2020
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_THROW_REPLY_HH_
#define DRIPLINE_THROW_REPLY_HH_

#include "return_codes.hh"

#include "member_variables.hh"
#include "param.hh"

#include <exception>
#include <memory>
#include <sstream>


namespace dripline
{

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
    class DRIPLINE_API throw_reply
    {
        public:
            throw_reply();
            throw_reply( const return_code& a_code, scarab::param_ptr_t a_payload_ptr = scarab::param_ptr_t(new scarab::param()) );
            throw_reply( const throw_reply& a_orig );
            virtual ~throw_reply() noexcept;

            throw_reply& operator=( const throw_reply& a_orig );

            template< class x_streamable >
            throw_reply& operator<<( x_streamable a_fragment );
            throw_reply& operator<<( const std::string& a_fragment );
            throw_reply& operator<<( const char* a_fragment );

            const std::string& return_message() const noexcept;
            std::string& return_message();

            const return_code& ret_code() const noexcept;
            void set_return_code( const return_code& a_code );

            const scarab::param& payload() const noexcept;
            scarab::param& payload();
            void set_payload( scarab::param_ptr_t a_payload );
            const scarab::param_ptr_t& get_payload_ptr() const noexcept;

#ifdef DL_PYTHON
            mv_referrable_static( std::string, py_throw_reply_keyword );
#endif

        protected:
            std::string f_return_message;
            std::shared_ptr< return_code > f_return_code;
            scarab::param_ptr_t f_payload;
    };


    template< class x_streamable >
    throw_reply& throw_reply::operator<<( x_streamable a_fragment )
    {
        std::stringstream stream;
        stream << a_fragment;
        stream >> f_return_message;
        return *this;
    }

    inline throw_reply& throw_reply::operator<<( const std::string& a_fragment ) 
    {
        f_return_message += a_fragment;
        return *this;
    }

    inline throw_reply& throw_reply::operator<<( const char* a_fragment ) 
    {
        f_return_message += std::string( a_fragment );
        return *this;
    }

    inline const std::string& throw_reply::return_message() const noexcept
    {
        return f_return_message;
    }

    inline std::string& throw_reply::return_message()
    {
        return f_return_message;
    }

    inline const return_code& throw_reply::ret_code() const noexcept
    {
        return *f_return_code;
    }

    inline void throw_reply::set_return_code( const return_code& a_code )
    {
        f_return_code.reset( new copy_code( a_code ) );
        return;
    }

    inline const scarab::param& throw_reply::payload() const noexcept
    {
        return *f_payload;
    }

    inline scarab::param& throw_reply::payload()
    {
        return *f_payload;
    }

    inline void throw_reply::set_payload( scarab::param_ptr_t a_payload )
    {
        f_payload = std::move( a_payload );
        return;
    }

    inline const scarab::param_ptr_t& throw_reply::get_payload_ptr() const noexcept
    {
        return f_payload;
    }

}

#endif /* DRIPLINE_THROW_REPLY_HH_ */
