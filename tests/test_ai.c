/* THUOS — host unit test for the AI-native OS core (ai_core.c).
 *
 * Compiles the SAME ai_core.c the kernel uses (THUOS_HOSTED_TEST) and checks the
 * service/model registries, task state machine, local-only permission policy,
 * audit ring, and shell subcommand parsing. No QEMU, no networking, no AI.
 *
 * Build & run:  make test   (or: gcc -O2 tests/test_ai.c -o build/test_ai) */
#define THUOS_HOSTED_TEST
#include "../kernel/ai/ai_core.c"

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    ai_world_t w;
    ai_world_init(&w);

    /* 1) default policy encodes the local-only doctrine */
    assert(ai_policy_allows(&w, AI_PERM_LOCAL_INFER));
    assert(ai_policy_allows(&w, AI_PERM_FILE_READ));
    assert(!ai_policy_allows(&w, AI_PERM_NET_BRIDGE));   /* OFF by default */
    assert(!ai_policy_allows(&w, AI_PERM_FILE_WRITE));
    assert(!ai_policy_allows(&w, AI_PERM_TOOL_EXEC));
    assert(!ai_policy_allows(&w, AI_PERM_CLOUD));        /* cloud denied by doctrine */

    /* 2) service registry */
    assert(ai_service_count(&w) == 0);
    int s1 = ai_service_register(&w, "thunity-backend", "http://127.0.0.1:8000", AI_SVC_BRIDGE, AI_STAT_DESIGN_ONLY);
    int s2 = ai_service_register(&w, "ollama", "http://127.0.0.1:11434", AI_SVC_BRIDGE, AI_STAT_DESIGN_ONLY);
    assert(s1 == 0 && s2 == 1);
    assert(ai_service_count(&w) == 2);
    assert(ai_service_find(&w, "ollama") == 1);
    assert(ai_service_find(&w, "nope") == -1);
    assert(w.services[0].status == AI_STAT_DESIGN_ONLY);   /* honest: not connected */

    /* 3) model registry */
    assert(ai_model_register(&w, "llama3.1", AI_ROLE_CHAT, AI_MODEL_DECLARED) == 0);
    assert(ai_model_register(&w, "qwen2.5-coder", AI_ROLE_CODE, AI_MODEL_DECLARED) == 1);
    assert(ai_model_count(&w) == 2);
    assert(w.models[0].state == AI_MODEL_DECLARED);        /* declared, not present (no runtime) */

    /* 4) task state machine — valid path */
    int t = ai_task_create(&w, AI_KIND_CHAT, s1, "chat");
    assert(t == 1 && ai_task_count(&w) == 1);
    assert(ai_task_set_state(&w, t, AI_TASK_QUEUED));
    assert(ai_task_set_state(&w, t, AI_TASK_DISPATCHED));
    assert(ai_task_set_state(&w, t, AI_TASK_DONE));
    /* terminal: no further transitions */
    assert(!ai_task_set_state(&w, t, AI_TASK_QUEUED));
    /* invalid skips rejected */
    int t2 = ai_task_create(&w, AI_KIND_CODE, s2, "code");
    assert(!ai_task_set_state(&w, t2, AI_TASK_DISPATCHED));   /* CREATED -> DISPATCHED is illegal */
    assert(ai_task_set_state(&w, t2, AI_TASK_DENIED));        /* may be denied straight away */
    assert(!ai_task_set_state(&w, 9999, AI_TASK_QUEUED));     /* unknown id */

    /* 5) audit ring records decisions, newest-first, and wraps without crashing */
    ai_audit_record(&w, "task.dispatch", AI_AUDIT_ALLOW, t);
    ai_audit_record(&w, "bridge.connect", AI_AUDIT_DENY, t2);
    assert(ai_audit_count(&w) == 2);
    assert(ai_audit_recent(&w, 0)->decision == AI_AUDIT_DENY);   /* newest */
    assert(ai_audit_recent(&w, 1)->decision == AI_AUDIT_ALLOW);
    assert(ai_audit_recent(&w, 2) == NULL);
    for (int i = 0; i < AI_AUDIT_CAP + 5; i++) ai_audit_record(&w, "x", AI_AUDIT_ALLOW, 0);
    assert(ai_audit_count(&w) == AI_AUDIT_CAP);                  /* capped, no overflow */
    assert(ai_audit_recent(&w, 0)->seq == w.audit_seq);

    /* 6) policy toggle is explicit (no silent enable) */
    ai_policy_set(&w, AI_PERM_NET_BRIDGE, 1);
    assert(ai_policy_allows(&w, AI_PERM_NET_BRIDGE));
    ai_policy_set(&w, AI_PERM_NET_BRIDGE, 0);
    assert(!ai_policy_allows(&w, AI_PERM_NET_BRIDGE));

    /* 7) shell subcommand parsing */
    assert(ai_parse_sub("") == AI_SUB_HELP);
    assert(ai_parse_sub("status") == AI_SUB_STATUS);
    assert(ai_parse_sub("models") == AI_SUB_MODELS);
    assert(ai_parse_sub("tasks") == AI_SUB_TASKS);
    assert(ai_parse_sub("policy") == AI_SUB_POLICY);
    assert(ai_parse_sub("bridge") == AI_SUB_BRIDGE);
    assert(ai_parse_sub("wat") == AI_SUB_UNKNOWN);

    /* 8) labels never NULL */
    assert(strcmp(ai_status_label(AI_STAT_DESIGN_ONLY), "design-only") == 0);
    assert(ai_task_state_label(AI_TASK_DENIED)[0] == 'd');
    assert(ai_kind_label(AI_SVC_BRIDGE)[0] == 'b');

    printf("test_ai: OK (services, models, task FSM, local-only policy, audit ring, parse)\n");
    return 0;
}
