// My libraries
#include <iostream>
#include <sstream>
#include <queue>
#include <string>
#include <algorithm>
#include <cmath>

#include <cstdlib>
#include <cstdio>
#include <sys/socket.h> // socket(), bind(), connect(), listen()
#include <unistd.h> // close(), read(), write()
#include <netinet/in.h> // struct sockaddr_in
#include <strings.h> // bzero()
//#include <wait.h> // waitpid()

using namespace std;

#define BUFFER_SIZE 10240
#define TIMEOUT 1
#define TIMEOUT_RECHARGING 5


const char SERVER_MOVE[]                    = "102 MOVE\a\b";
const char SERVER_TURN_LEFT[]               = "103 TURN LEFT\a\b";
const char SERVER_TURN_RIGHT[]              = "104 TURN RIGHT\a\b";
const char SERVER_PICK_UP[]                 = "105 GET MESSAGE\a\b";
const char SERVER_LOGOUT[]                  = "106 LOGOUT\a\b";
const char SERVER_KEY_REQUEST[]             = "107 KEY REQUEST\a\b";
const char SERVER_OK[]                      = "200 OK\a\b";

const char SERVER_LOGIN_FAILED[]            = "300 LOGIN FAILED\a\b";
const char SERVER_SYNTAX_ERROR[]            = "301 SYNTAX ERROR\a\b";
const char SERVER_LOGIC_ERROR[]             = "302 LOGIC ERROR\a\b";
const char SERVER_KEY_OUT_OF_RANGE_ERROR[]  = "303 KEY OUT OF RANGE\a\b";

enum Direction {
    UP = 1,
    RIGHT,
    DOWN,
    LEFT
};

void takeMessageFromQueue(queue<string> & queueOfMsgs, string & msgFromQueue, bool pop )
{
    msgFromQueue = queueOfMsgs . front () .substr(0, queueOfMsgs . front () . size() - 4);
    if ( pop )
        queueOfMsgs . pop ();
}

void getDirection ( Direction & directionOfRobot, const pair<int, int> & firstLocation, const pair<int, int> & secondLocation )
{
    int deltaX = firstLocation . first - secondLocation . first;
    int deltaY = firstLocation . second - secondLocation . second;

    if ( deltaY == -1 )
        directionOfRobot = UP;
    else if ( deltaY == 1 )
        directionOfRobot = DOWN;
    else if ( deltaX == -1 )
        directionOfRobot = RIGHT;
    else
        directionOfRobot = LEFT;

}

void transformIntoString ( char * bufferRead, char * transformed, int bytesRead, int & newLength )
{
    newLength = 0;
    for (int i = 0; i < bytesRead; i++) {
        if (bufferRead[i] == '\a') {
            transformed[newLength++] = '\\';
            transformed[newLength++] = 'a';
        } else if (bufferRead[i] == '\b') {
            transformed[newLength++] = '\\';
            transformed[newLength++] = 'b';
        } else if (bufferRead[i] == '\0') {
            transformed[newLength++] = '\\';
            transformed[newLength++] = '0';
        } else {
            transformed[newLength++] = bufferRead[i];
        }
    }
    transformed[newLength] = '\0';
}

bool getCoordinates ( string & msg, pair<int, int> & location )
{
    std::stringstream streamMsg (msg);
    std::string word;

    // Extract the "OK"
    streamMsg >> word;
//    if ( word != "OK")
//        return false;

    // Extract the x and y coordinates
    double x, y;
    streamMsg >> x >> y;
    int x1 = std::trunc(x);
    int y1 = std::trunc(y);
    if ( x != x1 || y != y1 )
        return false;

    location . first = x1;
    location . second = y1;
    return true;
}

bool isRecharging ( string & msg )
{
    if ( msg == "RECHARGING" )
        return true;
    else
        return false;
}

