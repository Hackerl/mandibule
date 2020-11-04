// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// this code is used to generate a stack with auxv envv argv argc
// this stack will be given to ld-linux after our manual ELF mapping

#ifndef _FAKESTACK_H
#define _FAKESTACK_H

// utilities to build a fake "pristine stack" as if we were just loaded by the kernel

#define FSTACK_PUSH_STR(sp, s)                  \
{                                               \
    unsigned long l = strlen(s) + 1;            \
    unsigned long a = (unsigned long)sp - l;    \
    while((a % sizeof(unsigned long)) != 0)     \
        a -= 1;                                 \
    memcpy((void*)a, s, l);                     \
    sp = (void*)a;                              \
}

#define FSTACK_PUSH_LONG(sp, n)                 \
{                                               \
    unsigned long l = sizeof(unsigned long);    \
    unsigned long v = n;                        \
    sp -= l;                                    \
    memcpy(sp, &v, l);                          \
}

#define FSTACK_PUSH_AUXV(sp, auxv)              \
{                                               \
    unsigned long * a = auxv;                   \
    FSTACK_PUSH_LONG(sp, 0);                    \
    FSTACK_PUSH_LONG(sp, 0);                    \
    while(*a)                                   \
    {                                           \
        FSTACK_PUSH_LONG(sp, a[1]);             \
        FSTACK_PUSH_LONG(sp, a[0]);             \
        a += 2;                                 \
    }                                           \
}

static inline uint8_t * fake_stack(uint8_t * sp, int ac, char ** av, char ** env, unsigned long * auxv)
{
    uint8_t *   av_ptr[256];
    uint8_t *   env_ptr[256];
    int         env_max = 0;

    memset(env_ptr, 0, sizeof(env_ptr));
    memset(av_ptr, 0, sizeof(av_ptr));

    // align stack
    FSTACK_PUSH_STR(sp, "");

    // copy original env
    while(*env && env_max < 254)
    {
        FSTACK_PUSH_STR(sp, *env);
        env_ptr[env_max++] = sp;
        env ++;
    }

    // add to envdata
    FSTACK_PUSH_STR(sp, "MANMAP=1");
    env_ptr[env_max++] = sp;

    // argv data
    for(int i=0; i<ac; i++) {
        FSTACK_PUSH_STR(sp, av[ac - i - 1]);
        av_ptr[i] = sp;
    }

    // auxv
    FSTACK_PUSH_AUXV(sp, auxv);

    // envp
    FSTACK_PUSH_LONG(sp, 0);
    for(int i=0; i<env_max; i++)
        FSTACK_PUSH_LONG(sp, (unsigned long)env_ptr[i]);

    // argp
    FSTACK_PUSH_LONG(sp, 0);
    for(int i=0; i<ac; i++)
        FSTACK_PUSH_LONG(sp, (unsigned long)av_ptr[i]);
    // argc
    FSTACK_PUSH_LONG(sp, ac);

    return sp;
}


#endif
