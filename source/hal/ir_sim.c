//
// Created by Samuel Jones on 2/21/22.
// Implemented by Stephen M. Cameron on 4/20/2023
//

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#ifdef linux
#define _GNU_SOURCE
#endif
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ir.h"
#include "badge.h"

#define DEBUG_UDP_TRAFFIC 0
#if DEBUG_UDP_TRAFFIC
#define udpdebug fprintf
#else
#define udpdebug(...) { }
#endif

#define IR_MAX_HANDLERS_PER_ID (3)
static int active_callbacks = 0;

static uint8_t current_rx_data[MAX_IR_MESSAGE_SIZE];
static IR_DATA current_rx_message = {
    .data = current_rx_data,
};

static int message_count;
/* static bool receiving_message = false; */

static ir_data_callback cb[IR_MAX_ID][IR_MAX_HANDLERS_PER_ID] = {};

/* ir_add_callback() and ir_remove_callback() are identical to what's in ir_rp2040.c */
bool ir_add_callback(ir_data_callback data_cb, IR_APP_ID app_id) {
    for (int i=0; i<IR_MAX_HANDLERS_PER_ID; i++) {
        if (cb[app_id][i] == data_cb) {
            return true; // already registered
        } else if (cb[app_id][i] == NULL) {
            cb[app_id][i] = data_cb;
            active_callbacks++;
            return true;
        }
    }
    return false;
}

bool ir_remove_callback(ir_data_callback data_cb, IR_APP_ID app_id) {
    bool removed = false;
    for (int i=0; i<IR_MAX_HANDLERS_PER_ID; i++) {
        // Once we identify the handler to remove, shuffle all later ones to the left
        if (removed || (cb[app_id][i] == data_cb)) {
            if (i < IR_MAX_HANDLERS_PER_ID - 1) {
                cb[app_id][i] = cb[app_id][i+1];
            } else {
                cb[app_id][i] = NULL;
            }
            removed = true;
        }
    }
    if (removed && active_callbacks)
        active_callbacks--;
    return removed;
}

static pthread_cond_t packet_write_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t interrupt_mutex = PTHREAD_MUTEX_INITIALIZER;
#define IR_OUTPUT_QUEUE_SIZE 16
static const IR_DATA *ir_output_queue[IR_OUTPUT_QUEUE_SIZE] = { 0 };
static int ir_output_queue_input = 0;
static int ir_output_queue_output = 0;
static int output_queue_has_new_packets = 0;
static int IRpacketOutNext = 0;
static int IRpacketOutCurr = 0;

static int ir_output_queue_empty(void)
{
	return ir_output_queue_input == ir_output_queue_output;
}

static void ir_output_queue_enqueue(const IR_DATA *packet)
{
	/* Note packet could be pointing into the stack, so we can't just
	 * store the pointer, we have to replicate the data
	 */
	IR_DATA *new_packet = malloc(sizeof(*new_packet));
	new_packet->data = malloc(packet->data_length);
	new_packet->recipient_address = packet->recipient_address;
	new_packet->app_address = packet->app_address;
	new_packet->data_length = packet->data_length;
	memcpy(new_packet->data, packet->data, packet->data_length);
	/* This data gets freed in write_udp_packets_thread_fn() */

	if (((IRpacketOutNext+1) % IR_OUTPUT_QUEUE_SIZE) != IRpacketOutCurr) {
		ir_output_queue[ir_output_queue_input] = new_packet;
		ir_output_queue_input = (ir_output_queue_input + 1) % IR_OUTPUT_QUEUE_SIZE;
		IRpacketOutNext = (IRpacketOutNext + 1) % IR_OUTPUT_QUEUE_SIZE;
	}
}

static const IR_DATA *ir_output_queue_dequeue(void)
{
	if (ir_output_queue_empty())
		return NULL; /* should not happen */
	const IR_DATA *v = ir_output_queue[ir_output_queue_output];
	ir_output_queue_output = (ir_output_queue_output + 1) % IR_OUTPUT_QUEUE_SIZE;
	IRpacketOutCurr = (IRpacketOutCurr + 1) % IR_OUTPUT_QUEUE_SIZE;
	return v;
}

