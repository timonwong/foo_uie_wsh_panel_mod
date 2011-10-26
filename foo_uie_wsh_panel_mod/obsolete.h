#pragma once

// TODO: Report only once
static inline void print_obsolete_message(const char * message)
{
    console::formatter() << WSPM_NAME ": Warning: Obsolete: " << message;
}
