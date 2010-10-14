#pragma once
#include "../classes.hpp"

struct rt_exception {
	struct rt_common common;
	rt_value message;
	rt_value backtrace;
};

#define RT_EXCEPTION(value) ((struct rt_exception *)(value))

extern rt_value rt_Exception;

void rt_exception_init(void);


