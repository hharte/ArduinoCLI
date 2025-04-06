# ArduinoCLI Library

A flexible command-line interface (CLI) framework for Arduino, designed to work with any Arduino `Stream` object (like `Serial`, `SoftwareSerial`, etc.).


## Overview

This library provides the structure to create interactive text-based command interfaces for your Arduino projects. It handles common CLI tasks such as reading input, parsing commands and arguments, command matching with prefix support, tab completion, argument validation, and executing user-defined functions for each command.


## Features



* **Stream-Based:** Works with any Arduino `Stream` object for input/output.
* **Command Structure:** Uses a simple struct (`CLI_Command_t`) to define commands, including name, handler function, maximum arguments, and help text.
* **<code>argc</code>/<code>argv</code> Style:** Parses input lines and passes arguments to handler functions in a standard `argc`/`argv` format.
* **Argument Validation:** Checks the number of provided arguments against the maximum allowed for each command (`max_args`).
* **Command Matching:**
    * Finds commands based on typed prefixes.
    * Prefers exact matches.
    * Handles ambiguity detection.
* **Tab Completion:**
    * Attempts command name completion when the Tab key is pressed.
    * Completes unique matches inline (terminal behavior dependent) and adds a space.
    * Completes to the Longest Common Prefix (LCP) for multiple matches.
    * Lists possible matches if the typed prefix is already the LCP.
* **Input Handling:**
    * Reads input character-by-character.
    * Handles standard line endings (`\r`, `\n`, `\r\n`).
    * Supports Backspace/Delete (attempts visual feedback).
    * Basic Ctrl+C handling (clears line, reprints prompt).
* **Formatted Output:** Inserts newlines before prompts, command execution, and error messages for readability.
* **Configurable:** Allows setting the prompt string, maximum line length, and maximum argument count.
* **Dynamic Memory:** Uses `malloc`/`free` for input buffers (be mindful of RAM on constrained devices).
* **Documentation Ready:** Designed with Doxygen comments in the header file.


## Installation



1. **Library Manager:** (Not available via Library Manager yet)
2. **Manual Installation:**
    * Download the library files (`ArduinoCLI.h`, `ArduinoCLI.cpp`, `keywords.txt`, `library.properties`).
    * Place the `ArduinoCLI` folder into your Arduino `libraries` directory (usually found in `Documents/Arduino/libraries`).
    * Restart the Arduino IDE.


## Basic Usage



1. **Include Header:**

    ```
    #include <ArduinoCLI.h>
    ```


1. **Define Command Handlers:** Create functions for each command that match the `cli_command_handler_t` signature:

    ```
    // Example handler void myCommandHandler(ArduinoCLI* cli, int argc, char *argv[]) { // Access arguments via argv[0], argv[1], ... // Access Serial via cli->getSerial() // Implement command logic... cli->getSerial().println("Command executed!"); }
    ```


1. **Define Command Table:** Create an array of `CLI_Command_t` structs:

    ```
    CLI_Command_t myCommands[] = { { "commandName", myCommandHandler, /*max_args*/ 2, "Help text for command" }, { "help", helpHandler, 0, "Displays help" }, // ... other commands }; size_t myCommandCount = sizeof(myCommands) / sizeof(myCommands[0]);
    ```


1. **Instantiate ArduinoCLI:** Create an instance, passing the `Stream`, command table, and count:

    ```
    ArduinoCLI myCli(Serial, myCommands, myCommandCount);
    ```


1. **Setup:** Initialize your `Stream` (e.g., `Serial.begin(...)`) and start the CLI in `setup()`:

    ```
    void setup() { Serial.begin(115200); // ... other setup myCli.start(); // Prints the first prompt }
    ```


1. **Loop:** Call the `poll()` method repeatedly in your main `loop()`:

    ```
    void loop() { if (myCli.isRunning()) { myCli.poll(); } // ... other loop code }

    ```



## API Reference


### Constants



