#include <ArduinoCLI.h>
#include <limits.h>

/* --- Command Handler Functions --- */

void cmd_help_handler(ArduinoCLI* cli, int argc, char *argv[]) {
    (void)argc; /* Unused */
    (void)argv; /* Unused */
    cli->printHelp(); /* Use the library's help printer via the passed pointer */
}

void cmd_exit_handler(ArduinoCLI* cli, int argc, char *argv[]) {
    (void)argc; /* Unused */
    (void)argv; /* Unused */
    cli->getSerial().println(F("Stopping CLI processing."));
    cli->stop(); /* Tell the library to stop polling */
}

void cmd_greet_handler(ArduinoCLI* cli, int argc, char *argv[]) {
    Stream& serial = cli->getSerial(); /* Get serial port if needed */
    if (argc == 1) {
        serial.println(F("Hello there!"));
    } else if (argc == 2) {
        serial.print(F("Hello, "));
        serial.print(argv[1]);
        serial.println(F("!"));
    }
    /* Argument count validation is done by the library */
}

void cmd_add_handler(ArduinoCLI* cli, int argc, char *argv[]) {
     Stream& serial = cli->getSerial();
    if (argc < 2) {
        serial.print(F("Usage: "));
        serial.print(argv[0]); // argv[0] is the command name ("add")
        serial.println(F(" <num1> [num2] ..."));
        return;
    }
    long sum = 0;
    for (int i = 1; i < argc; i++) {
        char *endptr;
        /* Use strtol to convert to long integer */
        long val = strtol(argv[i], &endptr, 10);
        if (*endptr != '\0' || argv[i] == endptr) { /* Check conversion validity */
            serial.print(F("Error: Argument '"));
            serial.print(argv[i]);
            serial.println(F("' is not a valid integer."));
            return;
        }
        /* Check for potential overflow/underflow before adding */
        if (val > 0 && sum > (LONG_MAX - val)) {
             serial.println(F("Error: Potential integer overflow detected."));
             return;
        }
        if (val < 0 && sum < (LONG_MIN - val)) {
             serial.println(F("Error: Potential integer underflow detected."));
             return;
        }
        sum += val;
    }
    serial.print(F("Sum: "));
    /* Use %ld format specifier for long */
    char sumBuffer[15]; // Buffer size for long + sign + null
    snprintf(sumBuffer, sizeof(sumBuffer), "%ld", sum);
    serial.println(sumBuffer);
}

void cmd_pin_handler(ArduinoCLI* cli, int argc, char *argv[]) {
    Stream& serial = cli->getSerial();
    if (argc != 3) {
        serial.print(F("Usage: "));
        serial.print(argv[0]);
        serial.println(F(" <pin_number> <0|1>"));
        return;
    }
    int pin = atoi(argv[1]);
    int value = atoi(argv[2]);

    if (value != 0 && value != 1) {
         serial.println(F("Error: Value must be 0 or 1."));
         return;
    }

    /* Basic check - add more specific board checks if needed */
    if (pin < 0) {
        serial.println(F("Error: Invalid pin number."));
        return;
    }

    serial.print(F("Setting pin "));
    serial.print(pin);
    serial.print(F(" to "));
    serial.println(value == 1 ? "HIGH" : "LOW");

    pinMode(pin, OUTPUT);
    digitalWrite(pin, value);
}

void cmd_gr_handler(ArduinoCLI* cli, int argc, char *argv[]) {
    (void)argc; // Unused
    (void)argv; // Unused
    Stream& serial = cli->getSerial();
    serial.println(F("Grrrr.....!"));
}


/* --- Command Table --- */
CLI_Command_t commands[] = {
    {"help", cmd_help_handler, 0, "Show this help message"},
    {"gr", cmd_gr_handler, 0, "Prints Grrrr.....!"},
    {"greet", cmd_greet_handler, 1, "Greets the user or a specific name"},
    {"add", cmd_add_handler, CLI_DEFAULT_MAX_ARGS-1, "Adds numbers together"},
    {"pin", cmd_pin_handler, 2, "Set digital pin to 0 or 1"},
    {"exit", cmd_exit_handler, 0, "Stop CLI processing"},
    {"quit", cmd_exit_handler, 0, "Alias for exit"},
};
const size_t commandCount = sizeof(commands) / sizeof(commands[0]);


/* --- Create CLI Instance (using 'my_cli') --- */
ArduinoCLI my_cli(Serial, commands, commandCount);


void setup() {
  Serial.begin(115200);
  while (!Serial); /* Wait for Serial connect */

  Serial.println(F("\r\n\n--- Arduino CLI Example ---"));
  Serial.println(F("Type 'help' for commands."));
  Serial.println(F("Press Tab for completion attempt."));

  /* Optional: Customize CLI settings BEFORE starting */
  my_cli.setPrompt("Arduino> ");
// my_cli.setMaxLineLen(128);
// my_cli.setMaxArgs(10);

  /* Start the CLI and print the first prompt */
  my_cli.start();

}

void loop() {
  /* Poll the CLI in the main loop */
  if (my_cli.isRunning()) {
      my_cli.poll();
  } else {
      /* CLI has been stopped by the 'exit' command */
      delay(1000);
  }

  /* Your other Arduino code can run here */
  // delay(10);
}
