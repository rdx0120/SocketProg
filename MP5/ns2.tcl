# Step 0. Check input and set parameters accordingly.
if { $argc != 2 } {
    puts "Usage: ns ns2.tcl <TCP_flavor> <case_no>"
}

set TCP_flavor [lindex $argv 0]
set case_no [lindex $argv 1]

if {$TCP_flavor == "VEGAS"} {
    set TCP_flavor "Vegas"
} elseif {$TCP_flavor == "SACK"} {
    set TCP_flavor "Sack1"
} else {
    puts "TCP_flavor not supported. Exiting the simultion."
    exit
}

global delay1
set delay1 "5ms"
global delay2
set delay2 0
global counter
set counter 0
global throughput1
global throughput2
set throughput1 0
set throughput2 0

if {$case_no == 1} {
    puts "case_no 1. delay1 = 5ms, delay2 = 12.5ms"
    global delay2
    set delay2 "12.5ms"
} elseif {$case_no == 2} {
    puts "case_no 2. delay1 = 5ms, delay2 = 20ms"
    global delay2
    set delay2 "20ms"
} elseif {$case_no == 3} {
    puts "case_no 3. delay1 = 5ms, delay2 = 27.5ms"
    global delay2
    set delay2 "27.5ms"
} else {
    puts "case_no not supported. Exiting the simultion."
    exit
}

# Step 1. Create Simulator Obj 
set f1 [open out1.tr w] 
set f2 [open out2.tr w]

# Create a simulator object
set ns [new Simulator]
set file "out_$TCP_flavor$case_no"

# Step 2. Tracing
#Open the NS trace file
set tracefile [open out.tr w] 
$ns trace-all $tracefile

#Open the NAM trace file
set namfile [open out.nam w]
$ns namtrace-all $namfile

# Step 3. Create network
set src1 [$ns node]
set src2 [$ns node]
set R1 [$ns node]
set R2 [$ns node]
set rcv1 [$ns node]
set rcv2 [$ns node]

# Format: $ns duplex-link $n0 $n1 <bandwidth> <delay> <queue_type>: DropTail, RED, etc.
$ns duplex-link $R1 $R2 1.0Mb 5ms DropTail
$ns duplex-link $src1 $R1 10.0Mb $delay1 DropTail  
$ns duplex-link $src2 $R1 10.0Mb $delay2 DropTail  
$ns duplex-link $rcv1 $R2 10.0Mb $delay1 DropTail  
$ns duplex-link $rcv2 $R2 10.0Mb $delay2 DropTail  

# Step 4. Network Dynamics
# N/A

# Step 5. Creating TCP connection

# Create TCP connection
set tcp1 [new Agent/TCP/$TCP_flavor]
set tcpsink1 [new Agent/TCPSink]
$ns attach-agent $src1 $tcp1
$ns attach-agent $rcv1 $tcpsink1
$ns connect $tcp1 $tcpsink1

set tcp2 [new Agent/TCP/$TCP_flavor]
set tcpsink2 [new Agent/TCPSink]

$ns attach-agent $src2 $tcp2
$ns attach-agent $rcv2 $tcpsink2
$ns connect $tcp2 $tcpsink2

# Step 6. Creating Traffic (On Top of TCP)
# FTP Application over TCP connection
set ftp1 [new Application/FTP]
set ftp2 [new Application/FTP]
$ftp1 attach-agent $tcp1
$ftp2 attach-agent $tcp2

# record procedure 
proc record {} {
	global tcpsink1 tcpsink2 f1 f2 throughput1 throughput2 counter
	# NS simulator instance
	set ns [Simulator instance]
	# iteration time (sec)
	set time 0.5
	set now [$ns now]
        
	# traffic received by sinks
	set bandwidth1 [$tcpsink1 set bytes_]
	set bandwidth2 [$tcpsink2 set bytes_]
               
	# calculate throughputs
	puts $f1 "$now [expr $bandwidth1/$time*8/1000000]"
	puts $f2 "$now [expr $bandwidth2/$time*8/1000000]"
	if { $now >= 100 } {
		set throughput1 [expr $throughput1 + $bandwidth1/$time*8/1000000]
		set throughput2 [expr $throughput2 + $bandwidth2/$time*8/1000000]
	}
	set counter [expr $counter + 1]
        
	$tcpsink1 set bytes_ 0
	$tcpsink2 set bytes_ 0
        
	$ns at [expr $now+$time] "record"
}
##################################

# Step 7. Post-processing Procedures
#Finish procedure for terminating simulations
proc finish {} {
    global ns tracefile namfile file throughput1 throughput2 counter
    $ns flush-trace
    #puts "counter=[expr $counter]"
    puts "(first 100s ignored)"
    puts "Src1 Avg throughput: [expr $throughput1/($counter-200)] Mb/sec"
    puts "Src2 Avg throughput: [expr $throughput2/($counter-200)] Mb/sec"
    puts "src1/src2 ratio: [expr $throughput1/($throughput2)]"
    close $tracefile
    close $namfile
    #exec nam out.nam &
    exit 0
}

# Step 8. Start Simulation
$ns at 0 "record"
$ns at 0 "$ftp1 start"
$ns at 0 "$ftp2 start"
$ns at 400 "$ftp1 stop"
$ns at 400 "$ftp2 stop"
$ns at 400 "finish"

set tf1 [open "$file-S1.tr" w]
$ns trace-queue  $src1  $R1  $tf1

set tf2 [open "$file-S2.tr" w]
$ns trace-queue  $src2  $R1  $tf2
$ns run