* `CLI_DEFAULT_MAX_LINE_LEN` (64): Default maximum input line length.
* `CLI_DEFAULT_MAX_ARGS` (8): Default maximum number of arguments (excluding command name).
* `CLI_DEFAULT_PROMPT` ("> "): Default command prompt string.
* `CLI_MAX_PROMPT_LEN` (18): Maximum allowed length for the prompt string.


### Types


```


#### cli_command_handler_t
```


Function pointer type for command handler functions.

**Signature:**


```
    void your_command_handler(ArduinoCLI* cli, int argc, char *argv[]);
```


**Parameters:**



* `cli`: Pointer to the `ArduinoCLI` instance.
* `argc`: Argument count (including command name).
* `argv`: Argument vector (array of C-strings). `argv[0]` is the command name.


```


#### CLI_Command_t
```


Structure defining a command for the CLI.

**Members:**



* `name` (const char*): Command keyword string.
* `func` (cli_command_handler_t): Function pointer to the command handler.
* `max_args` (int): Maximum number of *user-provided* arguments allowed (0 for none).
* `help_text` (const char*): Brief description of the command for help output.


### Class: `ArduinoCLI`


#### Public Methods


##### `ArduinoCLI()` (Constructor)

Creates an instance of the CLI handler.


```
    ArduinoCLI(Stream& serialPort, const CLI_Command_t commands[], size_t commandCount);
```


**Parameters:**



* `serialPort`: Reference to the `Stream` object (e.g., `Serial`).
* `commands`: Pointer to an array of `CLI_Command_t` structures.
* `commandCount`: The number of commands in the `commands` array.


```


##### start()
```


Starts or restarts CLI processing and prints the initial prompt.


```
    void start();


##### poll()
```


Polls the associated Stream for input. Call this repeatedly in the main `loop()`.


```
    void poll();


##### processInput()
```


Processes a single, complete line of input.


```
    void processInput(char* line);
```


**Parameters:**



* `line`: A null-terminated C-string containing the command line input.


```


##### stop()
```


Stops the CLI from processing further input via `poll()`.


```
    void stop();


##### isRunning()
```


Checks if the CLI is currently in a running state.


```
    bool isRunning() const;
```


**Returns:**



* `true` if the CLI is processing input, `false` otherwise.


```


##### getSerial()
```


Gets a reference to the `Stream` object used by the CLI instance.


```
    Stream& getSerial();
```


**Returns:**



* Reference to the configured `Stream` object.


```


##### printHelp()
```


Helper function to print the list of available commands based on the registered command table. Intended to be called from a user-defined 'help' command handler.


```
    void printHelp();


##### setPrompt()
```


Sets a custom command prompt string.


```
    void setPrompt(const char* prompt);
```


**Parameters:**



* `prompt`: The desired prompt string (up to `CLI_MAX_PROMPT_LEN` - 1 characters).


```


##### setMaxLineLen()
```


Sets the maximum length of the internal line buffer. Call before `start()` if changing from the default.


```
    void setMaxLineLen(size_t len);
```


**Parameters:**



* `len`: The desired maximum length (must be > 0).


```


##### setMaxArgs()
```


Sets the maximum number of arguments (including the command name) the parser will handle. Call before `start()` if changing from the default.


```
    void setMaxArgs(size_t num);
```


**Parameters:**



* `num`: The desired maximum number of arguments (must be > 0).


## Terminal Compatibility Notes



* **Tab Completion & Backspace:** The visual appearance of inline tab completion and backspace character deletion depends heavily on the capabilities of the terminal emulator connected to the Arduino (e.g., Arduino IDE Serial Monitor, PuTTY, Tera Term, etc.). The library sends standard control codes, but not all terminals render them effectively. Listing options on ambiguous tab completion should work reliably.


## Memory Usage

This library uses dynamic memory allocation (`malloc`/`free`) for its internal input line buffer and argument vector (`argv`). The amount of memory used depends on the `maxLineLen` and `maxArgs` settings. Be mindful of these settings on memory-constrained devices like the Arduino Uno.


## License

This library is released under the[ MIT License](https://opensource.org/licenses/MIT).


## Contributing

Contributions are welcome!
