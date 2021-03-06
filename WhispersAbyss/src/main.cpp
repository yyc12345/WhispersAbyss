#include "../include/abyss_client.hpp"
#include "../include/celestia_server.hpp"
#include "../include/output_helper.hpp"
#include "../include/leylines_bridge.hpp"
#include <atomic>
#include <conio.h>

void MainWorker(
	const char* serverUrl,
	uint16_t acceptPort,
	std::atomic_bool& signalStop,
	std::atomic_bool& signalProfile,
	WhispersAbyss::OutputHelper& output) {

	// init steam works for abyss client
	output.Printf("Preparing abyss...");
	WhispersAbyss::AbyssClient::Init(&output);
	output.Printf("Abyss started.");

	// start server and client and waiting for initialize
	// start abyss client first, because abyss client need more time to init
	output.Printf("Preparing celestia...");
	WhispersAbyss::CelestiaServer server(&output, acceptPort);
	server.Start();
	while (!server.mIsRunning.load());
	output.Printf("Celestia started.");

	// core processor
	std::deque<WhispersAbyss::CelestiaGnosis*> conns;
	std::deque<WhispersAbyss::LeyLinesBridge*> conn_pairs, cached_pairs;
	bool order_profile = false;
	while (true) {
		// check exit
		if (signalStop.load()) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		// get whether need profile
		order_profile = false;
		order_profile = signalProfile.exchange(order_profile);
		if (order_profile) {
			output.Printf("Start collecting profile...");
		}

		// getting new tcp connections
		server.GetConnections(&conns);
		while (conns.begin() != conns.end()) {
			WhispersAbyss::LeyLinesBridge* bridge = new WhispersAbyss::LeyLinesBridge(&output, serverUrl, *conns.begin());
			cached_pairs.push_back(bridge);
			conns.pop_front();
		}

		// process existed conns
		while (conn_pairs.begin() != conn_pairs.end()) {
			WhispersAbyss::LeyLinesBridge* bridge = *conn_pairs.begin();
			bridge->ProcessBridge();
			if (bridge->GetConnStatus() == WhispersAbyss::ModuleStatus::Stopped) {
				// destroy this instance
				delete bridge;
			} else {
				// otherwise, push into list for next query
				cached_pairs.push_back(bridge);
			}
			conn_pairs.pop_front();
		}

		// move back conn_pairs
		// and check profile if need
		while (cached_pairs.begin() != cached_pairs.end()) {
			if (order_profile) {
				(*cached_pairs.begin())->ReportStatus();
			}
			conn_pairs.push_back(*cached_pairs.begin());
			cached_pairs.pop_front();
		}
	}

	// stop celestia server first to reject more incoming connetions
	output.Printf("Stoping celestia...");
	server.Stop();
	output.Printf("Celestia stoped.");

	output.Printf("Stoping bridges...");
	// actively order stop
	for (auto it = conn_pairs.begin(); it != conn_pairs.end(); ++it) {
		(*it)->ActiveStop();
	}
	// check stop status and wait all bridge stopped
	while (conn_pairs.begin() != conn_pairs.end()) {
		WhispersAbyss::LeyLinesBridge* bridge = *conn_pairs.begin();
		if (bridge->GetConnStatus() == WhispersAbyss::ModuleStatus::Stopped) {
			delete bridge;
			conn_pairs.pop_front();
		} else std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	output.Printf("All bridges stoped.");

	output.Printf("Stoping abyss...");
	WhispersAbyss::AbyssClient::Dispose();
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
	puts("");

	// ========== Check Parameter ==========
	if (argc != 3) {
		puts("Wrong arguments.");
		puts("Syntax: WhispersAbyss [accept_port] [server_address]");
		puts("Program will exit. See README.md for more detail about commandline arguments.");
		return 0;
	}
	char* argsServerUrl = argv[2];
	long int argsAcceptPort = strtoul(argv[1], NULL, 10);
	if (argsAcceptPort == LONG_MAX || argsAcceptPort == LONG_MIN || argsAcceptPort > 65535u) {
		puts("Wrong arguments. Port value is illegal.");
		puts("Syntax: WhispersAbyss [accept_port] [server_address]");
		puts("Program will exit. Please specific a correct port number.");
		return 0;
	}

	// ==========Real Work ==========
	// allocate signal for worker
	std::atomic_bool signalStop, signalProfile;
	signalStop.store(false);

	WhispersAbyss::OutputHelper outputHelper;

	// start worker
	std::thread tdMainWorker(
		&MainWorker,
		argsServerUrl,
		6172,
		std::ref(signalStop),
		std::ref(signalProfile),
		std::ref(outputHelper)
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
			case 'p':
			{
				signalProfile.store(true);
				break;
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

	outputHelper.Printf("See ya~");

	return 0;
}

