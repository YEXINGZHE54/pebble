#ifndef STONE_ERROR_H
#define STONE_ERROR_H

// error while module_init
#define STONE_ERROR_MODULE  -1 
// error while triger app_init 
#define STONE_ERROR_INIT    -2
// error while sys call, such as malloc, pipe 
#define STONE_ERROR_SYSCALL -3
// error while create thread
#define STONE_ERROR_THREAD  -4
#endif

