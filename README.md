# GodotSteamClientToCPlusPlusServer

Example of how to get a GodotSteam client to send/receive packets with a server running C++ (using the C++ Steam SDK) <br />
Uses Steam's NetworkingSockets and Steam's Peer-to-peer connections over the Steam Data Relay network.<br />
<br />
FOR THE CLIENT:<br />
-Get the pre-compiled GodotSteam editor (https://github.com/CoaguCo-Industries/GodotSteam/releases/tag/v4.4)<br />
-Get the GodotSteam example project (https://github.com/CoaguCo-Industries/GodotSteam-Example-Project/tree/godot4)<br />
-Open the precompiled Godotsteam editor and open the example project<br />
-Create an empty node in the main scene of the GodotSteam example project<br />
-Put the GDSteamClient.gd from this repository inside the GodotSteam example project and attach it (the GDscript) to the empty node you just created so that the GDscript will run when you start the example project<br />
<br />
FOR THE SERVER:<br />
 1. Do everything in the "Getting Started" section of this webpage-> (https://partner.steamgames.com/doc/sdk/api) to get the Steam SDK going. <br />
 2. Get the main.cpp from this repository and put it into a new C++ project along with your Steam header files folder and your Steam SDK library file that you got from the above step. <br />
 3. Figure out how to compile the main.cpp with the Steam library file and header files you downloaded in step 1 on whatever platform you're using. (I used CodeBlocks in Linux, as it was straightforward)  <br />
    (For CodeBlocks, go to Project->Properties->Project Settings[tab]->Project's build options[button]->Linker settings[tab]->Add[button] and find your library from the Steamworks sdk (either .lib, .dylib, or .so depending on your OS. I used Debian Linux, so I linked to my libsteam_api.so file) <br />

<br />
-Build/compile the server project, start Steam on the server computer, run the server app you just made.<br />
-Start Steam on the client computer, run the GodotSteam example project that has the Node with GDSteamClient.gd attached.<br />
-Watch the consoles for both apps as they talk to each other!