static uint64_t cpu_to_be64(uint64_t v)
{
	/* This works whether the native byte order is big or little endian.  Think about it. */
	unsigned char *x = (unsigned char *) &v;
	return	((uint64_t) x[0] << 56) |
		((uint64_t) x[1] << 48) |
		((uint64_t) x[2] << 40) |
		((uint64_t) x[3] << 32) |
		((uint64_t) x[4] << 24) |
		((uint64_t) x[5] << 16) |
		((uint64_t) x[6] << 8) |
		((uint64_t) x[7] << 0);
}

static uint64_t be64_to_cpu(uint64_t v)
{
	return cpu_to_be64(v);
}

struct network_data_packet {
	uint64_t badge_id;
	uint16_t recipient_address;
	uint8_t app_address;
	uint8_t data_length;
	uint8_t data[MAX_IR_MESSAGE_SIZE];
};

struct udp_thread_info {
	unsigned short recv_port;
};

static void *write_udp_packets_thread_fn(void *thread_info)
{
	struct udp_thread_info *ti = thread_info;
	struct in_addr localhost_ip;
	int on;
	int rc, bcast;
	uint32_t netmask;
	struct ifaddrs *ifaddr, *a;
        struct sockaddr_in bcast_addr;
	int found_netmask = 0;
	unsigned short port_to_recv_on = ti->recv_port;

	free(ti);

	/* Get localhost IP addr in byte form */
	rc = inet_aton("127.0.0.1", &localhost_ip);
	if (rc == 0) {
		fprintf(stderr, "inet_aton: invalid address 127.0.0.1");
		return NULL;
	}

	/* Make a UDP datagram socket */
	bcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (bcast < 0) {
		fprintf(stderr, "socket: Failed to create UDP datagram socket: %s\n", strerror(errno));
		return NULL;
	}

	/* Set our socket up for broadcasting */
	on = 1;
	rc = setsockopt(bcast, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, (const char *) &on, sizeof(on));
	if (rc < 0) {
		fprintf(stderr, "setsockopt(SO_REUSEADDR | SO_BROADCAST): %s\n", strerror(errno));
		close(bcast);
		return NULL;
	}

	/* Get the netmask for our localhost interface */
	rc = getifaddrs(&ifaddr);
	if (rc < 0) {
		fprintf(stderr, "getifaddrs() failed: %s\n", strerror(errno));
		close(bcast);
		return NULL;
	}
	found_netmask = 0;
	for (a = ifaddr; a; a = a->ifa_next) {
		struct in_addr s;
		if (a->ifa_addr == NULL)
			continue;
		if (a->ifa_addr->sa_family != AF_INET)
			continue;
		s = ((struct sockaddr_in *) a->ifa_addr)->sin_addr;
		bcast_addr = *(struct sockaddr_in *) a->ifa_addr;
		if (s.s_addr == localhost_ip.s_addr) {
			netmask = ((struct sockaddr_in *) a->ifa_netmask)->sin_addr.s_addr;
			found_netmask = 1;
			break;
		}
	}
	if (ifaddr)
		freeifaddrs(ifaddr);
	if (!found_netmask) {
		fprintf(stderr, "failed to get netmask.\n");
		close(bcast);
		return NULL;
	}

	/* Compute the broadcast address and set the port */
	bcast_addr.sin_addr.s_addr = ~netmask | localhost_ip.s_addr;
	bcast_addr.sin_port = htons(port_to_recv_on);

	/* Start sending packets */
	pthread_mutex_lock(&interrupt_mutex);
	do {
		/* Wait for a packet to write to appear */
		udpdebug(stderr, "Waiting for a packet to appear for transmission\n");
		rc = pthread_cond_wait(&packet_write_cond, &interrupt_mutex);
		if (rc != 0)
			fprintf(stderr, "pthread_cond_wait failed\n");
		if (!output_queue_has_new_packets) {
			udpdebug(stderr, "Woken for output packet, but none present\n");
			continue;
		}
		do {
			udpdebug(stderr, "Woken for output packet, should be there\n");
			/* fucking const cancer... */
			IR_DATA *v = (IR_DATA *) ir_output_queue_dequeue();
			if (!v) {
				udpdebug(stderr, "Unexpectedly no output packet found\n");
			}

			/* Serialize data */
			struct network_data_packet ndp;
			ndp.badge_id = cpu_to_be64(badge_system_data()->badgeId);
			ndp.recipient_address = htons(v->recipient_address);
			ndp.app_address = v->app_address;
			ndp.data_length = v->data_length;
			assert(v->data_length <= MAX_IR_MESSAGE_SIZE);
			memcpy(&ndp.data[0], v->data, v->data_length);

			udpdebug(stderr, "Transmitting: rcp: 0x%04x, appid: 0x%02x, len: %d\n",
				v->recipient_address, v->app_address, v->data_length);
			udpdebug(stderr, "data: ");
			for (int i = 0; i < v->data_length; i++)
				udpdebug(stderr, " %02x", v->data[i]);
			udpdebug(stderr, "\n");
			free(v->data);
			free(v);

			rc = sendto(bcast, &ndp, sizeof(ndp), 0, (struct sockaddr *) &bcast_addr, sizeof(bcast_addr));
			if (rc < 0)
				fprintf(stderr, "sendto failed: %s\n", strerror(errno));
		} while (!ir_output_queue_empty());
	} while (1);
	pthread_mutex_unlock(&interrupt_mutex);
	return NULL;
}

