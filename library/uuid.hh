/*
 * uuid.hh
 *
 *  Created on: Sep 16, 2015
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_UUID_HH_
#define DRIPLINE_UUID_HH_

#include "dripline_api.hh"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp> // to allow streaming of uuid_t

#include <string>

// In Windows there's a preprocessor macro called uuid_t that conflicts with this typdef
#ifdef uuid_t
#undef uuid_t
#endif

namespace dripline
{
    /// Universally-unique-identifier type containing 16 hexadecimal characters
    typedef boost::uuids::uuid uuid_t;

    /// Generates a UUID containing random numbers (RNG is a Mersenne Twister)
    uuid_t DRIPLINE_API generate_random_uuid();
    /// Generates a UUID containing all 0s
    uuid_t DRIPLINE_API generate_nil_uuid();

    /*!
     Converts a string to a UUID object.
     Throws a std::runtime_error if the string is an invalid UUID.
     @return the uuid_t object representing the given string
     @param a_id_str The input UUID represented as a string.  
         Valid formats are `hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh`, 
         where each `h` is a case-insensitive hexidecimal character, 
         and `hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh`.
    */
    uuid_t DRIPLINE_API uuid_from_string( const std::string& a_id_str );
    uuid_t DRIPLINE_API uuid_from_string( const char* a_id_str );

    /*!
     Converts a string to a UUID object.
     @return the uuid_t object representing the given string, or a nil UUID if the string is not a valid UUID
     @param[in] a_id_str The input UUID represented as a string.  
         Valid formats are `hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh`, 
         where each `h` is a case-insensitive hexidecimal character, 
         and `hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh`.
     @param[out] a_valid_flag Returns whether the provided string was a valid UUID.
    */
    uuid_t DRIPLINE_API uuid_from_string( const std::string& a_id_str, bool& a_valid_flag );
    uuid_t DRIPLINE_API uuid_from_string( const char* a_id_str, bool& a_valid_flag );

    /// Generates a string representation of the provided UUID.
    std::string DRIPLINE_API string_from_uuid( const uuid_t& a_id );

} /* namespace dripline */

#endif /* DRIPLINE_UUID_HH_ */
