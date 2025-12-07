Austin Zhang asz43  
Amanda Lei al1458

## Implementation Notes:
- **Our server handles the extra credit**
- nimd accepts a port number as an argument and uses a while loop to constantly listen for TCP connections to match players together
- fork() is used to handle concurrent games by spawning a child process for each active game
- I/O multiplexing via poll() is used in each game to listen for messages from both players simultaneously and handle them immediately (for the extra credit)
- poll() is also used in the main connection-accepting loop to monitor the listening socket and a waiting player simultaneously
- The file protocol.c handles parsing messages to detect the presentation errors defined in section 3.4.2 of the writeup, and detects invalid messages as early as possible
- A global linked list is used to keep track of all active games
- Player names are rejected if they include non-ASCII characters or control characters

## Testing Plan
Our test plan focuses on validating the correctness and robustness of nimd. Our tests check if our program handles player connections properly, manages concurrent games, and sends errors in the right cases.

### Description of Testing Files
All of the programs that we use to test nimd are in the `clients/` directory, which was provided for testing purposes.
- rawc
- testc

#### Test 1
