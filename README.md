# Whispers Abyss

First, let we say loudly together: Fuck Valve Gns.  
A simple protocol converter. It mainly served for the protocol convertion of [Swung0x48/BallanceMMO](https://github.com/Swung0x48/BallanceMMO). Because my project [yyc12345/BallanceStalker](https://code.blumia.cn/yyc12345/BallanceStalker) couldn't use any Valve Gns code due to the shitty C# support of Godot. I need convert all Valve Gns message into plain message over a normal TCP connection. This is what this peoject do. There is an important thing you should be knowing. TCP is a reliable protocol, but its performance is pretty bad, especially in a bad network. So usually, WhispersAbyss should be run together with the client which want to use WhispersAbyss. Then, all connections are over local loop back, and you will get the best performance. Due to this feature, if you run WhispersAbyss on a server then connect it from other place, it will cause huge performance loss.

This project consist of 2 parts. WhispersAbyss is the main converter written in C++, and ShadowWalker is a tiny BMMO client written in Python. ShadowWalker only implement some basic functions of BMMO protocol, such as login, logout and etc. It is used as a tester for WhispersAbyss and check whether WhispersAbyss can work normally.

## Usage

### WhispersAbyss

Syntax: `WhispersAbyss [accept_port]`

`accept_port` is the port which will accept TCP connections, for example, `6172`.

WhispersAbyss is a console application. You can see some real-time output after starting this application.  
You can press `p` on keyboard directly to show all profiles of running connections.  
Press `q` to exit application.

### ShadowWalker

Syntax: `python3 ShadowWalker.py -p [local_port] -u [remote_url] -n [username] -i [uuid]`

`local_port` is pointed to the acceptor port of WhispersAbyss.  
`remote_url` is the combination of host and port which you want to connect, for example, `bmmmo.server.net:26675`.  
`username` is in-game name.  
`uuid` is an unique identifier for each user. Its ususally have the format like `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`.

Press `q` to exit program.  
Press `p` for show user list.  
Press Tab to open a chat prompt and you can input your chat content.

## Compile

Linux and CMake support is on the way. We only introduce Windows build now.

Open `WhispersAbyss/WhispersAbyss.props` and point macro define to correct folder.

* ASIO_PATH: Root of asio library.
* VALVE_GNS_PATH: Root of [ValveSoftware/GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets).
* FUCK_VALVE_GNS_PATH: The folder containing Valve Gns link file `GameNetworkingSockets.lib`.

Tips: If you have [Swung0x48/BallanceMMO](https://github.com/Swung0x48/BallanceMMO), you can point `ASIO_PATH` to `BallanceMMO/submodule/asio` and point `VALVE_GNS_PATH` as `BallanceMMO/submodule/GameNetworkingSockets` as a alternative.  
Considering the diffuculty of compiling Valve Gns on Windows, I suggest you use alternative to get the `.lib` file of Valve Gns. First, open released BMMO mod and decompress it to get `GameNetworkingSockets.dll`. Then, use Visual Studio dev tools `dumpbin` to get its export table, use some tools to format it and get legal `GameNetworkingSockets.def`. At last, use Visual Studio dev environment and execute `LIB /DEF:GameNetworkingSockets.def /machine:IX86` to get a workable `GameNetworkingSockets.lib` for linker.

Then, open Visual Studio solution, choose proper configuration, such as Debug and Release. Then, compile it.

## Related project:

* [Swung0x48/BallanceMMO](https://github.com/Swung0x48/BallanceMMO)
* [yyc12345/bmmo-protocol-compiler](https://github.com/yyc12345/bmmo-protocol-compiler)
* [yyc12345/BallanceStalker](https://code.blumia.cn/yyc12345/BallanceStalker)