static void *read_udp_packets_thread_fn(void *thread_info)
{
	struct udp_thread_info *ti = thread_info;
	int rc, bcast = -1;
	struct sockaddr_in bcast_addr;
	struct sockaddr remote_addr;
	socklen_t remote_addr_len;
	unsigned short port_to_recv_from = ti->recv_port;

	free(ti);

        /* Make ourselves a UDP datagram socket */
        bcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (bcast < 0) {
                fprintf(stderr, "read_udp_packets_thread_fn: socket() failed: %s\n", strerror(errno));
                return NULL;
        }

	/* Set our socket up for reuse, so multiple clients on this host can recv broadcast packets on same port */
	rc = setsockopt(bcast, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if (rc < 0) {
		fprintf(stderr, "setsockopt(SO_REUSEADDR: %s\n", strerror(errno));
		close(bcast);
		return NULL;
	}

	/* Bind to any address on our port */
        bcast_addr.sin_family = AF_INET;
        bcast_addr.sin_addr.s_addr = INADDR_ANY;
        bcast_addr.sin_port = htons(port_to_recv_from);
        rc = bind(bcast, (struct sockaddr *) &bcast_addr, sizeof(bcast_addr));
        if (rc < 0) {
                fprintf(stderr, "bind() bcast_addr failed: %s\n", strerror(errno));
                return NULL;
        }

	/* Start receiving packets */
        do {
		struct network_data_packet ndp;
		IR_DATA *p;
		remote_addr_len = sizeof(remote_addr);
		udpdebug(stderr, "Waiting for incoming IR packets\n");
		rc = recvfrom(bcast, &ndp, sizeof(ndp), 0, &remote_addr, &remote_addr_len);
		if (rc < 0) {
			fprintf(stderr, "recvfrom failed: %s\n", strerror(errno));
			continue;
		}
		if (be64_to_cpu(ndp.badge_id) == badge_system_data()->badgeId) {
			udpdebug(stderr, "Rejected packet we sent to ourself\n");
			continue;
		}
		udpdebug(stderr, "Received incoming IR packet\n");
		/* fprintf(stderr, "Received broadcast lobby info: addr = %08x, port = %04x\n",
				ntohl(payload.ipaddr), ntohs(payload.port)); */
		p = &current_rx_message;
		p->recipient_address = ntohs(ndp.recipient_address);
		p->app_address = ndp.app_address;
		p->data_length = ndp.data_length;
		memset(p->data, 0, MAX_IR_MESSAGE_SIZE);
		memcpy(p->data, ndp.data, ndp.data_length);
		if (p->recipient_address != IR_BADGE_ID_BROADCAST)
			/* TODO: OR recipient_address != badge address */
			continue;
		if (p->app_address > IR_MAX_ID)
			continue;
		int message_processed = 0;
		for (int i = 0; i < IR_MAX_HANDLERS_PER_ID; i++) {
			if (cb[p->app_address][i] == NULL)
				break; /* no handler */
			pthread_mutex_lock(&interrupt_mutex);
			cb[p->app_address][i](p);
			pthread_mutex_unlock(&interrupt_mutex);
			message_processed = 1;
		}
		if (message_processed)
			message_count++;
	} while(1);
	return NULL;
}

#ifdef linux
/* This extern should not be needed with #define _GNU_SOUCE #include <pthread.h>
 * but for some unknown reason (cmake problem?) I'm getting implicit
 * declaration of pthread_setname_np() without this.
 * TODO: figure this out properly.
 */
extern int pthread_setname_np(pthread_t thread, const char *name);
#endif

static void setup_ir_sensor(unsigned short port_to_recv_from)
{
	pthread_t thr;
	int rc;

	struct udp_thread_info *ti = malloc(sizeof(*ti));
	ti->recv_port = port_to_recv_from;
	rc = pthread_create(&thr, NULL, read_udp_packets_thread_fn, ti);
	if (rc < 0)
		fprintf(stderr, "Failed to create thread to read udp packets: %s\n", strerror(errno));
#ifdef linux
	pthread_setname_np(thr, "badge_ir_read");
#endif
}

static void setup_ir_transmitter(unsigned short port_to_recv_from)
{
	pthread_t thr;
	int rc;

	struct udp_thread_info *ti = malloc(sizeof(*ti));
	ti->recv_port = port_to_recv_from;
	rc = pthread_create(&thr, NULL, write_udp_packets_thread_fn, ti);
	if (rc < 0)
		fprintf(stderr, "Failed to create thread to write udp packets: %s\n", strerror(errno));
#ifdef linux
	pthread_setname_np(thr, "badge_ir_write");
#endif
}

static void setup_linux_ir_simulator(unsigned short port_to_recv_from)
{
	setup_ir_sensor(port_to_recv_from);
	setup_ir_transmitter(port_to_recv_from);
}

void disable_interrupts(void)
{
	pthread_mutex_lock(&interrupt_mutex);
}

void enable_interrupts(void)
{
	pthread_mutex_unlock(&interrupt_mutex);
}

void ir_init(void)
{
	unsigned short recv_port = 12345;

	char *rp = getenv("BADGE_RECV_PORT");
	if (rp) {
		unsigned short p;
		int rc = sscanf(rp, "%hu", &p);
		if (rc == 1)
			recv_port = p;
	}
	setup_linux_ir_simulator(recv_port);
}

bool ir_transmitting(void) {
    return false;
}

void ir_send_complete_message(const IR_DATA *data)
{
	int rc;

	pthread_mutex_lock(&interrupt_mutex);
	ir_output_queue_enqueue(data);
	output_queue_has_new_packets = 1;
	rc = pthread_cond_broadcast(&packet_write_cond);
	pthread_mutex_unlock(&interrupt_mutex);
	if (rc)
		fprintf(stderr, "pthread_cond_broadcast failed: %s\n", strerror(errno));
}

// Returns the number of data packets that were queued to send, instead of blocking to send out all data.
uint8_t ir_send_partial_message(__attribute__((unused)) const IR_DATA *data, __attribute__((unused)) uint8_t starting_sequence_num) {
	/* TODO: implement this */
	printf("stub fn: %s in %s\n", __FUNCTION__, __FILE__);
	return data->data_length;
}

// TODO maybe we can track this outside of the HAL and in the badge system files somewhere
bool ir_messages_seen(bool reset)
{
	bool r = message_count != 0;
	if (reset)
		message_count = 0;
	return r;
}

int ir_message_count(void)
{
    return message_count;
}