void splitToQueue (string & inputBuffer, string & storeBuffer, queue<std::string> & queueOfMsgs )
{
    storeBuffer += inputBuffer;
    string msg;

    size_t pos = storeBuffer . find("\\a\\b");
    while ( pos != string::npos )
    {
        msg = storeBuffer.substr(0, pos + 4);
        queueOfMsgs.push(msg);
        storeBuffer = storeBuffer.substr(pos + 4);
        pos = storeBuffer.find("\\a\\b");
    }
}

int calculateHash(std::string & msg)
{
    int hashValue = 0;
    for ( int i = 0; i < msg . size (); ++i )
    {
        if ( msg[i] == '\\' && msg[i + 1] == 'a')
        {
            hashValue += static_cast<int>('\a');
            ++i;
        }
        else if ( msg[i] == '\\' && msg[i + 1] == 'b')
        {
            hashValue += static_cast<int>('\b');
            ++i;
        }
        else if ( msg[i] == '\\' && msg[i + 1] == '0')
            ++i;
        else
            hashValue += static_cast<int>(msg[i]);
    }

    return (hashValue * 1000) % 65536;
}

inline int sendMsg(int c, const char* error_msg) {
    if (send(c, error_msg, strlen(error_msg), 0) < 0)
    {
        close(c);
        return -3;
    }
    return 0;
}

void nextMove( int c, Direction & directionOfRobot, pair<int, int> & currentCoordinates, pair<int, int> & nextCoordinates, bool & indicator2 )
{
    Direction neededDirection;
    getDirection(neededDirection, currentCoordinates, nextCoordinates);

    if ( neededDirection != directionOfRobot )
    {
        sendMsg(c, SERVER_TURN_RIGHT);
        if ( directionOfRobot == 1 )
            directionOfRobot = RIGHT;
        else if ( directionOfRobot == 2 )
            directionOfRobot = DOWN;
        else if ( directionOfRobot == 3 )
            directionOfRobot = LEFT;
        else
            directionOfRobot = UP;
        indicator2 = false;
    } else {
        sendMsg( c, SERVER_MOVE);
        indicator2 = true;
    }
}

bool algorithmOfMovements ( pair<int, int> & firstLocation, pair<int, int> & locationOfRobot, Direction & directionOfRobot,  int c, int & indicator, bool & indicator2 )
{
    pair<int, int> nextLocation;

    if ( locationOfRobot . second > 0 && !indicator )
    {
        if ( locationOfRobot == firstLocation && indicator2 )
        {
            nextLocation . first = locationOfRobot . first - 1;
            nextLocation . second = locationOfRobot . second;
            nextMove(c, directionOfRobot, locationOfRobot, nextLocation, indicator2 );
            if ( !indicator2 )
                indicator2 = true;
        } else
        {
            nextLocation . first = locationOfRobot . first;
            nextLocation . second = locationOfRobot . second - 1;
            nextMove(c, directionOfRobot, locationOfRobot, nextLocation, indicator2 );
        }
    } else if ( locationOfRobot . second < 0 && !indicator )
    {
        if ( locationOfRobot == firstLocation && indicator2 )
        {
            nextLocation . first = locationOfRobot . first + 1;
            nextLocation . second = locationOfRobot . second;
            nextMove(c, directionOfRobot, locationOfRobot, nextLocation, indicator2 );
            if ( !indicator2 )
                indicator2 = true;
        } else
        {
            nextLocation . first = locationOfRobot . first;
            nextLocation . second = locationOfRobot . second + 1;
            nextMove(c, directionOfRobot, locationOfRobot, nextLocation, indicator2 );
        }

    } else
    {
        if ( locationOfRobot . first > 0 )
        {
            if ( ( locationOfRobot == firstLocation && indicator2 ) || indicator )
            {
                if ( indicator == 0 )
                {
                    sendMsg(c, SERVER_TURN_RIGHT);
                    ++indicator;
                } else if ( indicator == 1)
                {
                    sendMsg(c, SERVER_MOVE);
                    ++indicator;
                } else if ( indicator == 2 )
                {
                    sendMsg(c, SERVER_TURN_LEFT);
                    ++indicator;
                } else if ( indicator == 3 )
                {
                    sendMsg(c, SERVER_MOVE);
                    ++indicator;
                } else if ( indicator == 4 )
                {
                    sendMsg(c, SERVER_MOVE);
                    indicator = 0;
                }

            } else
            {
                nextLocation . first = locationOfRobot . first - 1;
                nextLocation . second = locationOfRobot . second;
                nextMove(c, directionOfRobot, locationOfRobot, nextLocation, indicator2 );
            }
        } else if ( locationOfRobot . first < 0 )
        {
            if ( ( locationOfRobot == firstLocation && indicator2 ) || indicator )
            {
                if ( indicator == 0 )
                {
                    sendMsg(c, SERVER_TURN_LEFT);
                    ++indicator;
                } else if ( indicator == 1)
                {
                    sendMsg(c, SERVER_MOVE);
                    ++indicator;
                } else if ( indicator == 2 )
                {
                    sendMsg(c, SERVER_TURN_RIGHT);
                    ++indicator;
                } else if ( indicator == 3 )
                {
                    sendMsg(c, SERVER_MOVE);
                    ++indicator;
                } else if ( indicator == 4 )
                {
                    sendMsg(c, SERVER_MOVE);
                    indicator = 0;
                }

            } else
            {
                nextLocation . first = locationOfRobot . first + 1;
                nextLocation . second = locationOfRobot . second;
                nextMove(c, directionOfRobot, locationOfRobot, nextLocation, indicator2 );
            }
        } else
        {
            return true;
        }
    }
    return false;
}


