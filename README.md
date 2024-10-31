# README - Project 1 - Communication Protocols - Dataplane Router Implementation

---

### TASK1: Routing Process

The implementation for this task is taken from lab 4.

* After reading data from the files, the process waits for a packet. First, it checks if the protocol used is IPv4. If it is not, the packet is **dropped**.
* If the protocol is IPv4, the checksum is recalculated to verify the correctness of the received data. If the recalculated checksum differs from the original, the packet is considered corrupted and is **dropped**.
* If all data is correct, the next hop to the destination is determined using either the `get_best_route` function (which uses a less efficient linear search) or `get_best_route_trie` (which provides a more efficient search).
* The routing functions return the variables needed to forward the packet (next_hop and interface). If no route is found, the packet is **dropped**.
* Next, the **TTL** (Time To Live) value of the packet is checked. If TTL is 0, the packet is **dropped**. If not, TTL is decremented, and the checksum is recalculated.
* The MAC addresses in the Ethernet header are also modified, setting the source address to the current address where the packet is located, and the destination address to the next_hop.
* After these modifications, the packet is forwarded to the next hop, where the entire process described above is repeated.

---

### TASK2: Efficient Longest Prefix Match

To optimize the **Longest Prefix Match (LPM)** algorithm, a binary tree was created in which each node has two children representing values 1 or 0, corresponding to the bits in the prefix address of the routing table.

* The tree is constructed by adding each prefix from the routing table. The sizes of the prefixes are scaled according to the mask since the remaining bits are not relevant for a correct match.
* Once the tree is built, it only needs to be traversed to find the longest prefix matching `IP_DEST`.
* Information about the interface and next_hop is stored in the tree's leaf nodes, so when a match is found, we will always be at a leaf node, where we can retrieve the necessary information.

---
