//
// In this file, we define the classes
//
//    vrpn_BaseConnection
//    vrpn_ConnectionController
//
// as well as various helper stuff.  For convenience, comments
// in this file will often drop the vrpn_ prefix.
//
// servers and client devices (like vrpn_TrackerRemote) interract
// with ConnectionController.  ConnectionController HAS-A list
// of BaseConnections.
// 
// Each BaseConnection is a communication
// link with a single client.  BaseConnection is an abstract
// base class.  One example of a concrete class that implements
// this interface is NetworkConnection.
//
// ConnectionController used to be called Connection.
// NetworkConnection used to be called OneConnection.
// BaseConnection is new.
//
// As a transition measure, we plan to provide either a typedef
// or an adapter called vrpn_Connection that has the same
// interface as the old vrpn_Connection that ConnectionController
// replaces.
//

#ifndef	VRPN_CONNECTIONCONTROLLER_H
#define VRPN_CONNECTIONCONTROLLER_H

#include <stdio.h>  // for FILE

#ifndef VRPN_SHARED_H
#include "vrpn_Shared.h"
#endif


#include "vrpn_ConnectionOldCommonStuff.h"
#include "vrpn_BaseConnection.h"

#include "hp_pxfl_list.h"

////////////////////////////////////////////////////////
//
// class vrpn_ConnectionController
//
// this class used to be called vrpn_Connection
// it HAS-A list of BaseConnection objects that it
// communicates through
//

class vrpn_ConnectionController
{
public:  // c'tors and d'tors
    // No public constructors because ...
    // Users should not create vrpn_Connection directly -- use 
    // vrpn_Synchronized_Connection (for servers) or 
    // vrpn_get_connection_by_name (for clients)

    //XXX Destructor should delete all entries from callback lists
    virtual ~vrpn_ConnectionController();

protected:  // c'tors

    // constructors ...XXX...
    vrpn_ConnectionController();

    // helper function
    // XXX if this doesn't go away, give a more descriptive comment
    virtual int connect_to_client( const char *machine, int port );

public:  // status

    // are there any connections?
    int at_least_one_open_connection() const;

    // some way to get a list of open connection names
    // (one need is for the hiball control panel)

public:  // buffer sizes.  XXX don't know if these belong here or not

    // Applications that need to send very large messages may want to
    // know the buffer size used or to change its size.  Buffer size
    // is returned in bytes.
    vrpn_int32 tcp_outbuf_size () const { /*return d_tcp_buflen;*/ }
    vrpn_int32 udp_outbuf_size () const { /*return d_udp_buflen;*/ }

    // Allows the application to attempt to reallocate the buffer.
    // If allocation fails (error or out-of-memory), -1 will be returned.
    // On successful allocation or illegal input (negative size), the
    // size in bytes will be returned.  These routines are NOT guaranteed
    // to shrink the buffer, only to increase it or leave it the same.
    vrpn_int32 set_tcp_outbuf_size (vrpn_int32 bytecount);


public:  // handling incomming and outgoing messages

    // * call each time through the program main loop
    // * handles receiving any incoming messages
    //   and sending any packed messages
    // * returns -1 when connection dropped due to error, 0 otherwise
    //   (only returns -1 once per connection drop)
    // * optional argument is TOTAL time to block on select() calls;
    //   there may be multiple calls to select() per call to mainloop(),
    //   and this timeout will be divided evenly between them.
    virtual int mainloop( const timeval * timeout = NULL );



	// * send pending report (that have been packed), and clear the buffer
	// * this function was protected, now is public, so we can use
    //   it to send out intermediate results without calling mainloop
	virtual int send_pending_reports();

    // * pack a message that will be sent the next time mainloop() is called
	// * turn off the RELIABLE flag if you want low-latency (UDP) send
    // * was: pack_message
	virtual int handle_outgoing_message(
        vrpn_uint32 len, 
        timeval time,
		vrpn_int32 type,
        vrpn_int32 sender,
        const char * buffer,
		vrpn_uint32 class_of_service );


protected:  // handling incomming and outgoing messages

    virtual int pack_sender_description( vrpn_int32 which );
    virtual int pack_type_description( vrpn_int32 which );
    virtual int pack_udp_description( int portno );
    //virtual int pack_log_description( long mode, const char * filename );

	// Routines to handle incoming messages on file descriptors
    int	handle_incomming_udp_messages (int fd, const timeval * timeout);
	int	handle_incomming_tcp_messages (int fd, const timeval * timeout);

	// Routines that handle system messages
    // these are registered as callbacks
    static int handle_incomming_sender_message(
        void * userdata, vrpn_HANDLERPARAM p);
	static int handle_incomming_type_message(
        void * userdata, vrpn_HANDLERPARAM p);
	static int handle_incomming_udp_message(
        void * userdata, vrpn_HANDLERPARAM p);
	static int handle_incomming_log_message(
        void * userdata, vrpn_HANDLERPARAM p);

public:  // callbacks
    // * clients and servers register callbacks.  The callbacks are how they
    //   are informed about messages passing between the connections.
    // * when registering a callback, you indicte which message types
    //   and which senders are pertinant to that callback.  Only messages
    //   with matching type/sender ID's will cause the callback to be invoked
    // * vrpn_ANY_SENDER and vrpn_ANY_TYPE exist as a way to say you want
    //   a callback to be invoked regardless of the type/sender id


