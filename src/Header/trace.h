#pragma once

#ifdef TRACING
#define TRACE(x) {fprintf(stderr, "%s\n", x);}
#else
#define TRACE(x)
#endif
