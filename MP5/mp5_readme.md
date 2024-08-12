This is the readme file for ECEN602 - MP5.


Roles:
Donguk Kim - Create ns2.tcl file.
Rohan Dalvi - Run tests and create report.

Architecture of the code:
This project consists of only one file, ns2.tcl, which simulates the following configuration.
• Two routers (R1, R2) connected with a 1 Mbps link and 5ms of latency 
• Two senders (src1, src2) connected to R1 with 10 Mbps links
• Two receivers (rcv1, rcv2) connected to R2 with 10 Mbps links
• Application sender is FTP over TCP
Users can simulate a 400s simulation for the following configurations.
TCP_flovor: {SACK, VEGAS}
case_no: {1, 2, 3}

Usage:
1. ns ns2.tcl <TCP_flavor> <case_no>
> ns ns2.tcl VEGAS 1
> ns ns2.tcl VEGAS 2
> ns ns2.tcl VEGAS 3
> ns ns2.tcl SACK 1
> ns ns2.tcl SACK 2
> ns ns2.tcl SACK 3 

