
#define DRIPLINE_API_EXPORTS

#include "dripline_error.hh"

namespace dripline
{

    dripline_error::dripline_error() :
            ::std::exception(),
            f_error()
    {}

    dripline_error::dripline_error( const dripline_error& an_error ) :
            std::exception(),
            f_error( an_error.f_error )
    {}

    dripline_error::~dripline_error() throw ()
    {}

    const char* dripline_error::what() const throw ()
    {
        return f_error.c_str();
    }

    throw_reply::throw_reply() :
            dripline_error(),
            f_retcode( new dl_unhandled_exception() )
    {}

    throw_reply::throw_reply( const return_code& a_code ) :
            dripline_error(),
            f_retcode( new copy_code( a_code ) )
    {}

    throw_reply::throw_reply( const throw_reply& a_throw ) :
            dripline_error( a_throw ),
            f_retcode( a_throw.f_retcode )
    {}

    throw_reply::~throw_reply() throw ()
    {}

    const char* throw_reply::what() const throw ()
    {
        std::stringstream t_stream;
        t_stream << "Return code: " << f_retcode->rc_name() << "(" << f_retcode->rc_value() << ") -- " << f_error;
        f_error = t_stream.str();
        return dripline_error::what();
    }
}
