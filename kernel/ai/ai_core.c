/* THUOS — AI-native OS core (pure logic). See ai_core.h. No kernel deps. */
#include "ai_core.h"

/* tiny self-contained string helpers (so this stays freestanding + host-portable) */
static void cp(char *d, const char *s, int max) {
    int i = 0;
    if (max <= 0) return;
    for (; s && s[i] && i < max - 1; i++) d[i] = s[i];
    d[i] = 0;
}
static int eq(const char *a, const char *b) {
    int i = 0;
    for (; a[i] && b[i]; i++) if (a[i] != b[i]) return 0;
    return a[i] == b[i];
}

void ai_world_init(ai_world_t *w) {
    for (int i = 0; i < AI_MAX_SERVICES; i++) w->services[i].used = 0;
    for (int i = 0; i < AI_MAX_MODELS; i++)   w->models[i].used = 0;
    for (int i = 0; i < AI_MAX_TASKS; i++)    w->tasks[i].used = 0;
    for (int i = 0; i < AI_AUDIT_CAP; i++)    w->audit[i].used = 0;
    /* Default policy = THUOS local-only doctrine: local inference + file read on;
     * network bridge, file write, tool exec and cloud OFF until explicitly allowed. */
    w->perms       = AI_PERM_LOCAL_INFER | AI_PERM_FILE_READ;
    w->next_task_id = 1;
    w->audit_seq   = 0;
    w->audit_head  = 0;
    w->audit_total = 0;
}

/* ---- services ---- */
int ai_service_register(ai_world_t *w, const char *name, const char *endpoint,
                        ai_svc_kind_t k, ai_status_t st) {
    for (int i = 0; i < AI_MAX_SERVICES; i++) {
        if (!w->services[i].used) {
            cp(w->services[i].name, name, AI_NAME_MAX);
            cp(w->services[i].endpoint, endpoint, AI_ENDPOINT_MAX);
            w->services[i].kind = k;
            w->services[i].status = st;
            w->services[i].used = 1;
            return i;
        }
    }
    return -1;
}
int ai_service_count(const ai_world_t *w) {
    int n = 0;
    for (int i = 0; i < AI_MAX_SERVICES; i++) if (w->services[i].used) n++;
    return n;
}
int ai_service_find(const ai_world_t *w, const char *name) {
    for (int i = 0; i < AI_MAX_SERVICES; i++)
        if (w->services[i].used && eq(w->services[i].name, name)) return i;
    return -1;
}

/* ---- models ---- */
int ai_model_register(ai_world_t *w, const char *name, ai_role_t role, ai_model_state_t st) {
    for (int i = 0; i < AI_MAX_MODELS; i++) {
        if (!w->models[i].used) {
            cp(w->models[i].name, name, AI_NAME_MAX);
            w->models[i].role = role;
            w->models[i].state = st;
            w->models[i].used = 1;
            return i;
        }
    }
    return -1;
}
int ai_model_count(const ai_world_t *w) {
    int n = 0;
    for (int i = 0; i < AI_MAX_MODELS; i++) if (w->models[i].used) n++;
    return n;
}

/* ---- policy ---- */
int ai_policy_allows(const ai_world_t *w, uint32_t perm) { return (w->perms & perm) ? 1 : 0; }
void ai_policy_set(ai_world_t *w, uint32_t perm, int allow) {
    if (allow) w->perms |= perm;
    else       w->perms &= ~perm;
}

