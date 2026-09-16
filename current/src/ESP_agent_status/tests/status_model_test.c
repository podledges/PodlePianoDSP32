#include <assert.h>
#include <stdio.h>

#include "status_model.h"

int main(void)
{
    agent_status_t status;

    assert(status_parse_command("STATUS working\n", &status));
    assert(status == AGENT_STATUS_WORKING);
    assert(status_parse_command(" status NEEDS-DECISION \r\n", &status));
    assert(status == AGENT_STATUS_DECISION);
    assert(status_parse_command("STATUS done", &status));
    assert(status == AGENT_STATUS_FINISHED);
    assert(!status_parse_command("STATUS unknown", &status));
    assert(!status_parse_command("STATUS working extra", &status));
    assert(!status_parse_command("working", &status));

    puts("status parser check passed");
    return 0;
}
