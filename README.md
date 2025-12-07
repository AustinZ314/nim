Austin Zhang asz43  
Amanda Lei al1458

## Implementation Notes:
- **Our server handles the extra credit**
- nimd accepts a port number as an argument and constantly listens for TCP connections to match players together
- fork() is used to handle concurrent games by spawning a child process for each active game
- I/O multiplexing via poll() is used in each game to listen for messages from both players simultaneously and handle them immediately (for the extra credit)
- poll() is also used in the main connection-accepting loop to monitor the listening socket and a waiting player simultaneously
- The file protocol.c handles parsing messages to detect the presentation errors defined in section 3.4.2 of the writeup, and detects invalid messages as early as possible
- A global linked list is used to keep track of all active games
- Player names are rejected if they include non-ASCII characters or control characters

## Testing Plan


### Description of Testing Files


#### Test 1
