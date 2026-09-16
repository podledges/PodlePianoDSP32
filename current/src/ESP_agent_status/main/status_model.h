#pragma once

#include <stdbool.h>

typedef enum {
    AGENT_STATUS_WORKING,
    AGENT_STATUS_DECISION,
    AGENT_STATUS_FINISHED,
} agent_status_t;

bool status_parse_command(const char *line, agent_status_t *status);
const char *status_name(agent_status_t status);
