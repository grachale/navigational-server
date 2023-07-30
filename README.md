# Multithreaded server in C++
This repository contains an autonomous robot control server written in c++ designed to remotely manage a fleet of robots. The robots are capable of independently connecting to the server, which then guides them towards the center of a coordinate system. As part of the testing process, each robot starts at random coordinates and attempts to reach the target coordinates [0,0]. Upon reaching the destination, the robot must retrieve a valuable secret.

Communication between the server and robots is fully realized using a text-based protocol. Each command is terminated with a pair of special symbols "\a\b". (These are two characters '\a' and '\b'.) The server must strictly adhere to the communication protocol but should consider imperfect robot firmware (see the Special Situations section).

## Server Messages:
| Name                          | Message                                   | Description                                                                   |
|-------------------------------|-------------------------------------------|-------------------------------------------------------------------------------|
| SERVER_CONFIRMATION           | `<16-bit decimal number>\a\b`           | Message with a confirmation code. It can contain a maximum of 5 digits and the termination sequence \a\b. |
| SERVER_MOVE                   | 102 MOVE\a\b                            | Command to move one square forward.                                            |
| SERVER_TURN_LEFT              | 103 TURN LEFT\a\b                       | Command to turn left.                                                         |
| SERVER_TURN_RIGHT             | 104 TURN RIGHT\a\b                      | Command to turn right.                                                        |
| SERVER_PICK_UP                | 105 GET MESSAGE\a\b                     | Command to pick up a message.                                                 |
| SERVER_LOGOUT                 | 106 LOGOUT\a\b                          | Command to terminate the connection after successfully retrieving a message. |
| SERVER_KEY_REQUEST            | 107 KEY REQUEST\a\b                     | Server's request for a Key ID for communication.                              |
| SERVER_OK                     | 200 OK\a\b                              | Affirmative acknowledgment.                                                   |
| SERVER_LOGIN_FAILED           | 300 LOGIN FAILED\a\b                    | Authentication failed.                                                        |
| SERVER_SYNTAX_ERROR           | 301 SYNTAX ERROR\a\b                    | Incorrect message syntax.                                                     |
| SERVER_LOGIC_ERROR            | 302 LOGIC ERROR\a\b                     | Message sent in the wrong situation.                                          |
| SERVER_KEY_OUT_OF_RANGE_ERROR | 303 KEY OUT OF RANGE\a\b                | Key ID is out of the expected range.  
