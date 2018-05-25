#pragma once

/*
 * Bus Context
 */

#include <c-macro.h>
#include <c-rbtree.h>
#include <stdlib.h>
#include "bus/listener.h"
#include "bus/match.h"
#include "bus/name.h"
#include "bus/peer.h"
#include "util/metrics.h"
#include "util/user.h"

enum {
        _BUS_E_SUCCESS,

        BUS_E_FAILURE,
};

enum {
        BUS_LOG_POLICY_TYPE_INTERNAL,
        BUS_LOG_POLICY_TYPE_SELINUX,
};

typedef struct Bus Bus;
typedef struct Log Log;
typedef struct Message Message;
typedef struct User User;

struct Bus {
        Log *log;
        User *user;
        pid_t pid;
        char *seclabel;
        size_t n_seclabel;
        char machine_id[33];
        char guid[16];

        UserRegistry users;
        NameRegistry names;
        MatchRegistry wildcard_matches;
        MatchRegistry sender_matches;
        PeerRegistry peers;

        uint64_t n_monitors;
        uint64_t listener_ids;

        Metrics metrics;
};

#define BUS_NULL(_x) {                                                          \
                .users = USER_REGISTRY_NULL,                                    \
                .names = NAME_REGISTRY_INIT,                                    \
                .wildcard_matches = MATCH_REGISTRY_INIT((_x).wildcard_matches), \
                .sender_matches = MATCH_REGISTRY_INIT((_x).sender_matches),     \
                .peers = PEER_REGISTRY_INIT,                                    \
                .metrics = METRICS_INIT(CLOCK_THREAD_CPUTIME_ID),               \
        }

int bus_init(Bus *bus,
             Log *log,
             const char *machine_id,
             unsigned int max_bytes,
             unsigned int max_fds,
             unsigned int max_matches,
             unsigned int max_objects);
void bus_deinit(Bus *bus);

Peer *bus_find_peer_by_name(Bus *bus, Name **namep, const char *name);
int bus_get_monitor_destinations(Bus *bus, CList *destinations, Peer *sender, MessageMetadata *metadata);
int bus_get_broadcast_destinations(Bus *bus, CList *destinations, MatchRegistry *matches, Peer *sender, MessageMetadata *metadata);

void bus_log_append_transaction(Bus *bus, uint64_t sender_id, uint64_t receiver_id, NameSet *sender_names, NameSet *receiver_names, const char *sender_label, const char *receiver_label, Message *message);
void bus_log_append_policy_send(Bus *bus, int policy_type, uint64_t sender_id, uint64_t receiver_id, NameSet *sender_names, NameSet *receiver_names, const char *sender_label, const char *receiver_label, Message *message);
void bus_log_append_policy_receive(Bus *bus, uint64_t sender_id, uint64_t receiver_id, NameSet *sender_names, NameSet *receievr_names, Message *message);
