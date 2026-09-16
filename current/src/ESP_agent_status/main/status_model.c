#include "status_model.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

static bool token_equal(const char *start, size_t length, const char *expected)
{
    if (strlen(expected) != length) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        if (tolower((unsigned char)start[i]) != expected[i]) {
            return false;
        }
    }
    return true;
}

bool status_parse_command(const char *line, agent_status_t *status)
{
    if (!line || !status) {
        return false;
    }

    while (isspace((unsigned char)*line)) {
        ++line;
    }
    const char *command = line;
    while (*line && !isspace((unsigned char)*line)) {
        ++line;
    }
    if (!token_equal(command, (size_t)(line - command), "status")) {
        return false;
    }

    while (isspace((unsigned char)*line)) {
        ++line;
    }
    const char *value = line;
    while (*line && !isspace((unsigned char)*line)) {
        ++line;
    }
    const size_t length = (size_t)(line - value);
    while (isspace((unsigned char)*line)) {
        ++line;
    }
    if (*line != '\0') {
        return false;
    }

    if (token_equal(value, length, "working")) {
        *status = AGENT_STATUS_WORKING;
    } else if (token_equal(value, length, "decision") ||
               token_equal(value, length, "needs-decision")) {
        *status = AGENT_STATUS_DECISION;
    } else if (token_equal(value, length, "finished") || token_equal(value, length, "done")) {
        *status = AGENT_STATUS_FINISHED;
    } else {
        return false;
    }
    return true;
}

const char *status_name(agent_status_t status)
{
    switch (status) {
    case AGENT_STATUS_WORKING:
        return "WORKING";
    case AGENT_STATUS_DECISION:
        return "DECISION";
    case AGENT_STATUS_FINISHED:
        return "DONE";
    }
    return "UNKNOWN";
}
