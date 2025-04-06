/*************************************************************************
 *                                                                       *
 * https://github.com/hharte/ArduinoCLI                                  *
 *                                                                       *
 * Copyright (c) 2025 Howard M. Harte                                    *
 *                                                                       *
 * Module Description:                                                   *
 * Command-line interface for Arduino serial console.                    *
 *                                                                       *
 * Arduino 2.3.3 or later                                                *
 *                                                                       *
 *************************************************************************/

 /*!
+ * \file ArduinoCLI.cpp
+ * \brief Implements the ArduinoCLI class methods.
+ */

#include "ArduinoCLI.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Constructor */
ArduinoCLI::ArduinoCLI(Stream& serialPort, const CLI_Command_t commands[], size_t commandCount) :
    _serial(serialPort),
    _commands(commands),
    _commandCount(commandCount),
    _isRunning(false), /* Start in non-running state */
    _lineBuffer(nullptr),
    _maxLineLen(CLI_DEFAULT_MAX_LINE_LEN),
    _bufferPos(0),
    _argv(nullptr),
    _maxArgs(CLI_DEFAULT_MAX_ARGS + 1)
{
    strncpy(_prompt, CLI_DEFAULT_PROMPT, CLI_MAX_PROMPT_LEN - 1);
    _prompt[CLI_MAX_PROMPT_LEN - 1] = '\0';
    _allocateBuffers();
    _resetBuffer();
    /* Prompt printed by start() */
}

/* Destructor - uncomment if dynamic allocation needs cleanup */
/*
ArduinoCLI::~ArduinoCLI() {
    _freeBuffers();
}
*/

/* Configuration */
void ArduinoCLI::setMaxLineLen(size_t len) {
    if (len > 0) _maxLineLen = len;
    /* Note: Does not reallocate buffers if already allocated */
}

void ArduinoCLI::setMaxArgs(size_t num) {
    if (num > 0) _maxArgs = num + 1; /* +1 for command name */
     /* Note: Does not reallocate buffers if already allocated */
}

void ArduinoCLI::setPrompt(const char* prompt) {
    if (prompt) {
        strncpy(_prompt, prompt, CLI_MAX_PROMPT_LEN - 1);
        _prompt[CLI_MAX_PROMPT_LEN - 1] = '\0';
    }
}

/* Allocate memory for buffers */
bool ArduinoCLI::_allocateBuffers() {
    _freeBuffers(); /* Free existing if any */
    _lineBuffer = (char*)malloc(_maxLineLen * sizeof(char));
    _argv = (char**)malloc(_maxArgs * sizeof(char*));

    if (!_lineBuffer || !_argv) {
        /* Try printing error even if serial might not be ready */
        if (&_serial) _serial.println(F("Error: CLI buffer allocation failed!"));
        _freeBuffers(); /* Ensure consistency */
        return false;
    }
    return true;
}

/* Free buffer memory */
void ArduinoCLI::_freeBuffers() {
    if (_lineBuffer) {
        free(_lineBuffer);
        _lineBuffer = nullptr;
    }
    if (_argv) {
        free(_argv);
        _argv = nullptr;
    }
}


/* Reset input buffer */
void ArduinoCLI::_resetBuffer() {
    if (_lineBuffer) {
        _lineBuffer[0] = '\0';
    }
    _bufferPos = 0;
}

/* Print the prompt, preceded by CRLF */
void ArduinoCLI::_printPrompt() {
    _serial.print(F("\r\n"));
    _serial.print(_prompt);
}

/* Echo character (if needed) */
void ArduinoCLI::_printChar(char c) {
    _serial.write(c);
}

/* Start or restart CLI processing */
void ArduinoCLI::start() {
    _isRunning = true;
    /* Print initial prompt */
    _printPrompt();
}

/* Stop processing */
void ArduinoCLI::stop() {
    _isRunning = false;
}

/* Check run status */
bool ArduinoCLI::isRunning() const {
    return _isRunning;
}

/* Access Serial */
Stream& ArduinoCLI::getSerial() {
    return _serial;
}


