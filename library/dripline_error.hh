#ifndef DRIPLINE_ERROR_HH_
#define DRIPLINE_ERROR_HH_

#include "return_codes.hh"

#include "param.hh"

#include <exception>
#include <memory>
#include <sstream>


namespace dripline
{

    class DRIPLINE_API dripline_error : public ::std::exception
    {
        public:
            dripline_error();
            dripline_error( const dripline_error& );
            virtual ~dripline_error() throw ();

            template< class x_streamable >
            dripline_error& operator<<( x_streamable a_fragment );
            dripline_error& operator<<( const std::string& a_fragment );
            dripline_error& operator<<( const char* a_fragment );

            virtual const char* what() const throw();

        protected:
            mutable std::string f_error;
    };

    class DRIPLINE_API throw_reply : public dripline_error
    {
        public:
            throw_reply();
            throw_reply( const return_code& a_code );
            throw_reply( const throw_reply& );
            virtual ~throw_reply() throw();

            virtual const char* what() const throw();

            const return_code& ret_code() const;
            void set_return_code( const return_code& a_code );

            const scarab::param& payload() const;
            scarab::param& payload();
            void set_payload( scarab::param_ptr_t&& a_payload );

        protected:
            std::shared_ptr< return_code > f_retcode;
            scarab::param_ptr_t f_payload;
    };


    template< class x_streamable >
    dripline_error& dripline_error::operator<<( x_streamable a_fragment )
    {
        std::stringstream stream;
        stream << a_fragment;
        stream >> f_error;
        return *this;
    }

    inline dripline_error& dripline_error::operator<<( const std::string& a_fragment )
    {
        f_error += a_fragment;
        return *this;
    }

    inline dripline_error& dripline_error::operator<<( const char* a_fragment )
    {
        f_error += std::string( a_fragment );
        return *this;
    }

    inline const return_code& throw_reply::ret_code() const
    {
        return *f_retcode;
    }

    inline void throw_reply::set_return_code( const return_code& a_code )
    {
        f_retcode.reset( new copy_code( a_code ) );
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

}

#endif /* DRIPLINE_ERROR_HH_ */
