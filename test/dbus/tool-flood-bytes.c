/*
 * Flood broker with bytes
 */

#include <c-macro.h>
#include <math.h>
#include <stdlib.h>
#include "util-broker.h"
#include "util-message.h"
#include "dbus/protocol.h"

static void test_connect_blocking_fd(Broker *broker, int *fdp) {
        _c_cleanup_(c_closep) int fd = -1;
        _c_cleanup_(c_freep) void *hello = NULL;
        size_t n_hello = 0;
        uint8_t reply[316];
        ssize_t len;
        int r;

        test_message_append_sasl(&hello, &n_hello);
        test_message_append_hello(&hello, &n_hello);

        fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        assert(fd >= 0);

        r = connect(fd, (struct sockaddr *)&broker->address, broker->n_address);
        assert(r >= 0);

        len = write(fd, hello, n_hello);
        assert(len == (ssize_t)n_hello);

        len = recv(fd, reply, sizeof(reply), MSG_WAITALL);
        assert(len == (ssize_t)sizeof(reply));

        *fdp = fd;
        fd = -1;
}

#define TEST_PAYLOAD_SIZE (30 * 1024 * 1024)

static void test_flood(void) {
        _c_cleanup_(util_broker_freep) Broker *broker = NULL;
        _c_cleanup_(c_freep) char *payload = NULL;
        int fds[1];

        payload = malloc(TEST_PAYLOAD_SIZE);
        assert(payload);

        memset(payload, 'a', TEST_PAYLOAD_SIZE - 1);
        payload[TEST_PAYLOAD_SIZE - 1] = '\0';

        util_broker_new(&broker);
        util_broker_spawn(broker);

        fprintf(stderr, "spawned broker\n");

        for (unsigned int j = 0; j < sizeof(fds) / sizeof(*fds); ++j) {
                _c_cleanup_(c_freep) void *signal = NULL;
                size_t n_signal = 0;

                test_connect_blocking_fd(broker, fds + j);

                test_message_append_signal(&signal, &n_signal, payload, j + 1, j + 1);

                fprintf(stderr, "prepared signal\n");

                for (unsigned int i = 0; i < 4; ++i) {
                        size_t n_buf = 0;
                        ssize_t len;

                        do {
                                len = send(fds[j], signal + n_buf, n_signal - n_buf, 0);
                                fprintf(stderr, "sent\n");
                                assert(len > 0);
                                n_buf += len;
                        } while (n_buf < n_signal);

                        fprintf(stderr, "SIGNAL! (%u)\n", j);
                }
        }

        for (unsigned int j = 0; j < sizeof(fds) / sizeof(*fds); ++j)
                close(fds[j]);

        util_broker_terminate(broker);
}

int main(int argc, char **argv) {
        test_flood();
}
