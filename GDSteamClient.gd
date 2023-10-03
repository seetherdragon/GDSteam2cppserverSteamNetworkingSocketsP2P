extends Node #put this Node somewhere in the main scene so that it'll run when you startup the example. Feel free to put it anywhere else, but it needs the Steam startup bits that this example calls in global.gd

var timer = 0
var counter = 0
var serverConnectionHandle #uint32 received from connectP2P function
var weAreDoneSendingData = false
var NUM_PACKETS_TO_SEND = 20 #the 20th packet is a "/quit" signal to the server and then we stop sending/receiving
var isNetworkRelayInitialized = false #We use Steam's Data Relay Network to send/receive Peer-to-Peer packets, so we need to know when we're connected to the Relay network
var isConnectionAccepted = false #We also need to know when the C++ server has accepted our connection before we start spamming packets toward it

# Called when the node enters the scene tree for the first time.
func _ready():	
	#global.gd is the first node to run _ready()  (see Project->Project Settings->Autoload tab. Global is topmost)
	#because of this, the Steam API will already be loaded by the time we get to this code here. We don't need to do any fancy waiting or checking or anything
	_connect_Steam_Signals("network_connection_status_changed", "_networkConnectionChange") #GodotSteam receives a Godot signal whenever the connection status changes. Here, we connect that signal to the function _networkConnectionChange() below
	Steam.initRelayNetworkAccess() #Because we are using Steam's Peer-to-Peer relay network to handle our packets, we need to call this function first and wait until Steam's relay network is ready for us (we check using Steam.getRelayNetworkStatus() below)
	#The connection method outlined here leverages the Steam Relay network's ability to punchthrough NATs and firewalls and ip routing without you having to do any of that!	
	Steam.setIdentitySteamID64("server", 76561199493174690) #Steam ID of Steam account that's currently running on C++ server
	#GodotSteam will make an internal dictionary and record the above steam ID under the key of "server". All you need is the string identifier, but it might help to remember that it's actually a dictionary key that stores the true connection identity data

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if (weAreDoneSendingData): #this example only sends 20 packets to the server before closing the connection
		return  #you won't be closing the connection so early in a real game, but this way I can show you how to close the client's connection to the server gracefully whenever you want to	
	timer = timer + delta
	
	if (!isNetworkRelayInitialized):
		if (timer > 0.5):
			timer = 0
			var relayStatus = Steam.getRelayNetworkStatus()
			print("Steam Relay status: " + convertRelayStatusToString(relayStatus))
			if (relayStatus == 100): #steamnetworkingtypes.h tells us what when ESteamNetworkingAvailability is 100, the relay is online/available
				timer = -1
				isNetworkRelayInitialized = true
				print("Connecting to C++ server...")
				serverConnectionHandle = Steam.connectP2P("server", 0,[]) #this is where we need to use the same string that we used above in Steam.setIdentitySteamID64(). GodotSteam will use this string as a key to pull connection data out of a dictionary to start the connection attempt
	elif (isConnectionAccepted) and (timer > 0.5): #so long as you're sending/receiving packets to server within a few seconds, Steam claims they will
		#automatically send keep-alive packets to handle connection upkeep for you
		timer = 0
		counter = counter + 1
		
		if (!weAreDoneSendingData):
			var dataStringToSend = "Hi Server packet#" + str(counter)
			if counter == NUM_PACKETS_TO_SEND: #we arbitrarily decide to tell the server to close its connection with us on the 20th packet we send its way
				weAreDoneSendingData = true
				dataStringToSend = "/quit"  #the server is looking for this exact phrase in order to close the connection (see main.cpp to understand the server's code)
			print("Sending '" + dataStringToSend + "' to server...")
			var dataToSend:PackedByteArray = dataStringToSend.to_utf8_buffer() #convert string to PackedByteArray
			var sendResult = Steam.sendMessageToConnection(serverConnectionHandle,dataToSend,8) #8 is the send reliable flag, use 0 if you want unreliable packets.
			print("Send result: " + str(sendResult)) #a printout of 1 means OK. All of the possible results are here:  https://partner.steamgames.com/doc/api/steam_api  , look for the EResult table
		
			var listOfMessages = Steam.receiveMessagesOnConnection(serverConnectionHandle,128) #read up to 128 messages in buffer. GodotSteam follows Steam's lead in SpaceWar example by allocating 128 message spaces, so just keep in mind that this does take up memory
			for message in listOfMessages:
				var incomingData : PackedByteArray = message["payload"]
				var incomingDataStr : String = incomingData.get_string_from_ascii()
				print( message["identity"] + " said: " + incomingDataStr )
			#if you check out the function Steam::receiveMessagesOnChannel in godotsteam.cpp (https://github.com/CoaguCo-Industries/GodotSteam/blob/godot3/godotsteam.cpp), you can find all of the key/value pairs for the dictionary named message above
			#you can get size, connection, identity, user_data, time_received, message_number, channel, flags, 
			#the PackedByteArray you receive is constructed with the proper size, so you can just call incomingData.size() instead of message["size"]. They both work.

