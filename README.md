Austin Zhang asz43  
Amanda Lei al1458

## Implementation Notes:
- **Our server handles the extra credit** 
- nimd accepts a port number as an argument and uses a while loop to constantly accept incoming TCP connections from players
- I/O multiplexing via poll() is used in each game to listen for messages from both players simultaneously and handle them immediately (for the extra credit)
- poll() is also used in the main connection-accepting loop to monitor the listening socket and a waiting player simultaneously
- fork() is used to handle concurrent games by spawning a child process for each active game
- protocol.c handles parsing NGP messages to detect the presentation errors defined in section 3.4.2 of the writeup, and detects invalid messages as early as possible
- A global linked list is used to keep track of all active games and ensure that all player names are unique
- Player names are rejected if they include non-ASCII characters or control characters

## Testing Plan
Our test plan focuses on validating the correctness and robustness of nimd. Our tests check if our program handles player connections properly, manages concurrent games, and sends errors in the right cases.

### Description of Testing Files
All of the programs that we use to test nimd are in the provided `clients/` directory. This includes:
- `testc`, a basic client that prevents the user from making illegal moves
    - Usage: testc <host> <port> <name>
- `rawc`, a client that allows the user to send arbitrary strings to the server to test error handling
    - Usage: rawc <host> <port>

### **TESTC Tests**

#### **Test 1**

**Requirement:** nimd must support multiple concurrent games.  
**Detection Method:** If `./clients/testc` is run from four separate terminals, then two pairs of players should be able to play simultaneously without impacting one another (message flows should be the same as in Test 1, for each game).  
**Test:** Run `./nimd 9000`. Then start `./clients/testc localhost 9000 Alice`, `./clients/testc localhost 9000 Bob`, `./clients/testc localhost 9000 Amanda` and `./clients/testc localhost 9000 Austin`. nimd should create two independent games. Verify that both games can function independently and end correctly.  

#### **Test 2**

**Requirement:** nimd handles clients disconnecting properly.  
**Detection Method:** If a client disconnects with the keyboard interrupt Ctrl+C before it is placed into a game, the server should drop it and not consider it for matching when other clients connect (the next player to connect will have to wait for a new opponent). If a client disconnects during a game, the server will send the other player a message indicating that their opponent forfeited.  
**Test:** Run `./clients/testc 9000 Alice` and then enter the keyboard interrupt Ctrl+C. Then run `./clients/testc Bob` and `./clients/testc Charlie`. Bob and Charlie should be matched into a game, meaning that nimd handled Alice's disconnect. Then enter the keyboard interrupt Ctrl+C on Bob's client. The remaining player should receive a message saying that their opponent forfeited.  

### **RAWC Tests**

#### **Test 3**

**Requirement:** nimd should handle normal gameplay correctly.  
**Detection Method:** If `./clients/rawc` is run from two separate terminals, they should be matched into a game. nimd should send both clients a NAME message followed by a PLAY message. If players alternate sending MOVE to the server, the server should send OVER to the clients once the board is empty.  
**Test:** Run `./nimd 9000`. Then run `./clients/rawc localhost 9000` twice and send `0|11|OPEN|Alice|` and `0|09|OPEN|Bob|`. They should match into a game and should each receive NAME and PLAY messages. Then alternate valid moves until the board is empty and check that both clients receive OVER.  

#### **Test 4**

