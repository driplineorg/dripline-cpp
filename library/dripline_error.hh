#ifndef DRIPLINE_ERROR_HH_
#define DRIPLINE_ERROR_HH_

#include <sstream>
#include <exception>

#include "dripline_api.hh"

namespace dripline
{

    class DRIPLINE_API dripline_error : public ::std::exception
    {
        public:
            dripline_error();
            dripline_error( const dripline_error& );
            ~dripline_error() throw ();

            template< class x_streamable >
            dripline_error& operator<<( x_streamable a_fragment );
            dripline_error& operator<<( const std::string& a_fragment );
            dripline_error& operator<<( const char* a_fragment );

            virtual const char* what() const throw();

        private:
            std::string f_error;
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

}

#endif /* DRIPLINE_ERROR_HH_ */
