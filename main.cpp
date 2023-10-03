//STEAM MUST ALREADY BE RUNNING IN THE BACKGROUND ON THE SERVER FOR THIS CODE TO WORK!!!
//This code is the C++ server-side of the GodotSteam networking example.
#include <iostream>
#include <chrono>
#include <thread>
#include "steam_h/steam_api.h"
#include "steam_h/isteamnetworkingsockets.h"
#include "steam_h/isteamnetworkingutils.h"
//#include "steam_h/isteamnetworkingmessages.h"

using namespace std;

class CallBackClass
{
    public:
        bool connected = false;
        HSteamNetConnection connectionToClient;
        HSteamListenSocket listenSocket;
        string thisServerName;
        uint64 thisServerSteamID;
        string thisServerSteamIDAsString;

        CallBackClass()
        {
            connectionToClient = 0;  //HSteamNetConnection is really just an unsigned int. Connections will never use the 0 value, so I set it to 0 so that we know no connection has been established
            thisServerName = SteamFriends()->GetPersonaName();  //remember our Steam account nickname from Steam client
            thisServerSteamID = SteamUser()->GetSteamID().ConvertToUint64();  //remember this server's Steam ID number
            thisServerSteamIDAsString = to_string(thisServerSteamID);
        }
    private:

        STEAM_CALLBACK( CallBackClass, ConnectionStatusChange, SteamNetConnectionStatusChangedCallback_t );  //This is a macro we have to use so that the Steam API will connect our ConnectionStatusChange() function below into the Steam API callbacks routines. In other words, this is how Steam knows to call our function when a connection comes our way or if a connection changes.
};

