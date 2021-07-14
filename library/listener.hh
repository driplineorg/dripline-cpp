/*
 * listener.hh
 *
 *  Created on: Jun 23, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_LISTENER_HH_
#define DRIPLINE_LISTENER_HH_

#include "dripline_fwd.hh"

#include "receiver.hh"

#include "cancelable.hh"
#include "member_variables.hh"

#include <thread>

namespace dripline
{
    /*!
     @class listener
     @author N.S. Oblath

     @brief A listener is a class capable of listening for AMQP messages on an AMQP channel.  This class provides the 
     basic framework for doing that.

     @details
     The listener class is a mix-in class that provides the interface for listening for messages: 
     the pure virtual function `listen_on_queue()`.
     This function should be run in the thread provided by this class.
     However, it needs to be implemented by the inheriting class.

     The typical use case involves at least two threads:
     1. A listener gets messages from the AMQP channel (using `listen_on_queue(), e.g. @ref service or @ref endpoint_listener_receiver) and 
        calls `receiver::handle_message_chunk()`
     2. A receiver has a timing thread waiting for multiple message chunks (if relevant); 
        when the message is complete, `receiver::process_message()` is called.
    
     This class also provides the objects and information needed for listening on a queue:
     * The AMQP channel
     * A consumer tag
     * A timeout (in ms)
     * The thread object for listening
    */
    class DRIPLINE_API listener : public virtual scarab::cancelable
    {
        public:
            listener();
            listener( const listener& ) = delete;
            listener( listener&& a_orig );
            virtual ~listener();

            listener& operator=( const listener& ) = delete;
            listener& operator=( listener&& a_orig );

            /// Returns false if the return is due to an error in this function; returns true otherwise (namely because it was canceled)
            virtual bool listen_on_queue() = 0;

            mv_referrable( amqp_channel_ptr, channel );

            mv_referrable( std::string, consumer_tag );

            mv_accessible( unsigned, listen_timeout_ms );

            mv_referrable( std::thread, listener_thread );
    };

    /*!
     @class listener_receiver
     @author N.S. Oblath

     @brief Convenience class to bring together @ref listener and @ref concurrent_receiver
    */
    class DRIPLINE_API listener_receiver : public listener, public concurrent_receiver
    {
        public:
            listener_receiver() : scarab::cancelable(), listener(), concurrent_receiver() {}
            listener_receiver( const listener_receiver& ) = delete;
            listener_receiver( listener_receiver&& a_orig ) :
                scarab::cancelable( std::move(a_orig) ),
                listener( std::move(a_orig) ),
                concurrent_receiver( std::move(a_orig) )
            {}

            listener_receiver& operator=( const listener_receiver& ) = delete;
            listener_receiver& operator=( listener_receiver&& a_orig )
            {
                listener::operator=( std::move(a_orig) );
                concurrent_receiver::operator=( std::move(a_orig) );
                return *this;
            }
    };

    /*!
     @class endpoint_listener_receiver
     @author N.S. Oblath

     @brief Decorator class for a plain endpoint: adds listener_receiver capabilities.

     @details
     The endpoint_listener_receiver is used by @ref service to wrap an endpoint that is to listen for messages asynchronously.
    */
    class DRIPLINE_API endpoint_listener_receiver : public listener_receiver
    {
        public:
            endpoint_listener_receiver( endpoint_ptr_t a_endpoint_ptr );
            endpoint_listener_receiver( const endpoint_listener_receiver& ) = delete;
            endpoint_listener_receiver( endpoint_listener_receiver&& a_orig );
            virtual ~endpoint_listener_receiver();

            endpoint_listener_receiver& operator=( const endpoint_listener_receiver& ) = delete;
            endpoint_listener_receiver& operator=( endpoint_listener_receiver&& a_orig );

            /// Listens for AMQP messages and then passes them to be handled as Dripline message chunks
            /// Returns false if the return is due to an error in this function; returns true otherwise (namely because it was canceled)
            virtual bool listen_on_queue();

        protected:
            /// Direct submission of messages to the endpoint
            virtual void submit_message( message_ptr_t a_message );

            /// Pointer to the decorated endpoint
            mv_referrable( endpoint_ptr_t,  endpoint );
    };



} /* namespace dripline */

#endif /* DRIPLINE_LISTENER_HH_ */
