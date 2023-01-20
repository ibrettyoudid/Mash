// Copyright (c) 2023 Brett Curtis
#pragma once
#include "any.h"
#include "classes.h"

namespace List
{

Any eval(Any expr, Frame* env);
Any apply(Any func, Vec* args);

}