#include "diyafm_utils.h"

void timestr(time_t time, char* buf,int len,char* format, int gmt)
{
	struct tm t;
    if(gmt)
    {
        gmtime_r(&time, &t);
    }
    else
    {
        localtime_r(&time, &t);
    }
	strftime(buf, len, format, &t);
}