int main(int argc, char **argv) {

    pair<int,int> keys [5];
    keys [0] = make_pair(23019, 32037);
    keys [1] = make_pair(32037, 29295);
    keys [2] = make_pair(18789, 13603);
    keys [3] = make_pair(16443, 29533);
    keys [4] = make_pair(18189, 21952);

    if(argc < 2){
        cerr << "No server port!" << endl;
        return -1;
    }

    int l = socket(AF_INET, SOCK_STREAM, 0);
    if(l < 0){
        perror("Can not create socket: ");
        return -1;
    }

    int port = atoi(argv[1]);
    if(port == 0){
        cerr << "No server port!" << endl;
        close(l);
        return -1;
    }

    struct sockaddr_in adresa;
    bzero(&adresa, sizeof(adresa));
    adresa.sin_family = AF_INET;
    adresa.sin_port = htons(port);
    adresa.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(l, (struct sockaddr *) &adresa, sizeof(adresa)) < 0){
        perror("Problem with bind(): ");
        close(l);
        return -1;
    }

    if(listen(l, 10) < 0){
        perror("Problem with listen()!");
        close(l);
        return -1;
    }

    struct sockaddr_in vzdalena_adresa;
    socklen_t velikost;

    while(true){
        // Cekam na prichozi spojeni
        int c = accept(l, (struct sockaddr *) &vzdalena_adresa, &velikost);
        if(c < 0){
            perror("Problem s accept()!");
            close(l);
            return -1;
        }
        pid_t pid = fork();
        if(pid == 0){
            close(l);

            struct timeval timeout;
            timeout.tv_sec = TIMEOUT;
            timeout.tv_usec = 0;

            struct timeval timeoutRecharge;
            timeoutRecharge.tv_sec = TIMEOUT_RECHARGING;
            timeoutRecharge.tv_usec = 0;

            fd_set sockets;

            int retval;

            // buffers for msgs
            char bufferRead[BUFFER_SIZE];
            char transformed[BUFFER_SIZE];
            string bufferStore;
            queue<std::string> queueOfMsgs;

            int newLength;


            // Location of robots
            Direction directionOfRobot;
            pair<int, int> firstLocation;
            pair<int, int> locationOfRobot;

            // steps of authentication
            bool authentication = false;
            bool username = false;
            bool clientKeyId = false;
            bool clientConfirmation = false;
            bool gettingCoordinates = false;
            bool firstCoordinates = false;
            bool firstMovement = true;
            bool pickUp = false;
            bool rightMovement = false;

            int indicatorForMovement = 0;
            bool indicatorForMovement2 = false;

            bool recharging = false;

            // codes for authentication
            unsigned int clientId;
            int hashOfClientUsername = 0;

            while(true){
                FD_ZERO(&sockets);
                FD_SET(c, &sockets);

                retval = select(c + 1, &sockets, NULL, NULL, recharging ? &timeoutRecharge : &timeout);
                if(retval < 0){
                    perror("Error with select() ");
                    close(c);
                    return -1;
                }
                if(!FD_ISSET(c, &sockets)){
                    cout << "Connection timeout! " << endl;
                    close(c);
                    return 0;
                }
                int bytesRead = recv(c, bufferRead, BUFFER_SIZE - 1, 0);
                if(bytesRead <= 0){
                    perror("Error while reading from socket ");
                    close(c);
                    return -3;
                }


                if ( bytesRead > 100 || (bytesRead == 100 && bufferRead[99] != '\b' && bufferRead[98] != '\a') )
                {
                    sendMsg(c, SERVER_SYNTAX_ERROR);
                    break;
                }

                transformIntoString(bufferRead, transformed, bytesRead, newLength);
                string stringBufferRead (transformed, newLength);
                splitToQueue(stringBufferRead, bufferStore, queueOfMsgs);

                if ( queueOfMsgs . empty() && ((!username && bytesRead >= 20) ||
                (!clientConfirmation && bytesRead >= 7 ) ||
                (!firstCoordinates && bytesRead >= 12)) )
                {
                    sendMsg(c, SERVER_SYNTAX_ERROR);
                    break;
                }

                while ( !queueOfMsgs . empty () )
                {
                    if ( recharging )
                    {
                        string msg = queueOfMsgs . front();
                        queueOfMsgs . pop ();
                        if ( msg == "FULL POWER\\a\\b")
                        {
                            recharging = false;
                        }
                        else
                        {
                            recharging = false;
                            sendMsg(c, SERVER_LOGIC_ERROR);
                            break;
                        }
                    } else
                    {
                        if ( !authentication )
                        {
                            if ( !username )
                            {
                                string usernameMsg;
                                takeMessageFromQueue(queueOfMsgs, usernameMsg, true);
                                if ( usernameMsg . size () > 18 )
                                {
                                    sendMsg(c, SERVER_SYNTAX_ERROR);
                                    break;
                                }
                                sendMsg (c, SERVER_KEY_REQUEST);
                                hashOfClientUsername = calculateHash(usernameMsg);
                                username = true;

                            } else if ( !clientKeyId )
                            {
                                string msg;
                                takeMessageFromQueue(queueOfMsgs, msg, true);
                                if (isRecharging(msg))
                                {
                                    recharging = true;
                                    break;
                                }

                                if ( msg . size () > 1 )
                                {
                                    sendMsg(c, SERVER_SYNTAX_ERROR);
                                    break;
                                }

                                clientId = static_cast<unsigned int>(msg[0]) - 48;

                                if (clientId > 4 || clientId < 0)
                                {
                                    sendMsg(c, SERVER_KEY_OUT_OF_RANGE_ERROR);
                                    break;
                                }

                                int sendingCode = hashOfClientUsername + keys[clientId] . first;
                                sendingCode %= 65536;

                                string msgConfirmation (to_string(sendingCode));
                                msgConfirmation += "\a\b";

                                sendMsg(c, msgConfirmation . c_str());

                                clientKeyId = true;
                            } else if ( !clientConfirmation )
                            {
                                string msg;
                                takeMessageFromQueue(queueOfMsgs, msg, true);
                                if (isRecharging(msg))
                                {
                                    recharging = true;
                                    break;
                                }

                                if ( msg . size () > 5 || msg[msg. size() - 1] == ' ')
                                {
                                    sendMsg(c, SERVER_SYNTAX_ERROR);
                                    break;
                                }

                                int hashfOfClient = stoi(msg.substr(0, msg.size()));
                                hashfOfClient = ( ( hashfOfClient + 65536 ) - keys[clientId] . second ) % 65536;
                                if ( hashfOfClient == hashOfClientUsername )
                                {
                                    sendMsg(c, SERVER_OK);
                                    sendMsg(c, SERVER_MOVE);
                                    clientConfirmation = true;
                                    authentication = true;
                                } else
                                {
                                    sendMsg(c, SERVER_LOGIN_FAILED);
                                    break;
                                }
                            }
                        } else
                        {
                            if ( !gettingCoordinates )
                            {
                                if ( !firstCoordinates )
                                {
                                    string msg;
                                    takeMessageFromQueue(queueOfMsgs, msg, true);
                                    if ( msg[msg.size() - 1] == ' ')
                                    {
                                        sendMsg(c, SERVER_SYNTAX_ERROR);
                                        break;
                                    }
                                    if (isRecharging(msg))
                                    {
                                        recharging = true;
                                        break;
                                    }

                                    if ( !getCoordinates(msg, firstLocation) )
                                    {
                                        sendMsg(c, SERVER_SYNTAX_ERROR);
                                        break;
                                    }

                                    if ( firstLocation == make_pair(0,0) )
                                    {
                                        sendMsg (c, SERVER_PICK_UP);
                                        firstCoordinates = true;
                                        gettingCoordinates = true;
                                        pickUp = true;
                                    } else
                                    {
                                        sendMsg(c, SERVER_MOVE);
                                        firstCoordinates = true;
                                    }

                                } else
                                {
                                    string msg;
                                    takeMessageFromQueue(queueOfMsgs, msg, false);

                                    if (isRecharging(msg))
                                    {
                                        queueOfMsgs . pop ();
                                        recharging = true;
                                        break;
                                    }

                                    if ( !getCoordinates(msg, locationOfRobot) )
                                    {
                                        queueOfMsgs . pop ();
                                        sendMsg(c, SERVER_SYNTAX_ERROR);
                                        break;
                                    }

                                    if ( locationOfRobot == firstLocation && !rightMovement )
                                    {
                                        sendMsg(c, SERVER_TURN_RIGHT);
                                        queueOfMsgs . pop ();
                                        rightMovement = true;
                                    } else if ( locationOfRobot == firstLocation && rightMovement )
                                    {
                                        sendMsg(c, SERVER_MOVE);
                                        queueOfMsgs . pop ();
                                        rightMovement = false;
                                    } else
                                    {
                                        getDirection( directionOfRobot, firstLocation, locationOfRobot );
                                        gettingCoordinates = true;
                                    }
                                }
                            } else {
                                if ( !pickUp )
                                {
                                    if ( firstMovement )
                                    {
                                        getDirection(directionOfRobot, firstLocation, locationOfRobot);
                                        queueOfMsgs . pop ();
                                        indicatorForMovement2 = false;
                                        firstMovement = false;
                                    } else
                                    {
                                        string msg;
                                        takeMessageFromQueue(queueOfMsgs, msg, true);

                                        if (isRecharging(msg))
                                        {
                                            recharging = true;
                                            break;
                                        }

                                        firstLocation = locationOfRobot;
                                        if ( !getCoordinates(msg, locationOfRobot) )
                                        {
                                            sendMsg(c, SERVER_SYNTAX_ERROR);
                                            break;
                                        }
                                    }

                                    if ( algorithmOfMovements( firstLocation, locationOfRobot, directionOfRobot, c, indicatorForMovement, indicatorForMovement2 ) )
                                    {
                                        // take message
                                        sendMsg(c, SERVER_PICK_UP);
                                        pickUp = true;
                                    }

                                } else
                                {
                                    string msg;
                                    takeMessageFromQueue(queueOfMsgs, msg, true);
                                    if (isRecharging(msg))
                                    {
                                        recharging = true;
                                        break;
                                    }
                                    if ( msg . size () > 98 )
                                    {
                                        sendMsg(c, SERVER_SYNTAX_ERROR);
                                        break;
                                    }
                                    sendMsg(c, SERVER_LOGOUT);
                                    close(c);
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
            close(c);
            return 0;
        }

        int status = 0;
        waitpid(0, &status, WNOHANG);

        close(c);
    }

    close(l);
    return 0;
}
