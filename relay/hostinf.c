#include "hostinf.h"
#include <assert.h>

static ax_fail hostinf_init(void* p, va_list *ap)
{
        assert(ap);
        struct hostinf *info = p;
        info->token = va_arg(*ap, uint32_t);
        info->inaddr = va_arg(*ap, uint32_t);
        return false;
}

static ax_fail hostinf_copy(void *dst, const void *src) {
	memcpy(dst, src, sizeof(ax_type(hostinf)));
	return false;
}

ax_trait_define(hostinf, INIT(hostinf_init), COPY(hostinf_copy));
