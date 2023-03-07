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
