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
 * \file ArduinoCLI.h
 * \brief Defines the ArduinoCLI class for creating command-line interfaces.
 */
#ifndef ArduinoCLI_h
#define ArduinoCLI_h

#include <Arduino.h>
#include <stddef.h> // For size_t

/* Default configuration values */
#define CLI_DEFAULT_MAX_LINE_LEN 64 /**< Default maximum input line length. */
#define CLI_DEFAULT_MAX_ARGS 8      /**< Default maximum number of arguments (excluding command name). */
#define CLI_DEFAULT_PROMPT "> "     /**< Default command prompt string. */
#define CLI_MAX_PROMPT_LEN 18       /**< Maximum allowed length for the prompt string. */

/* Forward declaration */
class ArduinoCLI;

/**
 * @brief Function pointer type for command handler functions.
 * @param cli Pointer to the ArduinoCLI instance (allows access to Serial etc.).
 * @param argc Argument count (including command name).
 * @param argv Argument vector (array of strings). argv[0] is the command name.
 */
typedef void (*cli_command_handler_t)(ArduinoCLI* cli, int argc, char *argv[]);

/**
 * @brief Structure defining a command for the CLI.
 */
typedef struct {
    const char *name;            /**< Command keyword (string). */
    cli_command_handler_t func;  /**< Function pointer to the command handler. */
    int max_args;                /**< Maximum number of user-provided arguments allowed (0 for none). */
    const char *help_text;       /**< Brief description of the command for help output. */
} CLI_Command_t;

/**
 * @class ArduinoCLI
 * @brief Provides a command-line interface framework for Arduino using Stream objects.
 *
 * This class handles reading input, parsing commands and arguments, matching commands
 * (including partial matches and tab completion attempts), validating argument counts,
 * and executing corresponding handler functions.
 */
class ArduinoCLI {
public:
    /**
     * @brief Constructor for the ArduinoCLI class.
     * @param serialPort Reference to the Stream object used for input/output (e.g., Serial).
     * @param commands Pointer to an array of CLI_Command_t structures defining the available commands.
     * @param commandCount The number of commands in the commands array.
     */
    ArduinoCLI(Stream& serialPort, const CLI_Command_t commands[], size_t commandCount);

    /**
     * @brief Sets the maximum length of the internal line buffer.
     * @param len The desired maximum length (must be > 0).
     * @note Does not reallocate buffers if already allocated by constructor or start. Call before first use.
     */
    void setMaxLineLen(size_t len);

    /**
     * @brief Sets the maximum number of arguments (including the command name) the parser will handle.
     * @param num The desired maximum number of arguments (must be > 0).
     * @note Does not reallocate buffers if already allocated by constructor or start. Call before first use.
     */
    void setMaxArgs(size_t num);

    /**
     * @brief Sets a custom command prompt string.
     * @param prompt The desired prompt string (max length CLI_MAX_PROMPT_LEN).
     */
    void setPrompt(const char* prompt);

    /**
     * @brief Starts or restarts CLI processing.
     * Sets the running flag and prints the initial command prompt.
     */
    void start();

    /**
     * @brief Polls the associated Stream for input.
     * This should be called repeatedly in the main Arduino loop().
     * It reads available characters, handles line endings, backspace, tab completion attempts,
     * and calls processInput() when a full line is received.
     */
    void poll();

    /**
     * @brief Processes a single, complete line of input.
     * Parses the line into command and arguments and executes the matched command.
     * Can be used as an alternative to poll() if input lines are obtained externally.
     * @param line A null-terminated character array containing the command line input.
     */
    void processInput(char* line);

    /**
     * @brief Checks if the CLI is currently in a running state.
     * The state is set to false by the stop() method (typically called by an 'exit' command).
     * @return true if the CLI is processing input, false otherwise.
     */
    bool isRunning() const;

    /**
     * @brief Gets a reference to the Stream object used by the CLI instance.
     * Useful for command handlers that need to perform direct input/output.
     * @return Reference to the configured Stream object (e.g., Serial).
     */
    Stream& getSerial();

    /**
     * @brief Helper function to print the list of available commands.
     * Iterates through the registered command table and prints names, help text, and max arguments.
     * Designed to be called from a user-defined 'help' command handler.
     */
    void printHelp();

    /**
     * @brief Stops the CLI from processing further input via poll().
     * Typically called by an 'exit' or 'quit' command handler.
     */
    void stop();


private:
    Stream& _serial;             /**< Reference to the Stream object (e.g., Serial). */
    const CLI_Command_t* _commands; /**< Pointer to the user-provided command array. */
    size_t _commandCount;       /**< Number of commands in the _commands array. */
    bool _isRunning;            /**< Flag indicating if the CLI should process input. */

    char* _lineBuffer;          /**< Dynamically allocated input line buffer. */
    size_t _maxLineLen;         /**< Maximum size of the _lineBuffer. */
    size_t _bufferPos;          /**< Current position (index) in the _lineBuffer. */

    char** _argv;               /**< Dynamically allocated argument vector (array of char*). */
    size_t _maxArgs;            /**< Maximum size of the _argv array. */

    char _prompt[CLI_MAX_PROMPT_LEN]; /**< The current command prompt string. */

    /**
     * @brief Resets the input buffer position and clears its content.
     * @private
     */
    void _resetBuffer();

    /**
     * @brief Prints the command prompt, preceded by a newline.
     * @private
     */
    void _printPrompt();

    /**
     * @brief Splits a line into arguments (modifies the input line).
     * @param[in,out] line The input line string to be tokenized. Will be modified.
     * @param[out] argv_local The array to store argument pointers into.
     * @param[in] max_args_local The maximum number of arguments to store in argv_local.
     * @return The number of arguments found (argc).
     * @private
     */
    int _splitLine(char *line, char **argv_local, size_t max_args_local);

    /**
     * @brief Finds a command matching the given prefix. Handles exact matches preferentially.
     * @param[in] prefix The command name prefix to search for.
     * @return Pointer to the matched CLI_Command_t, or NULL if not found or ambiguous.
     * @private
     */
    const CLI_Command_t* _findCommand(const char *prefix);

    /**
     * @brief Parses and executes a command line stored in the line buffer.
     * Finds command, validates arguments, prints pre-command newline, calls handler, handles errors.
     * @param[in] line The completed command line string.
     * @private
     */
    void _parseAndExecute(char *line);

    /**
     * @brief Handles tab key press for command completion attempt.
     * Finds matches, attempts single completion or LCP completion, or lists options.
     * Relies on terminal behavior for visual correctness.
     * @private
     */
    void _handleTab();

    /**
     * @brief Finds the length of the Longest Common Prefix (LCP) among an array of strings.
     * @param[in] matches Array of C-string pointers.
     * @param[in] count Number of strings in the matches array.
     * @return The length of the LCP.
     * @private
     */
    int _findLcp(const char *matches[], int count);

    /**
     * @brief Writes a single character to the serial port (for echoing).
     * @param[in] c The character to write.
     * @private
     */
    void _printChar(char c);

    /**
     * @brief Allocates memory for internal line buffer and argv array.
     * @return true on success, false on allocation failure.
     * @private
     */
    bool _allocateBuffers();

    /**
     * @brief Frees memory allocated for internal buffers.
     * @private
     */
    void _freeBuffers();

};

#endif /* ArduinoCLI_h */