/* Poll for input (call in loop) */
void ArduinoCLI::poll() {
    if (!_isRunning || !_lineBuffer) return; /* Don't process if stopped or alloc failed */

    while (_serial.available() > 0) {
        char c = _serial.read();

        /* Handle Line Endings (CR, LF, or CR+LF) */
        if (c == '\r' || c == '\n') {
             if (_bufferPos > 0) { /* Process only if buffer has content */
                 _lineBuffer[_bufferPos] = '\0'; /* Null-terminate */
                 processInput(_lineBuffer);
             }
             /* Reset buffer and print prompt (if still running) */
             _resetBuffer();
             if (_isRunning) {
                 _printPrompt();
             }
             /* Skip potential second character of CRLF or LFCR */
             if (_serial.available() > 0) {
                 char next_c = _serial.peek();
                 if ((c == '\r' && next_c == '\n') || (c == '\n' && next_c == '\r')) {
                     _serial.read(); // Consume the second character
                 }
             }

        }
        /* Handle Tab Completion */
        else if (c == '\t') {
            _handleTab();
        }
        /* Handle Backspace/Delete */
        else if (c == 127 || c == '\b') { /* Handle DEL and Backspace */
            if (_bufferPos > 0) {
                _bufferPos--;
                _lineBuffer[_bufferPos] = '\0';
                /* Attempt visual backspace - may not work on all terminals */
                _serial.write("\b \b");
            }
        }
         /* Handle Ctrl+C (End of Text) - Simple version: clear line */
        else if (c == 3) {
            _resetBuffer();
            _serial.println("^C");
            _printPrompt();
        }
        /* Handle printable characters */
        else if (isprint(c)) {
            if (_bufferPos < _maxLineLen - 1) {
                _lineBuffer[_bufferPos++] = c;
                _lineBuffer[_bufferPos] = '\0'; /* Keep null-terminated */
                _printChar(c); /* Echo character */
            } else {
                /* Buffer full: optional bell sound */
                _serial.write('\a');
            }
        }
        /* Ignore other non-printable characters */
    }
}

/* Process a completed line */
void ArduinoCLI::processInput(char* line) {
     if (!_isRunning || !_lineBuffer || !_argv) return; /* Safety checks */
     _parseAndExecute(line);
}

/*
 * Simple tokenizer based on strtok_r (reentrant)
 * Modifies the input string 'line'!
 */
int ArduinoCLI::_splitLine(char *line, char **argv_local, size_t max_args_local) {
    int argc = 0;
    char *token;
    char *saveptr; /* For strtok_r */

    token = strtok_r(line, " \t\r\n\a", &saveptr);
    while (token != NULL && (size_t)argc < max_args_local - 1) {
        argv_local[argc++] = token;
        token = strtok_r(NULL, " \t\r\n\a", &saveptr);
    }
    argv_local[argc] = NULL; /* Null-terminate argv array */
    return argc;
}


/* Find command based on prefix */
const CLI_Command_t* ArduinoCLI::_findCommand(const char *prefix) {
    const CLI_Command_t *found_cmd = NULL;
    const CLI_Command_t *exact_match = NULL;
    int match_count = 0;
    size_t prefix_len = strlen(prefix);

    if (prefix_len == 0) return NULL;

    for (size_t i = 0; i < _commandCount; i++) {
         if (_commands[i].name == NULL) continue; /* Skip potentially null entries if any */

        /* Check for exact match */
        if (strcmp(prefix, _commands[i].name) == 0) {
            exact_match = &_commands[i];
        }
        /* Check for prefix match */
        if (strncmp(prefix, _commands[i].name, prefix_len) == 0) {
            if (found_cmd == NULL) {
                found_cmd = &_commands[i]; /* First potential match */
            }
            match_count++;
        }
    }

    /* Preference rule: Exact match wins */
    if (exact_match) {
        return exact_match;
    }

    /* If no exact match, check prefix matches */
    if (match_count == 1) {
        return found_cmd; /* Unique prefix match */
    }

    return NULL; /* Ambiguous or not found */
}

/* Parse input line and execute the command */
void ArduinoCLI::_parseAndExecute(char *line) {
    if (!_lineBuffer || !_argv) return; /* Alloc check */

    /* Skip leading whitespace */
    while (isspace((unsigned char)*line)) line++;

    if (*line == '\0') {
        return; /* Empty line */
    }

    /* strtok modifies the string, which is fine as _lineBuffer holds the command */
    int argc = _splitLine(line, _argv, _maxArgs);

    if (argc == 0) {
        return; /* Line had only whitespace */
    }

    const CLI_Command_t *cmd = _findCommand(_argv[0]);

    if (cmd == NULL) {
        /* Check for ambiguity for error message */
        int match_count = 0;
        size_t prefix_len = strlen(_argv[0]);
         for (size_t i = 0; i < _commandCount; i++) {
             if (_commands[i].name != NULL && strncmp(_argv[0], _commands[i].name, prefix_len) == 0) {
                 match_count++;
             }
        }

        _serial.println();
        if (match_count > 1) {
             _serial.print(F("Error: Ambiguous command '"));
             _serial.print(_argv[0]);
             _serial.println(F("'."));
        } else {
             _serial.print(F("Error: Unknown command '"));
             _serial.print(_argv[0]);
             _serial.println(F("'. Type 'help' for list."));
        }
        return; // Exit after printing error
    }

    /* Validate argument count */
    int user_args = argc - 1;
    if (user_args > cmd->max_args) {

        _serial.println();
        _serial.print(F("Error: Too many arguments for '"));
        _serial.print(cmd->name);
        _serial.print(F("' (max: "));
        _serial.print(cmd->max_args);
        _serial.print(F(", got: "));
        _serial.print(user_args);
        _serial.println(F(")."));
        return; // Exit after printing error
    }

    /* Execute command */
    if (cmd->func != NULL) {
        _serial.println();
        /* Pass 'this' pointer so command can access serial etc. if needed */
        cmd->func(this, argc, _argv);
    }
}

