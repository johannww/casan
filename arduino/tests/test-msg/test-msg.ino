/*
 * Test program for the "message" and "option" classes
 *
 * Create and destroy messages with options
 * No packet is sent on the network
 */

#include "casan.h"

#define PATH1	"/path1"
#define PATH2	"/path2"

#define URIQUERY1 "uriquery=1"
#define URIQUERY2 "uriquery=2"
#define URIQUERY3 "uriquery=3"

#define	DEBUGINTERVAL	1

Debug debug ;

void setup ()
{
    Serial.begin (38400) ;
    debug.start (DEBUGINTERVAL) ;
}

l2net *l2 ;

void test_msg (void)
{
    l2net *l2 ;

    Serial.println (F ("STEP 1: create 2 empty messages")) ;
    Msg m1 (l2) ;
    m1.print () ;
    Msg m2 (l2) ;
    m2.print () ;

    Serial.println (F ("STEP 2: create options")) ;
    option oup1 (option::MO_Uri_Path, (void *) PATH1, sizeof PATH1-1) ;
    option oup2 (option::MO_Uri_Path, (void *) PATH2, sizeof PATH2-1) ;
    option ouq1 (option::MO_Uri_Query, (void *) URIQUERY1, sizeof URIQUERY1-1) ;
    option ouq2 (option::MO_Uri_Query, (void *) URIQUERY2, sizeof URIQUERY2-1) ;
    option ouq3 (option::MO_Uri_Query, (void *) URIQUERY3, sizeof URIQUERY3-1) ;

    // REGISTER message
    m1.set_type (COAP_TYPE_CON) ;
    m1.set_code (COAP_CODE_GET) ;
    m1.set_id (10) ;

    Serial.println (F ("STEP 3a: M1 add uriquery2")) ;
    m1.push_option (ouq2) ;
    m1.print () ;	Serial.println () ;

    Serial.println (F ("STEP 3b: M1 add uripath1")) ;
    m1.push_option (oup1) ;
    m1.print () ;	Serial.println () ;

    Serial.println (F ("STEP 3c: M1 add uripath2")) ;
    m1.push_option (oup2) ;
    m1.print () ;	Serial.println () ;

    Serial.println (F ("STEP 3d: M1 add uriquery1")) ;
    m1.push_option (ouq1) ;
    m1.print () ;	Serial.println () ;

    Serial.println (F ("STEP 3e: M1 add uriquery3")) ;
    m1.push_option (ouq3) ;
    m1.print () ;	Serial.println () ;

    m2.set_type (COAP_TYPE_NON) ;
    m2.set_code (COAP_CODE_POST) ;
    m2.set_id (11) ;

    Serial.println (F ("STEP 3f: M2 add oriquery2")) ;
    m2.push_option (ouq2) ;
    m2.print () ;	Serial.println () ;

    if (option::get_errno () != 0)
    {
	Serial.print (F ("ERROR : ERRNO => ")) ;
	Serial.println (option::get_errno ()) ;
	option::reset_errno () ;
    }

    delay (1000) ;

    return ;
}

void loop ()
{
    if (debug.heartbeat ())
	test_msg () ;
}
