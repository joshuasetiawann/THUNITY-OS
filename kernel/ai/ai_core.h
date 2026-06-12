/* THUOS - AI-native OS core (pure logic, no kernel/hardware dependency).
 *
 * This is the honest foundation of THUOS's AI-native design: an in-kernel model
 * of AI *services*, *models*, *tasks*, a *permission policy*, and an *audit log*.
 * It is policy/bookkeeping only - it does NOT perform inference, open sockets, or
 * run any external program. THUOS has no TCP/IP stack or language runtime yet, so
 * the "bridge" to a local AI server (Thunity backend / Ollama) is DESIGN-ONLY:
 * this core records the request, applies the local-only policy, and audits the
 * decision, but no request leaves the machine.
 *
 * Because it has no kernel deps, the same file compiles into the kernel and into
 * the host unit test (tests/test_ai.c, THUOS_HOSTED_TEST). */
#ifndef THUOS_AI_CORE_H
#define THUOS_AI_CORE_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

#define AI_NAME_MAX      28
#define AI_ENDPOINT_MAX  48
#define AI_MAX_SERVICES  8
#define AI_MAX_MODELS    12
#define AI_MAX_TASKS     16
#define AI_AUDIT_CAP     32

/* How honest a thing is - mirrors the OS feature-registry status labels. */
typedef enum {
    AI_STAT_DESIGN_ONLY = 0,  /* designed, no code path that connects     */
    AI_STAT_CONFIGURED,       /* endpoint configured, not verified here   */
    AI_STAT_REACHABLE,        /* verified reachable (needs networking - not in THUOS yet) */
    AI_STAT_UNREACHABLE
} ai_status_t;

typedef enum { AI_SVC_NONE = 0, AI_SVC_BRIDGE, AI_SVC_LOCAL_RUNTIME, AI_SVC_TOOL } ai_svc_kind_t;

typedef struct {
    char          name[AI_NAME_MAX];
    char          endpoint[AI_ENDPOINT_MAX];
    ai_svc_kind_t kind;
    ai_status_t   status;
    int           used;
} ai_service_t;

typedef enum { AI_MODEL_DECLARED = 0, AI_MODEL_PRESENT } ai_model_state_t; /* PRESENT needs a runtime - not in THUOS yet */
typedef enum { AI_ROLE_CHAT = 0, AI_ROLE_CODE, AI_ROLE_EMBED, AI_ROLE_VISION } ai_role_t;

typedef struct {
    char             name[AI_NAME_MAX];
    ai_role_t        role;
    ai_model_state_t state;
    int              used;
} ai_model_t;

/* Permission bits. The default policy encodes THUOS's local-only doctrine:
 * local inference + file read are allowed; network bridge, file write, tool
 * exec and (especially) cloud are OFF until explicitly enabled. */
enum {
    AI_PERM_LOCAL_INFER = 1u << 0,
    AI_PERM_NET_BRIDGE  = 1u << 1,   /* reach a LOCAL AI server over the network (needs TCP/IP) */
    AI_PERM_FILE_READ   = 1u << 2,
    AI_PERM_FILE_WRITE  = 1u << 3,
    AI_PERM_TOOL_EXEC   = 1u << 4,
    AI_PERM_CLOUD       = 1u << 5    /* external/cloud providers - denied by doctrine */
};

typedef enum {
    AI_TASK_CREATED = 0, AI_TASK_QUEUED, AI_TASK_DISPATCHED,
    AI_TASK_DONE, AI_TASK_DENIED, AI_TASK_FAILED
} ai_task_state_t;
typedef enum { AI_KIND_CHAT = 0, AI_KIND_CODE, AI_KIND_EMBED, AI_KIND_TOOL } ai_task_kind_t;

typedef struct {
    int            id;
    int            service;          /* service index, or -1 */
    ai_task_kind_t kind;
    ai_task_state_t state;
    char           label[AI_NAME_MAX];
    int            used;
} ai_task_t;

typedef enum { AI_AUDIT_ALLOW = 0, AI_AUDIT_DENY } ai_decision_t;
typedef struct {
    uint32_t      seq;
    char          action[AI_NAME_MAX];
    ai_decision_t decision;
    int           task_id;
    int           used;
} ai_audit_t;

typedef struct {
    ai_service_t services[AI_MAX_SERVICES];
    ai_model_t   models[AI_MAX_MODELS];
    ai_task_t    tasks[AI_MAX_TASKS];
    ai_audit_t   audit[AI_AUDIT_CAP];
    uint32_t     perms;
    int          next_task_id;
    uint32_t     audit_seq;
    int          audit_head;         /* ring write cursor */
    int          audit_total;        /* total recorded */
} ai_world_t;

/* shell subcommand identity (parsing is host-testable) */
typedef enum {
    AI_SUB_HELP = 0, AI_SUB_STATUS, AI_SUB_MODELS, AI_SUB_TASKS,
    AI_SUB_POLICY, AI_SUB_BRIDGE, AI_SUB_UNKNOWN
} ai_sub_t;

/* lifecycle */
void ai_world_init(ai_world_t *w);                /* clear + default local-only policy */

/* services / models */
int  ai_service_register(ai_world_t *w, const char *name, const char *endpoint, ai_svc_kind_t k, ai_status_t st);
int  ai_service_count(const ai_world_t *w);
int  ai_service_find(const ai_world_t *w, const char *name);
int  ai_model_register(ai_world_t *w, const char *name, ai_role_t role, ai_model_state_t st);
int  ai_model_count(const ai_world_t *w);

/* policy */
int  ai_policy_allows(const ai_world_t *w, uint32_t perm);
void ai_policy_set(ai_world_t *w, uint32_t perm, int allow);

/* tasks (transition rules validated; pure - no policy/networking here) */
int  ai_task_create(ai_world_t *w, ai_task_kind_t kind, int service, const char *label);
int  ai_task_set_state(ai_world_t *w, int task_id, ai_task_state_t to);  /* 1 ok, 0 invalid */
int  ai_task_count(const ai_world_t *w);
const ai_task_t *ai_task_get(const ai_world_t *w, int idx);

/* audit ring */
void ai_audit_record(ai_world_t *w, const char *action, ai_decision_t d, int task_id);
int  ai_audit_count(const ai_world_t *w);                 /* retrievable entries (<= cap) */
const ai_audit_t *ai_audit_recent(const ai_world_t *w, int back);  /* back=0 -> newest */

/* parsing + labels */
ai_sub_t    ai_parse_sub(const char *s);
const char *ai_status_label(ai_status_t s);
const char *ai_task_state_label(ai_task_state_t s);
const char *ai_kind_label(ai_svc_kind_t k);

#endif /* THUOS_AI_CORE_H */
