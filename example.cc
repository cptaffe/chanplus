

#include "channel.h"
#include <iostream>
#include <future>

using namespace channel;

void SendFiveNumbers(Channel<int>::Sender s) {
	for (int i = 0; i < 5; i++) {
		s.Send(i);
	}
	// only sender is destroyed, so channel is killed.
}

void RecieveUntilEmpty(Channel<int>::Reciever r) {
	int i;
	while (r.Recieve(i)) {
		std::cout << "recieved: " << i << std::endl;
	}
	// reciever is destroyed.
}

int main() {
	Channel<int> chan;

	// create a sender and a reciever.
	auto sender = chan.CreateSender();
	auto reciever = chan.CreateReciever();

	auto st = std::async(SendFiveNumbers, sender);
	auto rt = std::async(RecieveUntilEmpty, reciever);
	rt.wait();
	st.wait();
}
