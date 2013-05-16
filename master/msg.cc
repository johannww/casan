#include <iostream>
#include <cstring>

#include "defs.h"
#include "sos.h"
#include "l2.h"
#include "msg.h"
#include "slave.h"
#include "utils.h"

int msg::global_message_id = 1 ;

// reset pointer and length
#define	RESET_PL(p,l)	do {					\
			    if (p)				\
			    {					\
				delete p ; p = 0 ; l = 0 ;	\
			    }					\
			} while (false)				// no ";"
// reset encoded message
#define	RESET_BINARY	do {					\
			    RESET_PL (msg_, msglen_) ;		\
			} while (false)				// no ";"
// reset all pointers
#define	RESET_POINTERS	do {					\
			    RESET_PL (msg_, msglen_) ;		\
			    RESET_PL (payload_, paylen_) ;	\
			    optlist_.clear () ;			\
			} while (false)				// no ";"
// reset all values and pointers (but don't deallocate them)
#define	RESET_VALUES	do {					\
			    peer_ = 0 ; reqrep_ = 0 ;		\
			    msg_ = 0     ; msglen_ = 0 ;	\
			    payload_ = 0 ; paylen_ = 0 ;	\
			    toklen_ = 0 ; ntrans_ = 0 ;		\
			    timeout_ = duration_t (0) ;		\
			    next_timeout_ = std::chrono::system_clock::time_point::max () ; \
			    expire_ = std::chrono::system_clock::time_point::max () ; \
			    sostype_ = SOS_UNKNOWN ;		\
			    id_ = 0 ;				\
			} while (false)				// no ";"
#define	STOP_TRANSMIT	do {					\
			    ntrans_ = MAX_RETRANSMIT ;		\
			} while (false)				// no ";"

#define	FORMAT_BYTE0(ver,type,toklen)				\
			((((unsigned int) (ver) & 0x3) << 6) |	\
			 (((unsigned int) (type) & 0x3) << 4) |	\
			 (((unsigned int) (toklen) & 0x7))	\
			 )
#define	COAP_VERSION(b)	(((b) [0] >> 6) & 0x3)
#define	COAP_TYPE(b)	(((b) [0] >> 4) & 0x3)
#define	COAP_TOKLEN(b)	(((b) [0]     ) & 0xf)
#define	COAP_CODE(b)	(((b) [1]))
#define	COAP_ID(b)	(((b) [2] << 8) | (b) [3])

#define	ALLOC_COPY(f,m,l)	do {				\
				    f = new byte [(l)] ;	\
				    std::memcpy (f, (m), (l)) ;	\
				} while (false)			// no ";"
// add a nul byte to ease string operations
#define	ALLOC_COPYNUL(f,m,l)	do {				\
				    f = new byte [(l) + 1] ;	\
				    std::memcpy (f, (m), (l)) ;	\
				    f [(l)]=0 ;			\
				} while (false)			// no ";"

// default constructor
msg::msg ()
{
    RESET_VALUES ;
}

// copy constructor
msg::msg (const msg &m)
{
    *this = m ;
    if (msg_)
	ALLOC_COPY (msg_, m.msg_, msglen_) ;
    if (payload_)
	ALLOC_COPYNUL (payload_, m.payload_, paylen_) ;
}

// copy assignment constructor
msg &msg::operator= (const msg &m)
{
    if (this != &m)
    {
	RESET_POINTERS ;
	*this = m ;
	if (msg_)
	    ALLOC_COPY (msg_, m.msg_, msglen_) ;
	if (payload_)
	    ALLOC_COPYNUL (payload_, m.payload_, paylen_) ;
    }
    return *this ;
}

// default destructor
msg::~msg ()
{
    RESET_POINTERS ;
}

/******************************************************************************
 * Operators
 */

// Only for received messages
int msg::operator == (msg &m)
{
    int r ;

    r = 0 ;
    if (this->msg_ != 0 && m.msg_ != 0 && this->msglen_ == m.msglen_)
	r = std::memcmp (this->msg_, m.msg_, this->msglen_) == 0 ;
    return r ;
}

/******************************************************************************
 * Message encoding and decoding (private methods)
 */

