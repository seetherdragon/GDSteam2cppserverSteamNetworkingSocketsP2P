# GDSteam2cppserverSteamNetworkingSocketsP2P

FOR THE CLIENT:<br />
-Get the pre-compiled GodotSteam editor (https://github.com/CoaguCo-Industries/GodotSteam/releases/tag/v4.4)<br />
-Get the GodotSteam example project (https://github.com/CoaguCo-Industries/GodotSteam-Example-Project/tree/godot4)<br />
-Open the precompiled Godotsteam editor and open the example project<br />
-Create an empty node in the main scene of the GodotSteam example project<br />
-Put the GDSteamClient.gd from this repository inside the GodotSteam example project and attach it (the GDscript) to the empty node you just created so that the GDscript will run when you start the example project<br />
<br />
FOR THE SERVER:<br />
-Do everything in the "Getting Started" section of this webpage-> (https://partner.steamgames.com/doc/sdk/api) to get the Steam SDK going. <br />
-Get the main.cpp from this repository and put it into a new C++ project along with your Steam header files folder and your Steam SDK library file that you got from the above step. <br />
-Figure out how to compile the main.cpp with the Steam library file and the header files on whatever platform you're using. (I used CodeBlocks in Linux, as it was straightforward)  <br />
    (For CodeBlocks, go to Project->Properties->

