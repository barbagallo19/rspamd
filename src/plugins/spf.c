/*
 * Copyright (c) 2009, Rambler media
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Rambler media ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Rambler BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/***MODULE:spf
 * rspamd module that checks spf records of incoming email
 *
 * Allowed options:
 * - symbol_allow (string): symbol to insert (default: 'R_SPF_ALLOW')
 * - symbol_fail (string): symbol to insert (default: 'R_SPF_FAIL')
 * - symbol_softfail (string): symbol to insert (default: 'R_SPF_SOFTFAIL')
 */

#include "../config.h"
#include "../main.h"
#include "../message.h"
#include "../modules.h"
#include "../cfg_file.h"
#include "../expressions.h"
#include "../util.h"
#include "../view.h"
#include "../map.h"
#include "../spf.h"

#define DEFAULT_SYMBOL_FAIL "R_SPF_FAIL"
#define DEFAULT_SYMBOL_SOFTFAIL "R_SPF_SOFTFAIL"
#define DEFAULT_SYMBOL_ALLOW "R_SPF_ALLOW"

struct spf_ctx {
	int                             (*filter) (struct worker_task * task);
	char                           *symbol_fail;
	char                           *symbol_softfail;
	char                           *symbol_allow;

	memory_pool_t                  *spf_pool;
};

static struct spf_ctx        *spf_module_ctx = NULL;

static void                   spf_symbol_callback (struct worker_task *task, void *unused);

int
spf_module_init (struct config_file *cfg, struct module_ctx **ctx)
{
	spf_module_ctx = g_malloc (sizeof (struct spf_ctx));

	spf_module_ctx->spf_pool = memory_pool_new (memory_pool_get_size ());

	*ctx = (struct module_ctx *)spf_module_ctx;

	return 0;
}


int
spf_module_config (struct config_file *cfg)
{
	char                           *value;
	int                             res = TRUE;
	
	if ((value = get_module_opt (cfg, "spf", "symbol_fail")) != NULL) {
		spf_module_ctx->symbol_fail = memory_pool_strdup (spf_module_ctx->spf_pool, value);
	}
	else {
		spf_module_ctx->symbol_fail = DEFAULT_SYMBOL_FAIL;
	}
	if ((value = get_module_opt (cfg, "spf", "symbol_softfail")) != NULL) {
		spf_module_ctx->symbol_softfail = memory_pool_strdup (spf_module_ctx->spf_pool, value);
	}
	else {
		spf_module_ctx->symbol_softfail = DEFAULT_SYMBOL_SOFTFAIL;
	}
	if ((value = get_module_opt (cfg, "spf", "symbol_allow")) != NULL) {
		spf_module_ctx->symbol_allow = memory_pool_strdup (spf_module_ctx->spf_pool, value);
	}
	else {
		spf_module_ctx->symbol_allow = DEFAULT_SYMBOL_ALLOW;
	}

	register_symbol (&cfg->cache, spf_module_ctx->symbol_fail, 1, spf_symbol_callback, NULL);

	return res;
}

int
spf_module_reconfig (struct config_file *cfg)
{
	memory_pool_delete (spf_module_ctx->spf_pool);
	spf_module_ctx->spf_pool = memory_pool_new (memory_pool_get_size ());

	return spf_module_config (cfg);
}

static void 
spf_plugin_callback (struct spf_record *record, struct worker_task *task)
{
	GList *cur;
	struct spf_addr *addr;
	uint32_t s, m;

	if (record) {
		cur = g_list_first (record->addrs);
		s = ntohl (task->from_addr.s_addr);
		while (cur) {
			addr = cur->data;
            if (addr != NULL) {
                if (addr->mask == 0) {
                    m = 0;
                }
                else {
                    m = G_MAXUINT32 << (32 - addr->mask);
                }
                if ((s & m) == (addr->addr & m)) {
                    switch (addr->mech) {
                        case SPF_FAIL:
                            insert_result (task, spf_module_ctx->symbol_fail, 1, g_list_prepend (NULL, addr->spf_string));
							task->messages = g_list_prepend (task->messages, "(SPF): spf fail");
                            break;
                        case SPF_SOFT_FAIL:
                        case SPF_NEUTRAL:
                            insert_result (task, spf_module_ctx->symbol_softfail, 1, g_list_prepend (NULL, addr->spf_string));
							task->messages = g_list_prepend (task->messages, "(SPF): spf softfail");
                            break;
                        default:
                            insert_result (task, spf_module_ctx->symbol_allow, 1, g_list_prepend (NULL, addr->spf_string));
							task->messages = g_list_prepend (task->messages, "(SPF): spf allow");
                            break;
                    }
                    /* Stop parsing */
                    break;
                }
            }
			cur = g_list_next (cur);
		}
	}
	if (task->save.saved == 0) {
		/* Call other filters */
		task->save.saved = 1;
		process_filters (task);
	}
}


static void 
spf_symbol_callback (struct worker_task *task, void *unused)
{
	if (task->from_addr.s_addr != INADDR_NONE && task->from_addr.s_addr != INADDR_ANY) {
		if (!resolve_spf (task, spf_plugin_callback)) {
			msg_info ("cannot make spf request for [%s]", task->message_id);
		}
	}
}
