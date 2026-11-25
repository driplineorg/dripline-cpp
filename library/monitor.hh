/*
 * monitor.hh
 *
 *  Created on: Jul 1, 2019
 *      Author: N.S. Oblath
 */

#ifndef DRIPLINE_MONITOR_HH_
#define DRIPLINE_MONITOR_HH_

#include "core.hh"
#include "message_dispatcher.hh"

namespace scarab
{
    class authentication;
}

namespace dripline
{

    /*!
     @class monitor
     @author N.S. Oblath

     @brief Listens for messages sent to a particular set of keys and prints them

     @details
     The monitor is initially configured with a set of keys to listen for.  
     Keys can be specified for the alerts exchange and for the requests exchange.

     Keys can be passed to the monitor in the configuration scarab::param_node object.  
     Alerts keys can be supplied as `alerts-keys` followed by an array of keys or 
     `alerts-key` followed by a single key.  Requests keys can be supplied as `requests-keys` 
     followed by an array of keys or `requests-key` followed by a single key.

     All AMQP-standard notation for keys, including wildcards, are allowed.

     When activated, the alerts keys are bound to the alerts exchange, and the 
     requests keys are bound to the requests exchange.  The monitor then waits to receive 
     a message.  When a message is seen, it prints it to stdout.
    */
    class DRIPLINE_API monitor :
            public core,
            public message_dispatcher
    {
        protected:
            enum class status
            {
                nothing = 0,
                channel_created = 10,  ///< broker connection established
                consuming = 50,        ///< queue declared, keys bound, consumer started
                listening = 60         ///< actively waiting for messages
            };

        public:
            monitor( const scarab::param_node& a_config, const scarab::authentication& a_auth );
            monitor( const monitor& ) = delete;
            monitor( monitor&& a_orig ) = default;
            virtual ~monitor();

            monitor& operator=( const monitor& ) = delete;
            monitor& operator=( monitor&& a_orig ) = default;

            mv_accessible( status, status );

            /// Name for this monitor; automiatically set to `monitor_[uuid]`
            mv_referrable( std::string, name );

            /// Flag to indicate whether syntactically-correct JSON should be printed, 
            /// or whether the default style (similar to but not exactly JSON) should be used.
            mv_accessible( bool, json_print );
            /// Flag to indicate whether JSON should be printed with extra whitespace for human readability.
            mv_accessible( bool, pretty_print );

            typedef std::vector< std::string > keys_t;
            /// Set of request keys to be listened for.
            mv_referrable( keys_t, requests_keys );
            /// Set of alerts keys to be listened for.
            mv_referrable( keys_t, alerts_keys );

        public:
            /// Opens the AMQP connection, binds keys, and starts consuming.
            bool start();

            /// Starts actively listening for and handling messages (blocking).
            bool listen();

            /// Stops listening for messages and closes the AMQP connection.
            bool stop();

        public:
            /// Handles a single Dripline message by printing it to stdout.
            /// Printing is done via a prog-level message in the logger.
            virtual void submit_message( message_ptr_t a_message );
    };

} /* namespace dripline */

#endif /* DRIPLINE_MONITOR_HH_ */
