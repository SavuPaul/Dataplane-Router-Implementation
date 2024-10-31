#include <arpa/inet.h> /* ntoh, hton and inet_ functions */
#include "queue.h"
#include "lib.h"

/* Routing table */
struct route_table_entry *rtable;
int rtable_len;

/* Mac table */
struct arp_table_entry *mac_table;
int mac_table_len;

/* Trie */
TrieNode *root;

/*
 Returns a pointer (eg. &rtable[i]) to the best matching route, or NULL if there
 is no matching route.
*/
struct route_table_entry *get_best_route_linear(uint32_t ip_dest) {
	/* LPM algorithm */
	int i, idx = -1;

	for(i = 0; i < rtable_len; i++){
		if((ip_dest & rtable[i].mask) == rtable[i].prefix){
			idx = i;
		}
	}

	if(idx == -1){
		return NULL;
	} else {
		return (&rtable[idx]);
	}
}

TrieNode *get_best_route_trie(uint32_t ip_dest) {
	TrieNode *crt = root;
	int levels = 0;

	for (int i = 0; i <= 31; i++) {
		if (((ip_dest >> i) & 1) == 1) {
			if (crt->right != NULL) {
				crt = crt->right;
				levels++;
			}
		} else {
			if (crt->left != NULL) {
				crt = crt->left;
				levels++;
			}
		}
	}

	if (levels == SIZE) {
		return crt;
	} else {
		return NULL;
	}
}

struct arp_table_entry *get_mac_entry(uint32_t given_ip) {
	/* Iterate through the MAC table and search for an entry
	 * that matches given_ip. */
	for (int i = 0; i < mac_table_len; i++) {
		if(mac_table[i].ip == given_ip) {
			return &mac_table[i];
		}
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	/* Code to allocate the MAC and route tables */
	rtable = malloc(sizeof(struct route_table_entry) * MAX_LEN);
	DIE(rtable == NULL, "memory");

	mac_table = malloc(sizeof(struct arp_table_entry) * 100);
	DIE(mac_table == NULL, "memory");
	
	/* Read the static routing table and the MAC table */
	rtable_len = read_rtable(argv[1], rtable);
	mac_table_len = parse_arp_table("arp_table.txt", mac_table);

	/* Create a trie using the rtable read above */
	/* 
	 * This will represent the root of the trie, which can be seen
	 * as a sentinel because its value will not be taken into account
	 * */
	root = createTrie();
	
	// insert all prefixes from the rtable into the trie
	for (int i = 0; i < rtable_len; i++) {
		root = insert_prefix(root, rtable[i]);
	}
	
	while (1) {

		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header *) buf;
		/* Note that packets received are in network order,
		any header field which has more than 1 byte will need to be conerted to
		host order. For example, ntohs(eth_hdr->ether_type). The oposite is needed when
		sending a packet on the link, */

		struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));

		/* Check if we got an IPv4 packet */
		if (eth_hdr->ether_type != ntohs(ETHERTYPE_IP)) {
			printf("Ignored non-IPv4 packet\n");
			continue;
		}

		/* Checksum verification */
		uint16_t packet_checksum = ntohs(ip_hdr->check);
		ip_hdr->check = 0;

		if(packet_checksum != checksum((uint16_t *)ip_hdr, sizeof(struct iphdr))) {
			continue;
		}

		ip_hdr->check = htons(packet_checksum);

		// Initial liniar method call to get the route with the LPM algorithm
		// struct route_table_entry *best_route = get_best_route_linear(ip_hdr->daddr);

		// More efficient way of finding the best route with a binary tree
		TrieNode *best_route = get_best_route_trie(ip_hdr->daddr);

		if(best_route == NULL) {
			continue;
		}

		/* Check TTL >= 1. Update TLL */
		if(ip_hdr->ttl == 0) {
			continue;
		}

		ip_hdr->ttl--;
		/* Update checksum */
		ip_hdr->check = ~(~ip_hdr->check +  ~((uint16_t)ip_hdr->ttl + 1) + (uint16_t)ip_hdr->ttl) - 1;

		/* Update the ethernet addresses. Use get_mac_entry to find the destination MAC
		 * address. Use get_interface_mac(m.interface, uint8_t *mac) to
		 * find the mac address of our interface. */
		uint8_t smac[6];
		struct arp_table_entry *dest_mac_entry = get_mac_entry(best_route->next_hop);
		get_interface_mac(best_route->interface, smac);

		for(int i = 0; i < 6; i++){
			eth_hdr->ether_dhost[i] = dest_mac_entry->mac[i];
			eth_hdr->ether_shost[i] = smac[i];
		}

		/* Forward the pachet to best_route->interface */		  
		send_to_link(best_route->interface, buf, len);
	}
}

