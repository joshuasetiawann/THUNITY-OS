/* THUOS - OS feature + honesty-status registry (pure logic, no kernel deps).
 *
 * Encodes THUOS's honesty doctrine *in code*: every advertised OS capability is
 * tagged with exactly how real it is, so the shell (`features`), the docs, and
 * the milestone report all read from one source of truth instead of prose that
 * can drift into overclaiming. Host-tested (tests/test_features.c). */
#ifndef THUOS_FEATURE_REGISTRY_H
#define THUOS_FEATURE_REGISTRY_H

#ifdef THUOS_HOSTED_TEST
#include <stdint.h>
#include <stddef.h>
#else
#include "types.h"
#endif

typedef enum {
    FEAT_IMPLEMENTED = 0,  /* implemented and boot-verified in QEMU            */
    FEAT_HOST_TESTED,      /* logic unit-tested on the host (make test)        */
    FEAT_COMPILE_ONLY,     /* compiled into the kernel, not exercised yet      */
    FEAT_DESIGN_ONLY,      /* documented design, no runtime code path          */
    FEAT_NOT_VERIFIED,     /* present but unverified in this environment       */
    FEAT_STATUS_COUNT
} feat_status_t;

#define FEAT_NAME_MAX  40
#define FEAT_MAX       80

typedef struct {
    char          name[FEAT_NAME_MAX];
    feat_status_t status;
    int           used;
} feature_t;

typedef struct {
    feature_t items[FEAT_MAX];
    int       n;
} feature_registry_t;

void        feat_init(feature_registry_t *r);
int         feat_add(feature_registry_t *r, const char *name, feat_status_t st); /* index or -1 */
int         feat_count(const feature_registry_t *r);
int         feat_count_by(const feature_registry_t *r, feat_status_t st);
const feature_t *feat_get(const feature_registry_t *r, int idx);
const char *feat_status_label(feat_status_t st);

#endif /* THUOS_FEATURE_REGISTRY_H */