/* ---- tasks ---- */
static ai_task_t *task_by_id(ai_world_t *w, int id) {
    for (int i = 0; i < AI_MAX_TASKS; i++)
        if (w->tasks[i].used && w->tasks[i].id == id) return &w->tasks[i];
    return 0;
}
int ai_task_create(ai_world_t *w, ai_task_kind_t kind, int service, const char *label) {
    for (int i = 0; i < AI_MAX_TASKS; i++) {
        if (!w->tasks[i].used) {
            w->tasks[i].id = w->next_task_id++;
            w->tasks[i].service = service;
            w->tasks[i].kind = kind;
            w->tasks[i].state = AI_TASK_CREATED;
            cp(w->tasks[i].label, label, AI_NAME_MAX);
            w->tasks[i].used = 1;
            return w->tasks[i].id;
        }
    }
    return -1;
}
/* Allowed transitions; terminal states (DONE/DENIED/FAILED) accept nothing. */
static int valid_transition(ai_task_state_t from, ai_task_state_t to) {
    switch (from) {
        case AI_TASK_CREATED:    return to == AI_TASK_QUEUED || to == AI_TASK_DENIED || to == AI_TASK_FAILED;
        case AI_TASK_QUEUED:     return to == AI_TASK_DISPATCHED || to == AI_TASK_DENIED || to == AI_TASK_FAILED;
        case AI_TASK_DISPATCHED: return to == AI_TASK_DONE || to == AI_TASK_FAILED;
        default:                 return 0;
    }
}
int ai_task_set_state(ai_world_t *w, int task_id, ai_task_state_t to) {
    ai_task_t *t = task_by_id(w, task_id);
    if (!t) return 0;
    if (!valid_transition(t->state, to)) return 0;
    t->state = to;
    return 1;
}
int ai_task_count(const ai_world_t *w) {
    int n = 0;
    for (int i = 0; i < AI_MAX_TASKS; i++) if (w->tasks[i].used) n++;
    return n;
}
const ai_task_t *ai_task_get(const ai_world_t *w, int idx) {
    int n = 0;
    for (int i = 0; i < AI_MAX_TASKS; i++)
        if (w->tasks[i].used) { if (n == idx) return &w->tasks[i]; n++; }
    return 0;
}

/* ---- audit ring ---- */
void ai_audit_record(ai_world_t *w, const char *action, ai_decision_t d, int task_id) {
    ai_audit_t *e = &w->audit[w->audit_head];
    e->seq = ++w->audit_seq;
    cp(e->action, action, AI_NAME_MAX);
    e->decision = d;
    e->task_id = task_id;
    e->used = 1;
    w->audit_head = (w->audit_head + 1) % AI_AUDIT_CAP;
    if (w->audit_total < AI_AUDIT_CAP) w->audit_total++;
}
int ai_audit_count(const ai_world_t *w) { return w->audit_total; }
const ai_audit_t *ai_audit_recent(const ai_world_t *w, int back) {
    if (back < 0 || back >= w->audit_total) return 0;
    int idx = (w->audit_head - 1 - back) % AI_AUDIT_CAP;
    if (idx < 0) idx += AI_AUDIT_CAP;
    return &w->audit[idx];
}

/* ---- parsing + labels ---- */
ai_sub_t ai_parse_sub(const char *s) {
    if (!s || !s[0])          return AI_SUB_HELP;
    if (eq(s, "status"))      return AI_SUB_STATUS;
    if (eq(s, "models"))      return AI_SUB_MODELS;
    if (eq(s, "tasks"))       return AI_SUB_TASKS;
    if (eq(s, "policy"))      return AI_SUB_POLICY;
    if (eq(s, "bridge"))      return AI_SUB_BRIDGE;
    if (eq(s, "help"))        return AI_SUB_HELP;
    return AI_SUB_UNKNOWN;
}
const char *ai_status_label(ai_status_t s) {
    switch (s) {
        case AI_STAT_DESIGN_ONLY: return "design-only";
        case AI_STAT_CONFIGURED:  return "configured (unverified)";
        case AI_STAT_REACHABLE:   return "reachable";
        case AI_STAT_UNREACHABLE: return "unreachable";
        default:                  return "?";
    }
}
const char *ai_task_state_label(ai_task_state_t s) {
    switch (s) {
        case AI_TASK_CREATED:    return "created";
        case AI_TASK_QUEUED:     return "queued";
        case AI_TASK_DISPATCHED: return "dispatched";
        case AI_TASK_DONE:       return "done";
        case AI_TASK_DENIED:     return "denied";
        case AI_TASK_FAILED:     return "failed";
        default:                 return "?";
    }
}
const char *ai_kind_label(ai_svc_kind_t k) {
    switch (k) {
        case AI_SVC_BRIDGE:        return "bridge";
        case AI_SVC_LOCAL_RUNTIME: return "local-runtime";
        case AI_SVC_TOOL:          return "tool";
        default:                   return "none";
    }
}
