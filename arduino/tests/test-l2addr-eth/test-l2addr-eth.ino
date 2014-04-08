/*
 * Test program for the "l2addr_eth" class
 */

#include <SPI.h>
#include "sos.h"

void setup ()
{
    Serial.begin(38400) ;
    Serial.println (F ("start")) ;
}

void test_l2addr (void)
{
    l2addr_eth *x = new l2addr_eth ("0a:0b:0c:0a:0b:0c") ;
    l2addr_eth *y = new l2addr_eth ("00:00:00:00:00:00") ;

    if (*x == *y)
    {
	x->print () ;
	y->print () ;
	PRINT_DEBUG_STATIC ("x == y (PROBLEM)") ;
    }
    else
    {
	x->print () ;
	y->print () ;
	PRINT_DEBUG_STATIC ("x != y (OK)") ;
    }
    *x = *y ;
    if (*x == *y)
    {
	x->print () ;
	y->print () ;
	PRINT_DEBUG_STATIC ("x == y (OK)") ;
    }
    else
    {
	x->print () ;
	y->print () ;
	PRINT_DEBUG_STATIC ("x != y (BAD)") ;
    }

    delete x ;
    delete y ;
}

void loop () 
{
    PRINT_DEBUG_STATIC ("\033[36m\tloop\033[00m") ;
    // to check if we have memory leak
    PRINT_FREE_MEM ;
    test_l2addr () ;
}