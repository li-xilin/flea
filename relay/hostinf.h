#ifndef HOSTINF_H
#define HOSTINF_H
#include <stdint.h>
#include <ax/trait.h>
struct hostinf
{
        uint32_t token;
        uint32_t inaddr;
};

ax_trait_declare(hostinf, struct hostinf);

#endif