/* --- Tab Completion Logic (Arduino Adaptation) --- */

/* Find Longest Common Prefix */
int ArduinoCLI::_findLcp(const char *matches[], int count) {
    if (count <= 0) return 0;
    if (count == 1) return strlen(matches[0]);

    int lcp_len = 0;
    const char *first = matches[0];
    while (true) {
        char current_char = first[lcp_len];
        if (current_char == '\0') break;

        bool all_match = true;
        for (int i = 1; i < count; i++) {
            if (matches[i][lcp_len] == '\0' || matches[i][lcp_len] != current_char) {
                all_match = false;
                break;
            }
        }
        if (all_match) lcp_len++;
        else break;
    }
    return lcp_len;
}

/* Handle Tab key press */
void ArduinoCLI::_handleTab() {
    if (!_lineBuffer) return; /* Check allocation */

    /* Extract the current word being typed (only completes first word/command) */
    char current_word[CLI_DEFAULT_MAX_LINE_LEN];
    strncpy(current_word, _lineBuffer, _bufferPos);
    current_word[_bufferPos] = '\0';

    /* Don't complete if there's a space (only complete command name) */
    if (strchr(current_word, ' ')) {
       _serial.write('\a'); /* Bell sound - can't complete args yet */
       return;
    }
    size_t current_len = strlen(current_word);
    if (current_len == 0) return; /* Nothing to complete */


    /* Find matches */
    const char *matches[_commandCount];
    int match_count = 0;
    for (size_t i = 0; i < _commandCount; i++) {
        if (_commands[i].name != NULL && strncmp(current_word, _commands[i].name, current_len) == 0) {
             if ((size_t)match_count < _commandCount) {
                matches[match_count++] = _commands[i].name;
             } else {
                break; /* Should not happen */
             }
        }
    }


    if (match_count == 0) {
        _serial.write('\a'); /* Bell sound */
        return;
    }

    if (match_count == 1) {
        /* Single match: try to complete inline */
        const char *completion = matches[0];
        size_t completion_len = strlen(completion);
        size_t remaining_len = completion_len - current_len;

        if (_bufferPos + remaining_len < _maxLineLen - 2) {
            /* Attempt inline completion (TERMINAL DEPENDENT) */
            strncpy(_lineBuffer + _bufferPos, completion + current_len, remaining_len);
            _lineBuffer[_bufferPos + remaining_len] = ' ';
            _lineBuffer[_bufferPos + remaining_len + 1] = '\0';

            _serial.print(completion + current_len);
            _serial.print(' ');

            _bufferPos += remaining_len + 1;
        } else {
            _serial.write('\a'); /* Not enough space */
        }
    } else { // match_count > 1
        /* Multiple matches: find LCP */
        int lcp_len = _findLcp(matches, match_count);
        if ((size_t)lcp_len > current_len) {
            /* Complete up to LCP (TERMINAL DEPENDENT) */
            size_t remaining_len = (size_t)lcp_len - current_len;
            if (_bufferPos + remaining_len < _maxLineLen - 1) {
                strncpy(_lineBuffer + _bufferPos, matches[0] + current_len, remaining_len);
                _lineBuffer[_bufferPos + remaining_len] = '\0';

                _serial.print(matches[0] + current_len);
                _bufferPos += remaining_len;
            } else {
                 _serial.write('\a'); /* Not enough space */
            }
        } else {
            /* LCP is same as current input: list options */
            _serial.println(); /* Newline before listing */
            for (int i = 0; i < match_count; i++) {
                _serial.print(matches[i]);
                _serial.print("  "); /* Add some spacing */
            }
            /* Reprint prompt and current buffer */
            _printPrompt();
            _serial.print(_lineBuffer);
        }
    }
}


/* --- Help Command Helper --- */
/* Can be called from the user-defined help command handler */
void ArduinoCLI::printHelp() {
    _serial.println(F("Available commands:"));
    for (size_t i = 0; i < _commandCount; i++) {
         if (_commands[i].name == NULL) continue;
        _serial.print(F("  "));
        _serial.print(_commands[i].name);
        /* Basic padding attempt */
        int nameLen = (int)strlen(_commands[i].name);
        int padding = 15 - nameLen;
        if (padding < 1) padding = 1;
        for (int pad = 0; pad < padding; pad++) {
             _serial.print(' ');
        }
        _serial.print(F("- "));
        _serial.print(_commands[i].help_text ? _commands[i].help_text : "");
        _serial.print(F(" (max args: "));
        _serial.print(_commands[i].max_args);
        _serial.println(F(")"));
    }
}
