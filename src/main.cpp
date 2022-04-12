#include "../include/abyss_client.hpp"
#include "../include/celestia_server.hpp"
#include "../include/output_helper.hpp"
#include <atomic>
#include <conio.h>

void MainWorker(
	std::atomic_bool& signalStop, 
	WhispersAbyss::OutputHelper& output, 
	WhispersAbyss::CelestiaServer& server, 
	WhispersAbyss::AbyssClient& client) {

	// start server and client and waiting for initialize
	// start abyss client first, because abyss client need more time to init
	output.Printf("Starting server & client...");
	server.Start();
	client.Start();
	while (!server.mIsRunning.load());
	output.Printf("Celestia Server started.");
	while (!client.mIsRunning.load());
	output.Printf("Abyss Client started.");

	// msg deliver
	while (true) {
		// check exit
		if (signalStop.load()) break;

	}

	output.Printf("Stoping server & client...");
	server.Stop();
	client.Stop();
	while (server.mIsRunning.load());
	output.Printf("Celestia Server stoped.");
	while (client.mIsRunning.load());
	output.Printf("Abyss Client stoped.");
}

int main(int argc, char* argv[]) {

	puts("Whispers Abyss - A sh*t protocol converter.");
	puts("");
	puts("From the ashes to this world anew.");
	puts("Ah... How brightly burned the oath. Hear the inferno's call.");
	puts("Now... Is the time for my second coming.");
	puts("");
	puts("======");

	// allocate signal for worker
	std::atomic_bool signalStop;
	signalStop.store(false);

	WhispersAbyss::OutputHelper outputHelper;
	WhispersAbyss::CelestiaServer celestiaServer(&outputHelper, 6172);
	WhispersAbyss::AbyssClient abyssClient(&outputHelper, "2.bmmo.swung0x48.com:26676", "SwungMoe");

	// start worker
	std::thread tdMainWorker(
		&MainWorker, 
		std::ref(signalStop),
		std::ref(outputHelper), 
		std::ref(celestiaServer), 
		std::ref(abyssClient)
	);

	// accept input
	char inc;
	while (true) {
		inc = _getch();

		switch (inc) {
			case 'q':
			{
				signalStop.store(true);
				goto exit_program;	// exit switch and while
			}
			default:
			{
				puts("Invalid command!");
				break;
			}
				
		}
	}

	// stop server and client
exit_program:
	if (tdMainWorker.joinable())
		tdMainWorker.join();

	puts("");
	puts("See ya~");

	return 0;
}

