/* THUOS — OS feature + honesty-status registry (pure logic). See header. */
#include "feature_registry.h"

static void cp(char *d, const char *s, int max) {
    int i = 0;
    if (max <= 0) return;
    for (; s && s[i] && i < max - 1; i++) d[i] = s[i];
    d[i] = 0;
}

void feat_init(feature_registry_t *r) {
    r->n = 0;
    for (int i = 0; i < FEAT_MAX; i++) r->items[i].used = 0;
}

int feat_add(feature_registry_t *r, const char *name, feat_status_t st) {
    if (r->n >= FEAT_MAX) return -1;
    feature_t *f = &r->items[r->n];
    cp(f->name, name, FEAT_NAME_MAX);
    f->status = st;
    f->used = 1;
    return r->n++;
}

int feat_count(const feature_registry_t *r) { return r->n; }

int feat_count_by(const feature_registry_t *r, feat_status_t st) {
    int n = 0;
    for (int i = 0; i < r->n; i++) if (r->items[i].status == st) n++;
    return n;
}

const feature_t *feat_get(const feature_registry_t *r, int idx) {
    if (idx < 0 || idx >= r->n) return 0;
    return &r->items[idx];
}

const char *feat_status_label(feat_status_t st) {
    switch (st) {
        case FEAT_IMPLEMENTED:  return "implemented";
        case FEAT_HOST_TESTED:  return "host-tested";
        case FEAT_COMPILE_ONLY: return "compile-only";
        case FEAT_DESIGN_ONLY:  return "design-only";
        case FEAT_NOT_VERIFIED: return "not-verified-here";
        default:                return "?";
    }
}