func _networkConnectionChange(incomingConnectionHandle:int, connectionInfo:Dictionary, oldConnectionState:int) -> void:
	#if you find Steam::network_connection_status_changed() in https://github.com/CoaguCo-Industries/GodotSteam/blob/godot4/godotsteam.cpp, you can see what key/value pairs the connectionInfo Dictionary has for you to use
	print("Previous connection state for this connection was: " + convertConnectionStateNumToString(oldConnectionState))
	print("Connection state now changed to: " + convertConnectionStateNumToString(connectionInfo["connection_state"]))
	if (connectionInfo["connection_state"] == 3):  #steamnetworkingtypes.h shows that 3 is defined as k_ESteamNetworkingConnectionState_Connected.
		isConnectionAccepted = true

func convertConnectionStateNumToString(connectionState:int):
	#steamnetworkingtypes.h defines the connectionstate number definitions under the enum ESteamNetworkingConnectionState
	if (connectionState==1):
		return "CONNECTING"
	elif (connectionState==2):
		return "FINDING ROUTE"
	elif (connectionState==3):
		return "CONNECTED"
	elif (connectionState==4):
		return "CLOSED BY SERVER"
	elif (connectionState==5):
		return "CONNECTION DISRUPTED FROM HERE"
	elif (connectionState==0):
		return "NO CONNECTION"
	else:
		return "UNKNOWN"
	
func convertRelayStatusToString(relayStatus:int):
	#steamnetworkingtypes.h defines the relay state number definitions under the enum ESteamNetworkingAvailability
	if (relayStatus==2):
		return "WAITING (FOR STEAM CERT OR FOR NETWORK CONFIG)"
	elif (relayStatus==3):
		return "ATTEMPTING"
	elif (relayStatus==100):
		return "ONLINE/AVAILABLE"
	elif (relayStatus==-101):
		return "FAILED AFTER MANY ATTEMPTS"
	elif (relayStatus==-10):
		return "FAILED BUT RETRYING"
	elif (relayStatus==-102):
		return "CANNOT TRY FOR RELAY, LOCAL NETWORK NOT WORKING"
	elif (relayStatus==1):
		return "DONT KNOW, NEVER TRIED RELAY"
	else:
		return "UKNOWN"
	#relay status 2=waiting, 3=attempting,100=online/available,-101=failed,-10=retrying,-102=cannottry,1=nevertried,0=unknown

# Connect a Steam signal and show the success code
func _connect_Steam_Signals(this_signal: String, this_function: String) -> void: #boilerplate function shamelessly stolen from GodotSteam's lobby example so that we can connect a network signal thrown by the Steam API to _networkConnectionChange() above.
	var SIGNAL_CONNECT: int = Steam.connect(this_signal, Callable(self, this_function))
	if SIGNAL_CONNECT > OK:
		print("[STEAM] Connecting "+str(this_signal)+" to "+str(this_function)+" failed: "+str(SIGNAL_CONNECT))
