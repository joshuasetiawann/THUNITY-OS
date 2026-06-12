/* THUOS - AI-native layer, kernel side. See ai.h. */
#include "types.h"
#include "kprintf.h"
#include "ai_core.h"
#include "ai.h"

static ai_world_t g_ai;

void ai_init(void) {
    ai_world_init(&g_ai);
    /* Designed bridges to a LOCAL AI server. design-only: THUOS cannot connect
     * yet (no TCP/IP). These describe where a future bridge would point. */
    ai_service_register(&g_ai, "thunity-backend", "http://127.0.0.1:8000",  AI_SVC_BRIDGE, AI_STAT_DESIGN_ONLY);
    ai_service_register(&g_ai, "ollama",          "http://127.0.0.1:11434", AI_SVC_BRIDGE, AI_STAT_DESIGN_ONLY);
    /* Example model names an operator's local Ollama might host. DECLARED only -
     * nothing is loaded inside THUOS. */
    ai_model_register(&g_ai, "llama3.1",        AI_ROLE_CHAT,  AI_MODEL_DECLARED);
    ai_model_register(&g_ai, "qwen2.5-coder",   AI_ROLE_CODE,  AI_MODEL_DECLARED);
    ai_model_register(&g_ai, "nomic-embed-text", AI_ROLE_EMBED, AI_MODEL_DECLARED);
}

static void yn(const char *name, int on) {
    kprintf("  %s : %s\n", name, on ? "ALLOW" : "DENY");
}

static void print_audit_tail(int max) {
    int n = ai_audit_count(&g_ai);
    if (n > max) n = max;
    if (n == 0) { kprintf("  (no AI actions audited yet)\n"); return; }
    for (int i = 0; i < n; i++) {
        const ai_audit_t *e = ai_audit_recent(&g_ai, i);
        if (!e) break;
        kprintf("  #%u %s : %s (task %d)\n", e->seq, e->action,
                e->decision == AI_AUDIT_ALLOW ? "ALLOW" : "DENY", e->task_id);
    }
}

static void cmd_status(void) {
    kprintf("THUOS AI-native layer - foundation (host-tested core; no inference here)\n");
    kprintf("Services (%d):\n", ai_service_count(&g_ai));
    for (int i = 0; i < AI_MAX_SERVICES; i++) {
        const ai_service_t *s = &g_ai.services[i];
        if (!s->used) continue;
        kprintf("  - %s (%s) %s [%s]\n", s->name, ai_kind_label(s->kind),
                s->endpoint, ai_status_label(s->status));
    }
    kprintf("Policy: local-infer=%s net-bridge=%s file-write=%s tool-exec=%s cloud=%s\n",
            ai_policy_allows(&g_ai, AI_PERM_LOCAL_INFER) ? "on" : "off",
            ai_policy_allows(&g_ai, AI_PERM_NET_BRIDGE)  ? "on" : "OFF",
            ai_policy_allows(&g_ai, AI_PERM_FILE_WRITE)  ? "on" : "OFF",
            ai_policy_allows(&g_ai, AI_PERM_TOOL_EXEC)   ? "on" : "OFF",
            ai_policy_allows(&g_ai, AI_PERM_CLOUD)       ? "on" : "OFF");
    kprintf("Recent AI audit:\n");
    print_audit_tail(5);
    kprintf("Note: no request leaves this machine; a bridge needs TCP/IP (not in THUOS yet).\n");
    ai_audit_record(&g_ai, "ai.status", AI_AUDIT_ALLOW, 0);
}

static void cmd_models(void) {
    kprintf("Registered models (%d) - DECLARED design entries, not loaded\n", ai_model_count(&g_ai));
    kprintf("(THUOS has no AI runtime; models live on a LOCAL Linux server e.g. Ollama)\n");
    static const char *role[] = { "chat", "code", "embed", "vision" };
    for (int i = 0; i < AI_MAX_MODELS; i++) {
        const ai_model_t *m = &g_ai.models[i];
        if (!m->used) continue;
        kprintf("  - %s (%s) [%s]\n", m->name, role[m->role],
                m->state == AI_MODEL_PRESENT ? "present" : "declared");
    }
    ai_audit_record(&g_ai, "ai.models", AI_AUDIT_ALLOW, 0);
}

