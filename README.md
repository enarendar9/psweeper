This package is used to check if a port (TCP for now) is open.
It can be used to check a range of ports.
For example, u can use './sweeper -d 5555 127.0.0.1 -r 2', which will check if
ports 5555 and 5556 are open or not.

Although this is what the psweeper does the main pieces of interest are under
the lib/ directory.
* dispatcher: It is used to dispatch work loads to a set of worker threads.
To use it a user will define a function to figure out the next workload and also
provide a function(worker function) that does the workload. The worker function
must be re-enterant as the work function will be called in multiple worker
threads.
* message-queue(msgq): This is a simple queue to send data between threads.

Using these two library functions it is possible to build
dispatcher->workers(s)->collector pattern.

Specifically in this case the workers check if a port is open or not and then
report their results to a collector thread which prints the status to a file
(stdout by default).

Misc Feature:
The worker thread tries to reuse the local-port it used previously. This part is
still work in progress and needs further optimization.

TBD:
* Clean up allocated memory.
* Make the # of worker threads dynamic. Right now they are static compiled.
* Make the queue size dynamic.