    // * set up a handler for messages of a given
    //   type_id and sender_id
    // * may use vrpn_ANY_TYPE or vrpn_ANY_SENDER or both
    // * handlers will be called during mainloop()
    // * your handler should return 0, or a communication error
    //   is assumed and the connection will be shut down
    virtual int register_handler(
        vrpn_int32 type,
        vrpn_MESSAGEHANDLER handler,
        void *userdata,
        vrpn_int32 sender = vrpn_ANY_SENDER );

    // * remove a handler for messages of a given
    //   type_id and sender_id
    // * in order for this function to succeed, a handler
    //   must already have been registered with identical
    //   values of these parameters
    virtual	int unregister_handler(
        vrpn_int32 type,
	    vrpn_MESSAGEHANDLER handler,
        void *userdata,
	    vrpn_int32 sender = vrpn_ANY_SENDER );

    // * invoke callbacks that have registered
    //   for messages with this (type and sender)
    virtual	int	do_callbacks_for(
        vrpn_int32 type, vrpn_int32 sender,
		timeval time,
        vrpn_uint32 len, const char * buffer);


public:  // senders and types
    // * messages in vrpn are identified by two different ID's
    // * one is the sender id.  It will be renamed.  An example is
    //   tracker0 or tracker1, button0 or button1, etc.
    // * the other ID is the type id.  This one speaks to how to
    //   decode the message.  Examples are ...XXX...
    // * the layer sitting above the connection (...XXX_name...)
    //   defines the types.  For example, vrpn_Tracker defines
    //   ...XXX_vel/pos/acc...
    // * when a client instantiates a vrpn_TrackerRemote,
    //   a connection is made and the pertainant types are registered
    // * the sender name is part of the connection name, for example
    //   "tracker0@ioglab.cs.unc.edu".


    // * get a token to use for the string name of the sender
    // * remember to check for -1 meaning failure
    virtual vrpn_int32 register_sender( const char * name );

    // * get a token to use for the string name of the message type
    // * remember to check for -1 meaning failure
    virtual vrpn_int32 register_message_type( const char * name );

	// * returns sender ID, or -1 if unregistered
	int get_sender_id( const char * ) const;

	// * returns message type ID, or -1 if unregistered
    // * was message_type_is_registered  
	int get_message_type_id( const char * ) const;

    // * returns the name of the specified sender,
    //   or NULL if the parameter is invalid
    // * was: sender_name
	virtual const char * get_sender_name( vrpn_int32 sender_id ) const;

	// * returns the name of the specified type,
    //   or NULL if the parameter is invalid
    // * only works for user messages (type_id >= 0)
    // * was: message_type_name
	virtual const char * get_message_type_name( vrpn_int32 type_id ) const;



public:

    // logging, etc. ...

private:
    //list <vrpn_BaseConnection *> connection_list;
};




////////////////////////////////////////////////////////
//
// class vrpn_Connection
//
// * this is provided as a transition convenience
// * it is an adapter so that old code will work as written
// * use of vrpn_Connection is deprecated
// * vrpn_Connection will set up a vrpn_ConnectionController
//   that will allow a maximum of one connection to it at a time
//   (like the old vrpn_Connection)
// * this allows the new vrpn_Connection to have an interface
//   identical to the old vrpn_Connection, while at the same time,
//   the new one is implemented in terms of vrpn_ConnectionController
//


class vrpn_Connection {
private:
    vrpn_ConnectionController * con;

protected:  // c'tors

    // constructors

    // helper function
    virtual int connect_to_client( const char *machine, int port ) {
        //XXXcon->connect_to_client( machine, port );
    }

public:  // handling incomming and outgoing messages

    // * call each time through the program main loop
    // * handles receiving any incoming messages
    //   and sending any packed messages
    // * returns -1 when connection dropped due to error, 0 otherwise
    //   (only returns -1 once per connection drop)
    // * optional argument is TOTAL time to block on select() calls;
    //   there may be multiple calls to select() per call to mainloop(),
    //   and this timeout will be divided evenly between them.
    virtual int mainloop( const timeval * timeout = NULL ) {
        con->mainloop( timeout );
    }

	// * send pending report (that have been packed), and clear the buffer
	// * this function was protected, now is public, so we can use
    //   it to send out intermediate results without calling mainloop
    virtual int send_pending_reports() {
        con->send_pending_reports();
    }

    // * pack a message that will be sent the next time mainloop() is called
	// * turn off the RELIABLE flag if you want low-latency (UDP) send
    // * was: pack_message
	virtual int pack_message(
        vrpn_uint32 len, 
        timeval time,
		vrpn_int32 type,
        vrpn_int32 sender,
        const char * buffer,
        vrpn_uint32 class_of_service )
    {
        con->handle_outgoing_message(
            len, time, type, sender, buffer, class_of_service);
    }


    // XXX ... everything else in the old vrpn_Connection
    //         but calling the functions in vrpn_ConnectionController
};




#endif  // VRPN_CONNECTIONCONTROLLER_H