static void cmd_tasks(void) {
    int n = ai_task_count(&g_ai);
    kprintf("AI tasks (%d):\n", n);
    if (n == 0) { kprintf("  (none yet - run `ai bridge` to see create->policy->audit)\n"); return; }
    for (int i = 0; i < n; i++) {
        const ai_task_t *t = ai_task_get(&g_ai, i);
        if (!t) break;
        kprintf("  #%d [%s] \"%s\" svc=%d\n", t->id, ai_task_state_label(t->state), t->label, t->service);
    }
    ai_audit_record(&g_ai, "ai.tasks", AI_AUDIT_ALLOW, 0);
}

static void cmd_policy(void) {
    kprintf("AI permission policy (local-only doctrine; nothing silently enabled):\n");
    yn("local-inference", ai_policy_allows(&g_ai, AI_PERM_LOCAL_INFER));
    yn("file-read",       ai_policy_allows(&g_ai, AI_PERM_FILE_READ));
    yn("net-bridge",      ai_policy_allows(&g_ai, AI_PERM_NET_BRIDGE));
    yn("file-write",      ai_policy_allows(&g_ai, AI_PERM_FILE_WRITE));
    yn("tool-exec",       ai_policy_allows(&g_ai, AI_PERM_TOOL_EXEC));
    yn("cloud",           ai_policy_allows(&g_ai, AI_PERM_CLOUD));
    kprintf("  net-bridge would reach a LOCAL AI server; needs TCP/IP (not in THUOS yet).\n");
    kprintf("  cloud is blocked by doctrine - THUOS adds no external/cloud AI default.\n");
    ai_audit_record(&g_ai, "ai.policy", AI_AUDIT_ALLOW, 0);
}

/* Demonstrate the full create -> policy -> audit pipeline honestly: no bytes sent. */
static void cmd_bridge(void) {
    int svc = ai_service_find(&g_ai, "thunity-backend");
    kprintf("Thunity AI bridge - DESIGN-ONLY (no connection is made)\n");
    if (svc >= 0)
        kprintf("Target: %s %s [%s]\n", g_ai.services[svc].name, g_ai.services[svc].endpoint,
                ai_status_label(g_ai.services[svc].status));
    int id = ai_task_create(&g_ai, AI_KIND_CHAT, svc, "bridge-demo");
    kprintf("  task #%d created (chat)\n", id);
    if (!ai_policy_allows(&g_ai, AI_PERM_NET_BRIDGE)) {
        ai_task_set_state(&g_ai, id, AI_TASK_DENIED);
        ai_audit_record(&g_ai, "bridge.connect", AI_AUDIT_DENY, id);
        kprintf("  policy net-bridge = DENY (off by default) -> task #%d denied\n", id);
    } else {
        ai_task_set_state(&g_ai, id, AI_TASK_QUEUED);
        ai_task_set_state(&g_ai, id, AI_TASK_FAILED);     /* cannot dispatch: no TCP/IP */
        ai_audit_record(&g_ai, "bridge.connect", AI_AUDIT_DENY, id);
        kprintf("  net-bridge allowed, but THUOS has no TCP/IP -> task #%d failed (not sent)\n", id);
    }
    kprintf("Result: no request sent. This is the PLANNED bridge, not a working one.\n");
    kprintf("To actually run Thunity AI today, use the Linux appliance on a separate\n");
    kprintf("host and (later) bridge to it. See docs/40_THUNITY_AI_BRIDGE_STRATEGY.md.\n");
}

static void cmd_help(void) {
    kprintf("THUOS AI-native layer (foundation). Usage: ai <subcommand>\n");
    kprintf("  status   services, policy summary, recent AI audit\n");
    kprintf("  models   registered model names (declared; no runtime here)\n");
    kprintf("  tasks    AI task list + state machine\n");
    kprintf("  policy   permission policy (local-only by default)\n");
    kprintf("  bridge   Thunity/Ollama bridge demo - DESIGN-ONLY (no networking)\n");
    kprintf("Honest: THUOS does not run AI inference, Docker, Python, or networking yet.\n");
}

void ai_command(const char *args) {
    switch (ai_parse_sub(args)) {
        case AI_SUB_STATUS: cmd_status(); break;
        case AI_SUB_MODELS: cmd_models(); break;
        case AI_SUB_TASKS:  cmd_tasks();  break;
        case AI_SUB_POLICY: cmd_policy(); break;
        case AI_SUB_BRIDGE: cmd_bridge(); break;
        case AI_SUB_HELP:   cmd_help();   break;
        default:
            kprintf("ai: unknown subcommand '%s' (try `ai help`)\n", args);
            break;
    }
}
