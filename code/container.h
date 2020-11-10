#ifndef _CONTAINER_H
#define _CONTAINER_H

#include <icrt_std.h>

int get_namespace_pid(int pid, int *ns_pid)
{
    char pid_s[24] = {};
    char path[1024] = {};

    if(fmt_num(pid_s, sizeof(pid_s), pid, 10) < 0)
        return -1;

    strlcat(path, "/proc/", sizeof(path));
    strlcat(path, pid_s, sizeof(path));
    strlcat(path, "/status", sizeof(path));

    int fd = _open(path, O_RDONLY, 0);

    if (fd < 0)
        return -1;

    char c;
    int i = 0;
    char line[1024] = {};

    while (_read(fd, &c, sizeof(c)) == 1)
    {
        line[i++] = c;

        if (c == '\n')
        {
            if (strncmp(line, "NStgid:", 7) == 0)
            {
                while (i-- > 0)
                {
                    if (line[i] == '\t')
                    {
                        *ns_pid = (int)strtoul(line + i + 1, NULL, 10);
                        break;
                    }
                }

                break;
            }

            i = 0;
            memset(line, 0, sizeof(line));
        }
    }

    _close(fd);

    return 0;
}

#endif
