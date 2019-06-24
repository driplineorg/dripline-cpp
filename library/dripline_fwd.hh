/*
 * dripline_fwd.hh
 *
 *  Created on: Jun 24, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_DRIPLINE_FWD_HH_
#define DRIPLINE_DRIPLINE_FWD_HH_

#include <memory>

namespace dripline
{
    class message;
    class msg_request;
    class msg_reply;
    class msg_alert;

    typedef std::shared_ptr< message > message_ptr_t;
    typedef std::shared_ptr< msg_request > request_ptr_t;
    typedef std::shared_ptr< msg_reply > reply_ptr_t;
    typedef std::shared_ptr< msg_alert > alert_ptr_t;

    class listener;
    typedef std::shared_ptr< listener > listener_ptr_t;

    class endpoint;
    typedef std::shared_ptr< endpoint > endpoint_ptr_t;

    class service;
    typedef std::shared_ptr< service > service_ptr_t;
}

#endif /* DRIPLINE_DRIPLINE_FWD_HH_ */