**Requirement:** nimd should detect and handle message framing errors.  
**Detection Method:** If the client sends a message with an invalid version number, message length, type, or fields (including too few or too many bytes, and vertical bars), then the server should respond with a 10 Invalid error in a FAIL message.  
**Test:** Run `./clients/rawc localhost 9000` several times. For each run, send one of the following invalid OPEN messages. The server should returns a FAIL 10 Invalid message and close the connection.
- 1|11|OPEN|Alice| (invalid version number)
- 0|9|OPEN|Bob| (message length isn't two decimal digits)
- 0|04|OPEN|Bob| (message length too small)
- 0|11|TPEN|Alice| (invalid message type)
- 0|11|OPEN|Bob| (message shorter than length)
- 0|09|OPEN|Alice| (message longer than length)

#### **Test 5**

**Requirement:** nimd should reject invalid names.  
**Detection Method:** If the client sends a name that includes a vertical bar, the server should respond with a FAIL 10 Invalid message. If the client sends a name that exceeds 72 bytes, the server should respond with a FAIL 21 Long Name message. In both cases, the server should also close the connection.  
**Test:** Run `./clients/rawc localhost 9000` twice. The first time, send the message `0|10|OPEN|Bo|b|` and verify that the server sends a FAIL 10 Invalid message and closes the connection. The second time, send the message `0|79|OPEN|a123456789b123456789c123456789d123456789e123456789f123456789g123456789h12|` (5 bytes for OPEN|, 73 bytes for the name, 1 byte for |). Verify that the server sends a FAIL 21 Long Name message and closes the connection.  

#### **Test 6**

**Requirement:** nimd should not allow clients to have duplicate names.  
**Detection Method:** If a client sends an OPEN message with a name that is already being used in an active game, then nimd should send a FAIL message with the 22 Already Playing error and close the connection.  
**Test:** Run `./clients/rawc localhost 9000` once with `0|11|OPEN|Alice|` and another time with `0|09|OPEN|Bob|`. The two players should be matched into a game with each other. Then run `rawc` again with `0|11|OPEN|Alice|`. nimd should respond with a FAIL message that specifies error 22 Already Playing.  

#### **Test 7**

**Requirement:** Client cannot send OPEN more than once.  
**Detection Method:** If a client sends an OPEN message after receiving WAIT (meaning that it has already sent OPEN once successfully and is waiting to be placed into a game), then nimd should send a FAIL message with the error 23 Already Open and close the connection.  
**Test:** Run `./clients/rawc localhost 9000` and send `0|11|OPEN|Alice|`. The server should send a WAIT message. Then send `0|11|OPEN|Alice|` again and confirm that the server responds with a FAIL 23 Already Open message and closes the connection.  

#### **Test 8**

**Requirement:** Client cannot send MOVE before receiving NAME.  
**Detection Method:** If a client sends MOVE while it is waiting to be matched into a game (after sending OPEN), then the server should respond with FAIL and the message code 24 Not Playing before closing the connection.  
**Test:** Run `./clients/rawc localhost 9000` and send `0|11|OPEN|Alice|`. The server should respond with a WAIT message. Then send `0|09|MOVE|1|1|`. Verify that the server responds with a FAIL 24 Not Playing message and closes the connection.  

#### **Test 9**

**Requirement:** nimd should detect and immediately handle when a client sends MOVE out of turn.  
**Detection Metohd:** If two players are matched into a game and Player 2 sends the first move (when it's Player 1's turn), then the server should immediately respond with a FAIL 31 Impatient message and keep the connection open.  
**Test:** Run `./clients/rawc localhost 9000` from two clients and send valid OPEN messages. The server should match the two players into a game and send NAME and PLAY messages. Then send `0|09|MOVE|1|1|` from Player 2. Check that the server responds with a FAIL 31 Impatient message and keeps the connection open.  

#### **Test 10**

**Requirement:** nimd should reject moves with invalid pile indices.  
**Detection Method:** If a player sends a move on their turn but chooses an index outside of 0-4, the server should respond with a FAIL 32 Pile Index message and keep the connection open.  
**Test:** Run `./clients/rawc localhost 9000` from two clients and send valid OPEN messages. The server should match the two players into a game. Then have Player 1 send `0|09|MOVE|9|1|`. Verify that the server returns a FAIL 32 Pile Index message and keeps the connection open.  

#### **Test 11**

**Requirement:** nimd should reject moves with invalid amounts of stones.  
**Detection Method:** If a player sends a move on their turn with a valid pile index but tries to remove more stones than the number in the pile, the server should respond with a FAIL 33 Quantity message and keep the connection open.  
**Test:** Run `./clients/rawc localhost 9000` from two clients and send valid OPEN messages. The server should match the two players into a game. Then, with the board as `1 3 5 7 9`, have Player 1 send `0|09|MOVE|1|9|`. Confirm that the server responds with a FAIL 33 Quantity message and keeps the connection open.  

#### **Test 12**

**Requirement:** nimd should properly handle clients sending messages in several parts and sending several messages at once.  
**Detection Method:** If a client sends two valid NGP messages all at once (all in one line, with no pause in between), the server should process them as two separate messages. If a client sends a valid NGP message in two halves (with a short pause between each half), the server should still process the message as a valid message.  
**Test:** Run `./clients/rawc localhost 9000` and send `0|11|OPEN|Alice|0|09|MOVE|1|1|` (two validly formatted NGP messages). Verify that the server responds with a WAIT message (for the OPEN) and then a FAIL 24 Not Playing message (for the MOVE). Run `./clients/rawc localhost 9000` again and send `0|11|`. Then wait a second before sending `OPEN|Alice|`. Check that the server responds with a WAIT message, meaning that the broken up message was processed successfully.  