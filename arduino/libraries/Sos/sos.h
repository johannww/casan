#ifndef __SOS_H__
#define __SOS_H__

#include "Arduino.h"
#include "defs.h"
#include "enum.h"
#include "option.h"
#include "l2.h"
#include "l2eth.h"
#include "msg.h"
#include "retrans.h"
#include "rmanager.h"
#include "memory_free.h"
#include "time.h"
#include "resource.h"

#define	SOS_ETH_TYPE		0x88b5		// public use for prototype

class Sos {
    public:
	Sos (l2net *l2, long int slaveid) ;
	~Sos () ;

	void register_resource (Resource *r) ;

	void reset (void) ;
	void loop () ;

	Rmanager *rmanager_ ;

    private:
	void reset_master (void) ;
	void change_master (long int hlid) ;
	bool same_master (l2addr *a) ;	

	bool is_ctl_msg (Msg &m) ;
	bool is_hello (Msg &m, long int &hlid) ;
	bool is_assoc (Msg &m, long int &sttl) ;

	void mk_ctl_msg (Msg &m) ;
	void send_discover (Msg &m) ;
	void send_assoc_answer (Msg &in, Msg &out) ;

	void print_coap_ret_type (l2_recv_t ret) ;
	void print_status (uint8_t status) ;

	time_t curtime_ ;
	Retrans *retrans_ ;
	l2addr *master_ ;		// NULL <=> broadcast
	l2net *l2_ ;
	long int slaveid_ ;		// slave id, manually config'd
	uint8_t status_ ;
	long int sttl_ ;		// slave ttl, given in assoc msg
	long int hlid_ ;		// hello ID
	int curid_ ;			// current message id

	// various timers handled by function
	Twait  twait_ ;
	Trenew trenew_ ;
} ;

#endif
