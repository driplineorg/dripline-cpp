/*
 * throw_reply.hh
 *
 *  Created on: Jan 2, 2020
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_THROW_REPLY_HH_
#define DRIPLINE_THROW_REPLY_HH_

#include "dripline_exceptions.hh"
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
    class DRIPLINE_API throw_reply : public base_exception< throw_reply >
    {
        public:
            throw_reply();
            throw_reply( const return_code& a_code, scarab::param_ptr_t a_payload_ptr = scarab::param_ptr_t(new scarab::param()) );
            throw_reply( const throw_reply& a_orig );
            virtual ~throw_reply() noexcept;

            throw_reply& operator=( const throw_reply& a_orig );

            virtual const char* what() const noexcept;

            const return_code& ret_code() const;
            void set_return_code( const return_code& a_code );

            const scarab::param& payload() const;
            scarab::param& payload();
            void set_payload( scarab::param_ptr_t a_payload );
            const scarab::param_ptr_t& get_payload_ptr() const;

#ifdef DL_PYTHON
            mv_referrable( std::string, py_throw_reply_keyword );
#endif

        protected:
            std::shared_ptr< return_code > f_return_code;
            scarab::param_ptr_t f_payload;
    };


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

    inline void throw_reply::set_payload( scarab::param_ptr_t a_payload )
    {
        f_payload = std::move( a_payload );
        return;
    }

    inline const scarab::param_ptr_t& throw_reply::get_payload_ptr() const
    {
        return f_payload;
    }

}

#endif /* DRIPLINE_THROW_REPLY_HH_ */