void CallBackClass::ConnectionStatusChange( SteamNetConnectionStatusChangedCallback_t* pCallback )  //This runs whenever this server receives a connection request or one of those connections change state
//You have to use this callback function EXACTLY with the function prototype defined here. You can change the name of CallBackClass and ConnectionStatusChange, but you must also change the STEAM_CALLBACK line above as well
{
    cout << "ConnectionStatusChange previous connection state: " << pCallback->m_eOldState << endl;
    cout << "ConnectionStatusChange callback executed status#: " << pCallback->m_info.m_eState << endl;
    if (pCallback->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting)
    //you can check the previous connecting state as well if you really want to. That way you can see if the client who is calling this function was previously in an unconnected state.
    //to do that, include (pCallback->m_eOldState == k_ESteamNetworkingConnectionState_None) in your if statement
    {
        cout << "Connecting" << endl;
        connectionToClient = pCallback->m_hConn;
        SteamNetworkingSockets()->AcceptConnection(connectionToClient);  //The client doesn't need to accept the connection. Only the server does. Once we accept this connection here, the client will automatically see the connection status change on their end.
        SteamNetworkingSockets()->SendMessageToConnection(connectionToClient, thisServerSteamIDAsString.c_str(), thisServerSteamIDAsString.size(), k_nSteamNetworkingSend_Reliable, nullptr); //can also replace k_nSteamNetworkingSend_Reliable with k_nSteamNetworkingSend_Unreliable. Reliable is TCP-like (guaranteed delivery + packets arriving in correct order) using UDP packets. Unreliable is fire-and-forget UDP.
        //Steam's SpaceWar example sends more info than just the server's ID string to the client as I do here, but the point is you need to start sending things immediately after accepting the connection to the client so that the connection doesn't timeout or get blocked.
    }
    else if (pCallback->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected)
    {
        cout << "Connected" << endl;
        connected = true;
    }
    else if (pCallback->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
    {
        cout << "ClosedByPeer" << endl;
        connected = false;
        SteamNetworkingSockets()->CloseConnection(pCallback->m_hConn,0,nullptr,false); //must close connection on this to prevent leaks/issues
        //see https://partner.steamgames.com/doc/api/ISteamNetworkingSockets  and look for CloseConnection() to understand the parameters. To start, you won't need to change them.
        connectionToClient = 0;
    }
    else if (pCallback->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
    {
        cout << "ProblemDetectedLocally" << endl;
        connected = false;
        SteamNetworkingSockets()->CloseConnection(pCallback->m_hConn,0,nullptr,false); //must close connection on this to prevent leaks/issues
        //see https://partner.steamgames.com/doc/api/ISteamNetworkingSockets  and look for CloseConnection() to understand the parameters. To start, you won't need to change them.
        connectionToClient = 0;
    }
}

int main()
{
    if (!SteamAPI_Init()) { return 0; }
    cout << "Steamworks active." << endl;
    //cout << "Connection Method: PEER TO PEER" << endl;

    SteamNetworkingUtils()->InitRelayNetworkAccess(); //Steam API recommends you run this first and wait a bit if you're using Peer to Peer connections like we are
    std::this_thread::sleep_for(std::chrono::nanoseconds( 4000000000 ));  //wait 4 seconds to give Steam time to spool up the InitRelayNetworkAccess line above
    CallBackClass* callbackHolder = new CallBackClass;  //creating an instance of this class starts the ConnectionStatusChange() callBack function above that will throw when a connection is received. This MUST be created after SteamAPI_Init() is done
    callbackHolder->listenSocket = SteamNetworkingSockets()->CreateListenSocketP2P(0,0,nullptr);
    cout << "Listen socket: " << callbackHolder->listenSocket << endl;
    //the above line of code is where we finally start listening for incoming connections that are called by Steam.connectP2P() on the GodotSteam side. New connection attempts will throw CallBackClass::ConnectionStatusChange() above
    bool shouldWeQuit = false; //in this example, the server will toggle this to boolean to true once the server receives "/quit" from the client
    int loopCounter = 0;
    while (!shouldWeQuit)
    {
        SteamAPI_RunCallbacks(); //You MUST run this function often in your server game loop! If you forget to run this, Steam will not execute a lot of the API, and you will wonder why packets aren't being sent/received!
        if (!(callbackHolder->connected)) { continue; }
        loopCounter++;
        if (loopCounter % 30 == 0) //Since this loop is at 60 fps, send/receive every half second. So long as you're sending/receiving packets to the client within a few seconds, Steam claims they will automatically send keep-alive packets to handle connection upkeep for you
        {
            int NUMBER_OF_MESSAGES_TO_RECEIVE_AT_ONCE = 128; //we are allocating memory space on the heap for this many potential messages. You want to make this number big enough so that you're catching all inbound packets from clients, but you'll be using a lot of memory if you make this number too big.
            SteamNetworkingMessage_t** channel_messages = new SteamNetworkingMessage_t*[NUMBER_OF_MESSAGES_TO_RECEIVE_AT_ONCE];
            int available_messages = SteamNetworkingSockets()->ReceiveMessagesOnConnection(callbackHolder->connectionToClient,channel_messages,NUMBER_OF_MESSAGES_TO_RECEIVE_AT_ONCE);

            for (int i=0;i<available_messages;i++) //here is where we parse the received messages. The code below is kind of technical, but this method of handling incoming data follows how Steam's latest SpaceWar AND how GodotSteam does it under the hood, so this is approved of and tested by far more than just myself
            {
                int message_size = channel_messages[i]->m_cbSize; //Steam sends us this size. As far as I know, this is trustworthy.
                uint8* source_data = (uint8*)channel_messages[i]->m_pData; //each message from a client is a pure byte array at this point. That's a little hardcore for a beginner-level example, so we'll convert this to a string with the lines of code below
                char *output_data = new char[message_size+1](); //I think the reason we create an entirely new space in memory to hold the messages is for safety reasons. Steam tells us how big the message is with m_cbSize, so this way, we can copy ONLY the bytes Steam says are valid and ignore anything afterwards without being subject to parsing/injection attacks that could cause overrun/overflow memory issues and cause our server to crash from a hacked client packet
                for (int j=0;j<message_size;j++)
                {
                    output_data[j] = (char)source_data[j]; //a literal copying over to new memory space, byte for byte. Not the greatest for performance, but because of how CPUs cache data, this will actually happen surprisingly fast if the message is small.
                    //copying the data over to source_data is also a good idea because Steam says we MUST clear the message data below with something like channel_messages[i]->Release();, which will nullify output_data and leave us with no data once we leave this for loop
                }
                output_data[message_size] = '\0';  //This is my thought on how to handle the fact that our Godot client code is sending us string data. It seems to work in testing.
                string output_data_as_str = output_data;  //Because of the above line, we can just dumbly assign the parsed&copied data as a string without fear of safety issues at this point.
                shouldWeQuit = (output_data_as_str.compare("/quit")==0); //the GodotSteam example client is coded to send us "/quit" when it's done with the connection. This is how we know to stop listening and close the connection if we're not using connectionMethod == "MESSAGES" (SteanNetworkingMessages does connections in the background without us handling them, so the connection will just timeout and close automatically)
                string serverReceivedStr = "Server received: " + output_data_as_str;
                cout << serverReceivedStr << endl;

                //the line of code below sends the string we received back to the client with "Server received:" before the client's sent string. You will want to change serverReceivedStr to have the data you actually want to send your client
                EResult sendResult = SteamNetworkingSockets()->SendMessageToConnection(callbackHolder->connectionToClient, serverReceivedStr.c_str(), serverReceivedStr.size(), k_nSteamNetworkingSend_Reliable, nullptr); //can also replace k_nSteamNetworkingSend_Reliable with k_nSteamNetworkingSend_Unreliable. Reliable is TCP-like (guaranteed delivery + packets arriving in correct order) using UDP packets. Unreliable is fire-and-forget UDP.
                cout << "**Send result: " << sendResult << endl; //A printout of 1 means OK. All of the possible results are here: https://partner.steamgames.com/doc/api/steam_api  , look for the EResult table

                delete [] output_data; //you could keep this copied over data longer for other uses, but you do have to delete this somewhere, or else you'll have memory leaks. Remember: every time you use the 'new' keyword, you have to 'delete' it somewhere later.
                channel_messages[i]->Release();  //Steam API requires us to do this with every message that's in the message array
            }
            delete [] channel_messages;  //Steam didn't do this in their SpaceWar example, but GodotSteam does do this. In my original code, I also did this. There's no reason to risk leaving this line out, as you risk a minor memory leak.
        }
        std::this_thread::sleep_for(std::chrono::nanoseconds( 16666666 ));  //wait 1/60th of a second. simply chosen to mimic a standard game loop time amount, as SteamAPI_RunCallbacks() above is expecting that kind of number of calls per second.
    }
    if (callbackHolder->connected) //should be true due to the way we coded the above loop
    {
        cout << "Client is still connected. Closing the connection before shutting down the server..." << endl;  //At this point, we will just have exited the above while loop and haven't closed our connection with the client. In your own code, you will probably want to close the connection in a different place. Remember, the CallBackClass::ConnectionStatusChange() only runs if the connection is changed from external forces. If the server (i.e. this code) decides to close a connection, this code here is how we need to do it, and it doesn't trigger CallBackClass::ConnectionStatusChange() because we changed the connection ourselves
        SteamNetworkingSockets()->CloseConnection(callbackHolder->connectionToClient,0,nullptr,false); //must close connection on this to prevent leaks/issues
    }
    SteamNetworkingSockets()->CloseListenSocket(callbackHolder->listenSocket); //should close client connections before doing this, as this will abruptly close all connections at once.
    delete callbackHolder;
    std::this_thread::sleep_for(std::chrono::nanoseconds( 4000000000 ));  //wait 4 seconds. If you immediately try to call SteamAPI_Shutdown() below without waiting for the client connection and listen socket to close, you'll get a Segmentation fault. Waiting 1 second seems to be sufficient, but I made it 4 seconds just to be safe here.
    cout << "Attempting to shut down Steam API..." << endl;
    SteamAPI_Shutdown();  //Steam API requires us to call this when we're done
    return 0;
}
