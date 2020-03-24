/*
 * mock_broker.hh
 *
 *  Created on: Mar 24, 2020
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_MOCK_BROKER_HH_
#define DRIPLINE_MOCK_BROKER_HH_

#include "endpoint.hh"

namespace dripline
{

    /*!
     @class mock_broker
     @author N.S. Oblath

     @brief 

     @details
    */
    class DRIPLINE_API mock_broker
    {
        enum class exchange
        {
            requests,
            alerts
        };

        mock_broker();
        mock_broker( const mock_broker& a_orig );
        mock_broker( mock_broker&& a_orig );
        virtual ~mock_broker();

        mock_broker& operator=( const mock_broker& a_orig );
        mock_broker& operator=( mock_broker&& a_orig );

        void take( message_ptr_t a_message, exchange an_exchange );

        void bind( exchange an_exchange, const std::string& a_key );
        void unbind( exchange an_exchange, const std::string& a_key );

    }

} /* namespace dripline */

#endif /*  DRIPLINE_MOCK_BROKER_HH_ */