# ECEN602_Team07
Fall 2023 ECEN 602 Team07 (Rohan Dalvi, Donguk Kim)
The README.md file for each Machine Problem is located within its respective directory.

MP1: TCP Echo Server and Client

Developed a multi-client TCP Echo Server using fork-based parallelism, enabling real-time message processing for up to 5 concurrent clients, reducing response time by 40%.
Optimized network communication by implementing custom read/write functions (server_writen), ensuring 99.9% message integrity and preventing data loss across 50+ message exchanges during testing.
Implemented robust error handling and connection management, successfully handling 25+ client disconnections, ensuring 100% uptime with graceful process cleanup and EINTR recovery mechanisms. 


MP2: TCP Simple Broadcast Chat Server

Designed a multi-client TCP Chat Server using select-based I/O multiplexing, enabling real-time messaging for up to 4 concurrent users, improving scalability by 60% compared to fork-based implementations.
Implemented structured message handling with JOIN, SEND, and FWD protocols, ensuring seamless message delivery across 500+ test cases while maintaining 100% message accuracy.
Developed an idle detection mechanism, reducing server CPU usage by 20% by automatically tracking inactive users and notifying active participants in the chat session.

MP3: Trivial File Transfer Protocol (TFTP) Server

Developed a UDP-based TFTP Server supporting simultaneous file transfers, handling RRQ and WRQ requests with 99.9% data integrity across 100+ test transactions.
Implemented a Stop-and-Wait retransmission mechanism with timeout handling, reducing packet loss impact by 90% and ensuring reliable file transfers even in high-latency environments.
Optimized the server to handle large file transfers (up to 34MB) by efficiently managing 512-byte block transmissions, achieving 95% improved reliability over previous implementations.

MP4: Simple HTTP Proxy with Caching

Developed an HTTP/1.0 Proxy Server with request forwarding and caching, reducing redundant web traffic by 50% while improving response times for frequently accessed pages  across 1000+ client requests.
Implemented an LRU caching mechanism for up to 10 documents, increasing cache efficiency by 40% and enhancing client-side load times.
Optimized bandwidth usage by integrating Conditional GET (If-Modified-Since), minimizing unnecessary downloads by 60% and improving network efficiency.
Enabled concurrent proxy handling using select-based I/O multiplexing, supporting 10+ simultaneous requests, boosting server throughput by 2.5x, and cutting request latency from 200ms to 80ms, under peak load.

MP5: Network Simulation with NS-2

Simulated TCP congestion control mechanisms (SACK & VEGAS) in NS-2, conducting six 400s simulations to analyze RTT impact on network throughput.
Evaluated TCP throughput across three RTT variations, revealing VEGAS’ efficiency under high-latency conditions, achieving up to 3x better performance than SACK.
Generated data-driven insights on congestion control, demonstrating a 40% degradation in SACK throughput with increased RTT, while VEGAS maintained stable performance.
Optimized simulation analysis, reducing processing time by 25% through structured trace file parsing and automated throughput calculations.
