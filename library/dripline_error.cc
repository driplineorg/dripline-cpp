
#define DRIPLINE_API_EXPORTS

#include "dripline_error.hh"

namespace dripline
{

    dripline_error::dripline_error() :
            ::std::exception(),
            f_error()
    {
    }

    dripline_error::dripline_error( const dripline_error& an_error ) :
            std::exception(),
            f_error( an_error.f_error )
    {
    }

    dripline_error::~dripline_error() throw ()
    {
    }

    const char* dripline_error::what() const throw ()
    {
        return f_error.c_str();
    }

}