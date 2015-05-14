
// Copyright (c) 2015 Connor Taffe. All rights reserved.

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <iostream>

namespace channel {

template <typename t>
class Channel {
public:
	Channel(int max = 10) : senders(0), chan(max) {}

	class Sender {
	public:
		Sender(Channel<t>& m) : mesg(m) {}
		~Sender() {
			mesg.DeadSender();
		}

		void Send(const t& item) {
			mesg.chan.Put(item);
		}
	private:
		Channel<t>& mesg;
	};

	class Reciever {
	public:
		Reciever(Channel<t>& m) : mesg(m) {}
		~Reciever() {}

		bool Recieve(t& item) {
			return mesg.chan.Get(&item);
		}
	private:
		Channel<t>& mesg;
	};

	Reciever CreateReciever() {
		Reciever r(*this);
		return r;
	}

	Sender CreateSender() {
		senders++;
		Sender s(*this);
		return s;
	}
private:

	class BaseChannel {
	public:
		BaseChannel(int max) : alive_(true), max_count(max) {}

		bool Get(t *item) {
			// aquire mutex & wait for empty.
			std::unique_lock<std::mutex> lock(mut);
			while (count_ == 0 && alive_) {
				nempty.wait(lock);
			}

			if (!alive_ && queue.empty()) {
				return false;
			}

			*item = queue.front();
			queue.pop();
			--count_;

			nfull.notify_one();
			lock.unlock();
			return true;
		}

		void Put(const t& item) {
			// aquire mutex & wait for empty.
			std::unique_lock<std::mutex> lock(mut);
			while (count_ == max_count && alive_) {
				nfull.wait(lock);
			}

			if (!alive_) {
				return;
			}

			queue.push(item);

			++count_;
			nempty.notify_one();
			lock.unlock();
		}

		void Kill() {
			alive_ = false;
			// wake up all waiting because death.
			nfull.notify_all();
			nempty.notify_all();
		}

		bool alive() const {
			return alive_;
		}
	private:
		std::mutex mut;
		std::queue<t> queue;

		// dead queue.
		std::atomic<bool> alive_;

		// atomic access full/empty
		const int max_count;
		std::atomic<int> count_ = {0};

		std::condition_variable nfull;
		std::condition_variable nempty;
	};

	void DeadSender() {
		senders--;
		if (senders <= 0 && chan.alive()) {
			chan.Kill(); // no more senders.
		}
	}

	BaseChannel chan;
	std::atomic<int> senders;
};

} // namespace channel

#endif // CHANNEL_H_
