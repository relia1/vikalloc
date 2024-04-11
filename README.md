<!---
Used https://www.makeareadme.com/ as a reference for making this readme
-->
# Vikalloc
Vikalloc is a simple heap implementation in C using the next-fit algorithm.

## Installation
`make`

## Usage
#### Vikalloc
```c
#include "vikalloc.h"

void * item = vikalloc(sizeof(int)*100);
```

#### Vikfree
```c
#include "vikalloc.h"

void * item = vikalloc(sizeof(int)*100);
vikfree(item);
```

#### Vikalloc Reset
```
#include "vikalloc.h"

void * item = vikalloc(sizeof(int)*100);
vikfree(item);
vikalloc_reset();
```

#### Vikcalloc
```
#include "vikalloc.h"

void * item = vikcalloc(sizeof(int)*100, 100);
```

#### Vikrealloc
```
#include "vikalloc.h"

void * item = vikalloc(sizeof(int)*100);
item = vikrealloc(item, 150);
```

#### Vikstrdup
```
#include "vikalloc.h"

char name[] = "Robert";
char * name_copy = vikstrdup(name);
```

## License
[MIT](https://choosealicense.com/licenses/mit/)

