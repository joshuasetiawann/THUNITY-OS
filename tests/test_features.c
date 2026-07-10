/* THUOS — host unit test for the OS feature/status registry (feature_registry.c).
 * Build & run:  make test   (or: gcc -O2 tests/test_features.c -o build/test_features) */
#define THUOS_HOSTED_TEST
#include "../kernel/os/feature_registry.c"

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void) {
    feature_registry_t r;
    feat_init(&r);
    assert(feat_count(&r) == 0);

    assert(feat_add(&r, "Paging (CR0.PG)", FEAT_IMPLEMENTED) == 0);
    assert(feat_add(&r, "AI core (registry/policy/audit)", FEAT_HOST_TESTED) == 1);
    assert(feat_add(&r, "Thunity AI bridge", FEAT_DESIGN_ONLY) == 2);
    assert(feat_add(&r, "TCP/IP networking", FEAT_DESIGN_ONLY) == 3);
    assert(feat_count(&r) == 4);

    assert(feat_count_by(&r, FEAT_IMPLEMENTED) == 1);
    assert(feat_count_by(&r, FEAT_HOST_TESTED) == 1);
    assert(feat_count_by(&r, FEAT_DESIGN_ONLY) == 2);
    assert(feat_count_by(&r, FEAT_NOT_VERIFIED) == 0);

    assert(strcmp(feat_get(&r, 0)->name, "Paging (CR0.PG)") == 0);
    assert(feat_get(&r, 99) == NULL);

    /* labels are stable + non-empty (the report/preview legends depend on them) */
    for (int s = 0; s < FEAT_STATUS_COUNT; s++) assert(feat_status_label((feat_status_t)s)[0] != '?');
    assert(strcmp(feat_status_label(FEAT_DESIGN_ONLY), "design-only") == 0);

    /* capacity guard never overflows */
    feature_registry_t big;
    feat_init(&big);
    for (int i = 0; i < FEAT_MAX + 10; i++) feat_add(&big, "x", FEAT_COMPILE_ONLY);
    assert(feat_count(&big) == FEAT_MAX);

    printf("test_features: OK (add, count_by, labels, capacity guard)\n");
    return 0;
}
