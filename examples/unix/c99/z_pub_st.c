//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

#if Z_FEATURE_PUBLICATION == 1
int main(int argc, char **argv) {
    const char *keyexpr = "demo/example/zenoh-pico-pub";
    const char *value = "Pub from Pico!";
    const char *mode = "client";
    char *clocator = NULL;
    char *llocator = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "k:v:e:m:l:")) != -1) {
        switch (opt) {
            case 'k':
                keyexpr = optarg;
                break;
            case 'v':
                value = optarg;
                break;
            case 'e':
                clocator = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'l':
                llocator = optarg;
                break;
            case '?':
                if (optopt == 'k' || optopt == 'v' || optopt == 'e' || optopt == 'm' || optopt == 'l') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                }
                return 1;
            default:
                return -1;
        }
    }

    z_owned_config_t config = z_config_default();
    zp_config_insert(z_config_loan(&config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    if (clocator != NULL) {
        zp_config_insert(z_config_loan(&config), Z_CONFIG_CONNECT_KEY, z_string_make(clocator));
    }
    if (llocator != NULL) {
        zp_config_insert(z_config_loan(&config), Z_CONFIG_LISTEN_KEY, z_string_make(llocator));
    }

    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_config_move(&config));
    if (!z_session_check(&s)) {
        printf("Unable to open session!\n");
        return -1;
    }

    printf("Declaring publisher for '%s'...\n", keyexpr);
    z_owned_publisher_t pub = z_declare_publisher(z_session_loan(&s), z_keyexpr(keyexpr), NULL);
    if (!z_publisher_check(&pub)) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    char *buf = (char *)malloc(256);
    z_clock_t now = z_clock_now();
    for (int idx = 0; 1;) {
        if (z_clock_elapsed_ms(&now) > 1000) {
            snprintf(buf, 256, "[%4d] %s", idx, value);
            printf("Putting Data ('%s': '%s')...\n", keyexpr, buf);
            z_publisher_put(z_publisher_loan(&pub), (const uint8_t *)buf, strlen(buf), NULL);
            ++idx;

            now = z_clock_now();
        }

        zp_read(z_session_loan(&s), NULL);
        zp_send_keep_alive(z_session_loan(&s), NULL);
        zp_send_join(z_session_loan(&s), NULL);
    }

    z_undeclare_publisher(z_publisher_move(&pub));

    z_close(z_session_move(&s));

    free(buf);
    return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif
