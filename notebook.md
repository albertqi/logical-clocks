# Design Notebook

## Networking

Setting up the communication network between the processes posed an interesting
problem. While it's possible to setup pair-wise TCP sockets by either having
each client have a listening socket and connecting socket or deciding which
processes would would listen and which would connect, this seemed tedious and 
not very conducive to the messaging style of the processes. Instead, datagrams
seemed like a much better match to the problem since we don't really care about
replying and each process initiates sends. However, UDP is unreliable
and doesn't maintain ordering (even on localhost, although the error rate isn't
bad). So we used UNIX Domain datagram sockets instead. We get all the benefits of
a datagram socket but the protocol is reliable. Since making a unix socket
also makes a file where you tell it, this can be used to assign process numbers
when starting the executable.

Unfortantely Linux doesn't let you query the size of the rx queue for datagram
sockets, just the size of the incoming packet, so we have to setup our own
incoming messages queue. Not a big deal, but something to note.

## Logical Clocks and Communication

We bound the size of a single processes' logical clock to an unsigned 32 bit
integer, then just send this number over the network to avoid string parsing.
Since each process knows it's own process number (see: Networking), the order
of the clocks are always the same between processes. This makes the actual
communication between processes simple: just send a message to one (or both) 
of the other socket files in the directory that is not your own.

## Testing methadology

Since there is some randomness involved in the spec, it's important to design
the system in a way that the randomness can be removed so the system can be
tested. We did this by ensuring that all random numbers are depdenency injected
instead of being calculated within the unit.

## Takeaways

### Slow processes

One of the first things we notice from the runs is that slow processes are quickly
overrun by receiving sends from fast processes. This causes the slow process to
have a very up-to-date view of the logical clocks across the system, but also
means that other processes are unaware of the slow process' clock because it
never gets to send.

### Message queue

Despite a slow process getting overrun with receives, the rx queue rarely grows
large. This is because a particular process only receives a send from another
process 1 in 5 times the other proccess loops. Accounting for both peer processes,
a process will only receive (Hz1 + Hz2) / 5 messages per second, where Hz1 and Hz2
are the clock rates for the peer processes. Therefore, the rx queue will never
grow unless (Hz1 + Hz2) / 5 > Hz3, where Hz3 is the clock rate of the examined
process. This can be verified looking at the collected data. Looking at run 1, 
the process running at 1Hz receives a message at about 1.4 messages/second, which
is faster than it can process, so the rx queue grows unbounded. In contrast,
in run 3, process 0 which runs at 3Hz receives (5 + 6) / 5 = 2.2 messages/second.
2.2 < 3, so the rx queue for process 3 does not grow.
