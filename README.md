Austin Zhang asz43  
Amanda Lei al1458

# Implementation Notes:
- **Our server handles the extra credit**
- nimd accepts a port number as an argument and uses a while loop to constantly listen for TCP connections to match players together
- fork() is used to handle concurrent games by spawning a child process for each active game
- I/O multiplexing via poll() is used in each game to listen for messages from both players simultaneously and handle them immediately (for the extra credit)
- poll() is also used in the main connection-accepting loop to monitor the listening socket and a waiting player simultaneously
- The file protocol.c handles parsing messages to detect the presentation errors defined in section 3.4.2 of the writeup, and detects invalid messages as early as possible
- A global linked list is used to keep track of all active games
- Player names are rejected if they include non-ASCII characters or control characters

# Testing Plan
Our test plan focuses on validating the correctness and robustness of nimd. Our tests check if our program handles player connections properly, manages concurrent games, and sends errors in the right cases.

### Description of Testing Files
All of the programs that we use to test nimd are in the `clients/` directory, which was provided for testing purposes.
- rawc
- testc

### **TESTC Tests**

### **Test 1 — Basic Gameplay**

**Requirement:** Server must correctly handle normal gameplay.
**Description:** Run `./testc localhost 9000 Alice` and `./testc localhost 9000 Bob`. They match into a game. Attempts to move out of turn result in “Shh.” Invalid numeric input is rejected by testc and reprompted.
**Result:** Game proceeds normally. Turns alternate correctly. Game ends with “You win!” and “You are defeated.”

### **Test 2 — Parallel Games**

**Requirement:** Server must support multiple concurrent games.
**Description:** While Alice/Bob is running, start `./testc localhost 9000 Amanda` and `./testc localhost 9000 Austin`.
**Result:** Server creates a second independent game. No cross-mixing of messages. Both games run normally to completion.

### **Test 3 — Player Already Playing**

**Requirement:** Second OPEN request from same user must fail with code 22.
**Description:** While Alice is already in a game, run `./testc localhost 9000 Alice`.
**Result:** testc displays an unexpected FAIL caused by `FAIL|22 Already Playing`, which is correct.

### **Test 4 — Forfeits**

**Requirement:** Disconnecting should forfeit the game.
**Description:** Close Alice’s testc window during her game with Bob.
**Result:** Bob receives “Alice forfeits” and final board state. Server cleans up the game.

### **RAWC Tests**

### **Test 5 — Normal Connection & Moves (rawc)**

**Requirement:** Server must accept valid OPEN, NAME, PLAY, and MOVE.
**Description:**
Send `0|12|OPEN|Amanda|` and `0|12|OPEN|Austin|` in two rawc terminals. Then send a valid move such as `0|09|MOVE|2|3|`.
**Result:** Server processes all messages correctly.

### **Test 6 — MOVE Out of Turn → FAIL 31**

**Requirement:** Sending MOVE during opponent’s turn should fail without closing connection.
**Description:** Send `MOVE` before receiving a PLAY message for that player.
**Result:** Server returns `FAIL|31 Impatient|` and keeps the connection open.

### **Test 7 — Invalid Message Format → FAIL 10**

**Requirement:** Server must detect malformed messages.
**Description:** Send any incorrectly formatted NGP frame.
**Result:** Server returns `FAIL|10 Invalid|` and closes connection.

### **Test 8 — Long Player Name → FAIL 21**

**Requirement:** Names longer than 72 characters must be rejected.
**Description:** Send an OPEN with 73 'A's.
**Result:** Server returns `FAIL|21 Long Name|` and closes connection.

### **Test 9 — Second OPEN → FAIL 23**

**Requirement:** Client cannot send OPEN twice.
**Description:** Send `OPEN|Alice|` and then send another OPEN.
**Result:** Server returns `FAIL|23 Already Open|` and closes connection.

### **Test 10 — MOVE Before NAME → FAIL 24**

**Requirement:** MOVE cannot be sent before NAME.
**Description:** Send `OPEN|Alice|` then immediately send a MOVE.
**Result:** Server returns `FAIL|24 Not Playing|` and closes connection.

### **Test 11 — Invalid Stone Quantity → FAIL 33**

**Requirement:** Removing too many stones (or invalid amount) must be rejected.
**Description:** With board `1 3 1 7 9`, send `MOVE|2|4|`.
**Result:** Server returns `FAIL|33 Quantity|` and keeps the connection open.

### **Test 12 — Invalid Pile Index → FAIL 32**

**Requirement:** Pile index must be 1–5.
**Description:** Send a move selecting an out-of-range pile.
**Result:** Server returns `FAIL|32 Pile Index|` and keeps the connection open.

### **Summary**

All core requirements (turn order, board updates, game termination, protocol validation, and every error code) were successfully tested. The server’s behavior matches the NGP specification and handles both valid and invalid client behavior correctly.
