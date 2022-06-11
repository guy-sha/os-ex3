#ifndef __REQUEST_H__
#define __REQUEST_H__
#include "threadStats.h"
#include "requestQueue.h"

void requestHandle(req_info req, thread_stats* stats);

#endif
