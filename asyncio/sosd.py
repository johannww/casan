#!/usr/bin/env python3

"""
This is the 'main' module of the program, containing the entry point.
"""
import signal
import argparse
from sys import stderr

from util.debug import print_debug, dbg_levels
import conf
from master import Master


def run():
    """
    Starts the program
    """
    # Read arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', type=str, help='Set the level of debugging '
                        'verbosity', default='', metavar='[[+|-]spec]...]')
    parser.add_argument('-c', type=str, help='Specify the configuration file '
                        'to use with pySos', default='./sosd.conf',
                        metavar='file')

    args = parser.parse_args()

    cf = conf.Conf()
    if not cf.parse(args.c):
        stderr.write('Cannot parse configuration file ' + args.c +
                     '\nAborting.\n')
    print_debug(dbg_levels.CONF, 'Read configuration :\n' + str(cf))

    # Start the master
    block_all_signals()
    master = Master(cf)
    master.start()
    undo_block_all_signals()

    # Finally, install signal handlers for the main event loop and start SOS.
    master.engine.start()
    wait_for_signal()

    master.stop()


# Signal related functions
def block_all_signals():
    """
    Blocks all blockable signals
    """
    global sigmask
    sigmask = signal.pthread_sigmask(signal.SIG_SETMASK, {sig for sig in range(1, signal.NSIG)})


def undo_block_all_signals():
    """
    Restore the signal mask from before the call to block_all_signals
    """
    global sigmask
    signal.pthread_sigmask(signal.SIG_UNBLOCK, sigmask)


def stop_sos(master):
    """
    Stops the SOS engine.
    :param master: reference to the instance of the master class.
    """
    print('Caught signal!')
    master.stop()


def wait_for_signal():
    """
    Blocks until a signal is received.
    """
    sigset = {signal.SIGINT, signal.SIGQUIT, signal.SIGTERM}
    signal.pthread_sigmask(signal.SIG_BLOCK, sigset)
    signal.sigwait(sigset)


if __name__ == '__main__':
    run()
