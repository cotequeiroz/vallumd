/*
 * This file is part of vallumd.
 *
 * Copyright (C) 2017  Stijn Tintel <stijn@linux-ipv6.be>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <arpa/inet.h>
#include <libipset/session.h>
#include <libipset/types.h>
#include <libipset/ipset.h>
#include <string.h>

#include "log.h"

#if IPSET_PROTOCOL < 7
/* compatibility shims */

static void ipset_envopt_set(struct ipset_session *session, enum ipset_envopt opt)
{
    ipset_envopt_parse(session, opt, NULL);
}

static const char * ipset_session_report_msg(const struct ipset_session *session)
{
  return ipset_session_error(session);
}
#endif

static int exit_error(int e, struct ipset_session *sess)
{
    pr_err("ipset: %s\n", ipset_session_report_msg(sess));
    ipset_session_fini(sess);

    return e;
}

static int ip_valid(char *ipaddr)
{
    unsigned int family = 0;
    unsigned int ret = 0;
    struct sockaddr_in sa;

    family = strchr(ipaddr, '.') ? AF_INET : AF_INET6;

    ret = inet_pton(family, ipaddr, &(sa.sin_addr));
    return ret != 0;
}

int ipset_do(int c, char *set, char *elem)
{
    const struct ipset_type *type = NULL;
    enum ipset_cmd cmd = c;
    int ret = 0;
    struct ipset_session *sess;

    if (!ip_valid(elem)) {
        pr_err("ipset: %s is not a valid IP address", elem);
        return 1;
    }

    ipset_load_types();

#if IPSET_PROTOCOL >= 7
    sess = ipset_session_init(NULL);
#else
    sess = ipset_session_init(printf);
#endif
    if (sess == NULL) {
        pr_err("ipset: failed to initialize session\n");
        return 1;
    }

    if (cmd == IPSET_CMD_ADD) {
        ipset_envopt_set(sess, IPSET_ENV_EXIST);
    }

    ret = ipset_parse_setname(sess, IPSET_SETNAME, set);
    if (ret < 0) {
        return exit_error(1, sess);
    }

    type = ipset_type_get(sess, cmd);
    if (type == NULL) {
        return exit_error(1, sess);
    }

    ret = ipset_parse_elem(sess, type->last_elem_optional, elem);
    if (ret < 0) {
        return exit_error(1, sess);
    }

    ret = ipset_cmd(sess, cmd, 0);
    if (ret < 0) {
        return exit_error(1, sess);
    }

    ipset_session_fini(sess);

    return 0;
}

int ipset_add(char *set, char *elem)
{
    int ret = ipset_do(IPSET_CMD_ADD, set, elem);
    if (ret == 0) {
        pr_info("ipset: added %s to %s\n", elem, set);
    }

    return ret;
}

int ipset_del(char *set, char *elem)
{
    int ret = ipset_do(IPSET_CMD_DEL, set, elem);
    if (ret == 0) {
        pr_info("ipset: deleted %s from %s\n", elem, set);
    }

    return ret;
}