void msg::coap_encode (void)
{
    int i ;
    int opt_nb ;

    /*
     * Format message, part 1 : compute message size
     */

    msglen_ = 5 + toklen_ + paylen_ ;

    optlist_.sort () ;			// sort option list
    opt_nb = 0 ;
    for (auto &o : optlist_)
    {
	int opt_delta, opt_len ;

	msglen_++ ;			// 1 byte for opt delta & len

	opt_delta = o.optcode_ - opt_nb ;
	if (opt_delta >= 269)		// delta >= 269 => 2 bytes
	    msglen_ += 2 ;
	else if (opt_delta >= 13)	// delta \in [13..268] => 1 byte
	    msglen_ += 1 ;
	opt_nb = o.optcode_ ;

	opt_len = o.optlen_ ;
	if (opt_len >= 269)		// len >= 269 => 2 bytes
	    msglen_ += 2 ;
	else if (opt_len >= 13)		// len \in [13..268] => 1 byte
	    msglen_ += 1 ;
	msglen_ += o.optlen_ ;
    }

    /*
     * Format message, part 2 : compute a default id
     */

    if (id_ == 0)
    {
	id_ = global_message_id++ ;
	if (global_message_id > 0xffff)
	    global_message_id = 1 ;
    }

    /*
     * Format message, part 3 : build message
     */

    msg_ = new byte [msglen_] ;

    i = 0 ;
    // header
    msg_ [i++] = FORMAT_BYTE0 (SOS_VERSION, type_, toklen_) ;
    msg_ [i++] = code_ ;
    msg_ [i++] = (id_ & 0xff00) >> 8 ;
    msg_ [i++] = id_ & 0xff ;
    // token
    if (toklen_ > 0)
    {
	std::memcpy (msg_ + i, token_, toklen_) ;
	i += toklen_ ;
    }
    // options
    opt_nb = 0 ;
    for (auto &o : optlist_)
    {
	int opt_delta, opt_len ;
	int posoptheader = i ;

	msg_ [posoptheader] = 0 ;

	i++ ;
	opt_delta = int (o.optcode_) - opt_nb ;
	if (opt_delta >= 269)		// delta >= 269 => 2 bytes
	{
	    opt_delta -= 269 ;
	    msg_ [i++] = (opt_delta >> 8) & 0xff ;
	    msg_ [i++] = (opt_delta)      & 0xff ;
	    msg_ [posoptheader] |= 0xe0 ;
	}
	else if (opt_delta >= 13)	// delta \in [13..268] => 1 byte
	{
	    opt_delta -= 13 ;
	    msg_ [i++] = (opt_delta)      & 0xff ;
	    msg_ [posoptheader] |= 0xd0 ;
	}
	else
	{
	    msg_ [posoptheader] |= (opt_delta << 4) ;
	}
	opt_nb = o.optcode_ ;

	opt_len = o.optlen_ ;
	if (opt_len >= 269)		// len >= 269 => 2 bytes
	{
	    opt_len -= 269 ;
	    msg_ [i++] = (opt_len >> 8) & 0xff ;
	    msg_ [i++] = (opt_len)      & 0xff ;
	    msg_ [posoptheader] |= 0x0e ;
	}
	else if (opt_len >= 13)		// len \in [13..268] => 1 byte
	{
	    msg_ [i++] = (opt_len)      & 0xff ;
	    msg_ [posoptheader] |= 0x0d ;
	}
	else
	{
	    msg_ [posoptheader] |= opt_len ;
	}
	std::memcpy (msg_ + i, (o.optval_?o.optval_:o.staticval_), o.optlen_) ;
	i += o.optlen_ ;
    }
    // payload
    if (paylen_ > 0)
    {
	msg_ [i++] = 0xff ;			// start of payload
	std::memcpy (msg_ + i, payload_, paylen_) ;
    }
}

/* returns true if message is decoding was successful */
bool msg::coap_decode (void)
{
    bool success ;

    if (COAP_VERSION (msg_) != SOS_VERSION)
    {
	success = false ;
    }
    else
    {
	int i ;
	byte opt_nb ;

	success = true ;

	type_ = msgtype_t (COAP_TYPE (msg_)) ;
	toklen_ = COAP_TOKLEN (msg_) ;
	code_ = COAP_CODE (msg_) ;
	id_ = COAP_ID (msg_) ;
	i = 4 ;

	if (toklen_ > 0)
	{
	    std::memcpy (token_, msg_ + i, toklen_) ;
	    i += toklen_ ;
	}

	/*
	 * Options analysis
	 */

	opt_nb = 0 ;
	while (i < msglen_ && msg_ [i] != 0xff)
	{
	    int opt_delta, opt_len ;
	    option o ;

	    opt_delta = (msg_ [i] >> 4) & 0x0f ;
	    opt_len   = (msg_ [i]     ) & 0x0f ;
	    i++ ;
	    switch (opt_delta)
	    {
		case 13 :
		    opt_delta = msg_ [i] - 13 ;
		    i += 1 ;
		    break ;
		case 14 :
		    opt_delta = (msg_ [i] << 8) + msg_ [i+1] - 269 ;
		    i += 2 ;
		    break ;
		case 15 :
		    success = false ;			// recv failed
		    break ;
	    }
	    opt_nb += opt_delta ;

	    switch (opt_len)
	    {
		case 13 :
		    opt_len = msg_ [i] - 13 ;
		    i += 1 ;
		    break ;
		case 14 :
		    opt_len = (msg_ [i] << 8) + msg_ [i+1] - 269 ;
		    i += 2 ;
		    break ;
		case 15 :
		    success = false ;			// recv failed
		    break ;
	    }

	    /* register option */
	    D ("OPTION " << opt_nb) ;
	    o.optcode (option::optcode_t (opt_nb)) ;
	    o.optval ((void *)(msg_ + i), opt_len) ;
	    pushoption (o) ;

	    i += opt_len ;
	}

	paylen_ = msglen_ - i - 1 ;
	if (paylen_ > 0)
	{
	    if (msg_ [i] != 0xff)
	    {
		success = false ;
	    }
	    else
	    {
		i++ ;
		ALLOC_COPYNUL (payload_, msg_ + i, paylen_) ;
	    }
	}
	else paylen_ = 0 ;			// protect further operations
    }

    return success ;
}

/******************************************************************************
 * Send and receive functions
 */

int msg::send (void)
{
    int r ;

    if (! msg_)
	coap_encode () ;

    D ("TRANSMIT id=" << id_ << " ntrans_=" << ntrans_) ;
    r = peer_->l2 ()->send (peer_->addr (), msg_, msglen_) ;
    if (r == -1)
    {
	std::cout << "ERREUR \n" ;
    }
    else
    {
	int maxlat = peer_ ->l2 ()->maxlatency () ;

	/*
	 * Timers for reliable messages
	 */

	switch (type_)
	{
	    case MT_CON :
		if (ntrans_ == 0)
		{
		    long int nmilli ;
		    int r ;

		    /*
		     * initial timeout should be \in
		     *	[ACK_TIMEOUT ... ACK_TIMEOUT * ACK_RANDOM_FACTOR] (1)
		     *
		     * Let's name i = initial timeout, t = ACK_TIMEOUT and
		     * f = ACK_RANDOM_FACTOR
		     * (1)	<==> t <= i < t*f
		     *		<==> 1 <= i/t < f
		     *		<==> 0 <= (i/t) - 1 < f-1
		     *		<==> 0 <= ((i/t) - 1) * 1000 < (f-1)*1000
		     * So, we take a pseudo-random number r between 0 and (f-1)*1000
		     *		r = ((i/t) - 1) * 1000
		     * and compute i = t(r/1000 + 1) = t*(r + 1)/1000
		     */

		    r = random_value (int ((ACK_RANDOM_FACTOR - 1.0) * 1000)) ;
		    nmilli = ACK_TIMEOUT * (r + 1) ;
		    nmilli = nmilli / 1000 ;
		    timeout_ = duration_t (nmilli) ;
		    expire_ = DATE_TIMEOUT (EXCHANGE_LIFETIME (maxlat)) ;
		}
		else
		{
		    timeout_ *= 2 ;
		}
		next_timeout_ = std::chrono::system_clock::now () + timeout_ ;

		ntrans_++ ;
		break ;
	    case MT_NON :
		STOP_TRANSMIT ;
		expire_ = DATE_TIMEOUT (NON_LIFETIME (maxlat)) ;
		break ;
	    case MT_ACK :
	    case MT_RST :
		/*
		 * Non reliable messages : arbitrary set the retransmission
		 * counter in order to skip further retransmissions.
		 */

		STOP_TRANSMIT ;
		expire_ = DATE_TIMEOUT (MAX_RTT (maxlat)) ;	// arbitrary
		break ;
	    default :
		std::cout << "Can't happen (msg type == " << type_ << ")\n" ;
		break ;
	}
    }
    return r ;
}

