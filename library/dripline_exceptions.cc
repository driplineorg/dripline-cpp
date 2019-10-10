
#define DRIPLINE_API_EXPORTS

#include "dripline_exceptions.hh"

namespace dripline
{

    dripline_error::dripline_error() :
            base_exception< dripline_error >()
    {}

    dripline_error::dripline_error( const dripline_error& an_error ) :
            base_exception< dripline_error >( an_error )
    {}

    dripline_error::~dripline_error() throw ()
    {}


    throw_reply::throw_reply() :
            base_exception< throw_reply >(),
            f_retcode( new dl_unhandled_exception() ),
            f_payload( new scarab::param() )
    {}

    throw_reply::throw_reply( const return_code& a_code, scarab::param_ptr_t&& a_payload_ptr ) :
            base_exception< throw_reply >(),
            f_retcode( new copy_code( a_code ) ),
            f_payload( std::move(a_payload_ptr) )
    {}

    throw_reply::throw_reply( const throw_reply& a_throw ) :
            base_exception< throw_reply >( a_throw ),
            f_retcode( a_throw.f_retcode ),
            f_payload( a_throw.f_payload->clone() )
    {}

    throw_reply::~throw_reply() throw ()
    {}

    const char* throw_reply::what() const throw ()
    {
        std::stringstream t_stream;
        t_stream << f_error;
        f_error = t_stream.str();
        return base_exception< throw_reply >::what();
    }
}