l2addr *msg::recv (l2net *l2)
{
    l2addr *a ;
    int len ;

    /*
     * Reset message object to a known state
     */

    RESET_POINTERS ;
    RESET_VALUES ;

    /*
     * Read message from L2 network
     */

    len = l2->mtu () ;
    msg_ = new byte [len] ;
    pktype_ = l2->recv (&a, msg_, &len) ; 	// create a l2addr *a
    msglen_ = len ;

    if (! ((pktype_ == PK_ME || pktype_ == PK_BCAST) && coap_decode ()))
    {
	/*
	 * Packet reception failed, not addressed to me, or not a SOS packet
	 */

	delete a ;			// remove address created by l2->recv ()
	a = 0 ;
    }

#ifdef DEBUG
    if (a)
    {
	const char *p ;
	if (pktype_ == PK_ME) p = "me" ;
	if (pktype_ == PK_BCAST) p = "bcast" ;

	D ("VALID RECV -> " << p << ", id=" << id_ << ", len=" << msglen_) ;
    }
    else D ("INVALID RECV pkt=" << pktype_ << ", len=" << len) ;
#endif

    return a ;
}

void msg::stop_retransmit (void)
{
    STOP_TRANSMIT ;
}

/******************************************************************************
 * Mutators
 */

void msg::peer (slave *p)
{
    peer_ = p ;
}

void msg::token (void *data, int len)
{
    std::memcpy (token_, data, len) ;
    toklen_ = len ;
    RESET_BINARY ;
}

void msg::id (int id)
{
    id_ = id ;
    RESET_BINARY ;
}

void msg::type (msgtype_t type)
{
    type_ = type ;
    RESET_BINARY ;
}

void msg::code (int code)
{
    code_ = code ;
    RESET_BINARY ;
}

void msg::payload (void *data, int len)
{
    if (payload_)
	delete payload_ ;

    ALLOC_COPYNUL (payload_, data, len) ;
    paylen_ = len ;
    RESET_BINARY ;
}

void msg::pushoption (option &o)
{
    optlist_.push_back (o) ;
}

void msg::link_reqrep (msg *m)
{
    if (m == 0)
    {
	// unlink
	reqrep_ = m ;
	m->reqrep_ = this ;
    }
    else
    {
	m->reqrep_ = 0 ;
	reqrep_ = 0 ;
    }
}

void msg::handler (reply_handler_t func)
{
    handler_ = func ;
}

/******************************************************************************
 * Accessors
 */

slave *msg::peer (void)
{
    return peer_ ;
}

pktype_t msg::pktype (void)
{
    return pktype_ ;
}

void *msg::token (int *toklen)
{
    *toklen = toklen_ ;
    return token_ ;
}

int msg::id (void)
{
    return id_ ;
}

msg::msgtype_t msg::type (void)
{
    return type_ ;
}

int msg::code (void)
{
    return code_ ;
}

void *msg::payload (int *paylen)
{
    *paylen = paylen_ ;
    return payload_ ;
}

msg *msg::reqrep (void)
{
    return reqrep_ ;
}

option msg::popoption (void)
{
    option o ;

    o = optlist_.front () ;
    optlist_.pop_front () ;
    return o ;
}

#define	SOS_REGISTER_STRING	"POST /.well-known/sos?register="
#define	SOS_ASSOCIATE_STRING	"POST /.well-known/sos?associate="

slaveid_t msg::is_sos_register (void)
{
    int len = sizeof SOS_REGISTER_STRING - 1 ;
    slaveid_t sid = 0 ;

    if (type_ == MT_NON && code_ == MC_POST
	    && paylen_ > len
	    && std::memcmp (payload_, SOS_REGISTER_STRING, len) == 0
	    )
    {
	// we benefit from the implicit nul byte at the end
	sid = std::atol ((const char *) payload_ + len) ;
	sostype_ = SOS_REGISTER ;
    }
    return sid ;
}

bool msg::is_sos_associate (void)
{
    if (sostype_ == SOS_UNKNOWN)
    {
	int len = sizeof SOS_ASSOCIATE_STRING - 1 ;

	if (type_ == MT_CON && code_ == MC_POST
		&& paylen_ > len
		&& std::memcmp (payload_, SOS_ASSOCIATE_STRING, len) == 0
		)
	{
	    sostype_ = SOS_ASSOC_REQUEST ;
	}
    }
    return sostype_ == SOS_ASSOC_REQUEST ;
}

msg::sostype_t msg::sos_type (void)
{
    if (sostype_ == SOS_UNKNOWN)
    {
	sostype_ = SOS_NONE ;
	if (is_sos_register () == 0 && ! is_sos_associate ())
	{
	    if (reqrep_ != 0)
	    {
		sostype_t st = reqrep_->sos_type () ;
		if (st == SOS_ASSOC_REQUEST)
		    sostype_ = SOS_ASSOC_ANSWER ;
	    }
	}
    }
    return sostype_ ;
}
