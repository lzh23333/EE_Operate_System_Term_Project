#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <chrono>
#include <string>
#include <fstream>
#include <map>

#define time_p std::chrono::system_clock::time_point
#define sys_clk std::chrono::system_clock
//consumer's id must starts from 1
//or it would cause error


class Semaphora {
	//利用互斥锁与条件变量
	//参考该博客代码：https://segmentfault.com/a/1190000006818772
private:
	std::mutex mtx;
	std::condition_variable cv;
	int signal;

public:
	Semaphora(int _count = 1) { signal = _count; }
	void up() {
		std::unique_lock<std::mutex> lock(mtx);
		signal++;
		cv.notify_one();
	}
	void down() {
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [=] {return signal > 0; });
		signal--;
	}
	int get_signal() { return signal; }
};

struct Consumer {
	int id;
	int serve_time;
	int enter_time;
	Consumer(int _id = 0, int _t = 0, int _e = 0) :
		id(_id), serve_time(_t), enter_time(_e) {}
};

struct msg {
	//用于记录结果信息
	int enter_t;
	int begin_t;
	int leave_t;
	int server_id;
	int self_id;
	msg(int s_id, int e_t, int b_t, int l_t, int server) :
		self_id(s_id), enter_t(e_t), leave_t(l_t), begin_t(b_t), server_id(server) {}
};
void server(int _id);
void consumer(Consumer c);

//全局变量
int consumer_num;			//顾客总数
int server_num;				//服务数
int done_num;				//顾客完成数
int number;					//顾客号码
bool done = false;
bool ready = false;
time_p begin_time;
int end_time = 0;

Semaphora call_signal(1);  //叫号的信号量
Semaphora take_signal(1);  //取号的信号量
Semaphora list_signal(1);  //对waitlist操作的信号量
Semaphora done_signal(1);

std::queue<Consumer> waitlist;				//排队队列
std::vector<std::thread> server_threads;	//服务员线程组
std::vector<std::thread> consumer_threads;	//顾客线程组
std::vector<Consumer> all_consumer;			//顾客数据
std::condition_variable begin_cv;
std::mutex begin_m;
std::mutex list_m;							//waitlist有成员时唤醒server线程
std::condition_variable list_cv;			//避免忙循环
std::vector<msg> msg_vec;
std::map<int, int> number_id;


int get_millisec(time_p a, time_p b) {
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(a - b);
	return duration.count();
}



int main(int argc, char *argv[]) {
	number = 1;
	done_num = 0;
	server_num = 1;
	consumer_num = -1;


	if (argc > 2) {
		std::string filename(argv[1]);
		std::string n_s(argv[2]);
		server_num = std::stoi(n_s);
		//std::cout << "server_num is " << server_num << std::endl;


		std::fstream consumer_stream(filename);
		if (!consumer_stream.fail()) {
			std::vector<Consumer> consumer_vec;

			int id = 0;
			int enter_time = 0;
			int serve_time = 0;
			while (consumer_stream >> id >> enter_time >> serve_time) {
				//std::cout << "id:" << id << "\tenter: " << enter_time << "\tneeds " << serve_time << std::endl;
				consumer_vec.push_back(Consumer(id, serve_time, enter_time));
				msg_vec.push_back(msg(id, 0, 0, 0, 0));
			}
			consumer_stream.close();
			consumer_num = int(consumer_vec.size());

			//初始化完毕，接下来进行线程

			//std::cout << "server num: " << server_num << std::endl;

			for (int i = 0; i < server_num; i++) {
				server_threads.push_back(std::thread(server, i));
			}
			//server初始化

			for (int i = 0; i < consumer_num; i++) {
				consumer_threads.push_back(std::thread(consumer, consumer_vec[i]));
			}
			//consumer初始化

			ready = true;
			begin_time = sys_clk::now();
			begin_cv.notify_all();

			for (int i = 0; i < int(consumer_threads.size()); i++)
				consumer_threads[i].join();

			for (int i = 0; i < int(server_threads.size()); i++)
				server_threads[i].join();

			//std::cout << "one day's work done " << std::endl;


			for (int i = 0; i < int(msg_vec.size()); i++) {
				std::cout << "consumer " << msg_vec[i].self_id << "\t server " <<
					msg_vec[i].server_id << "\t enter " << msg_vec[i].enter_t << "\t begin "
					<< msg_vec[i].begin_t << "\t leave " << msg_vec[i].leave_t << std::endl;
				end_time = (end_time > msg_vec[i].leave_t) ? end_time : msg_vec[i].leave_t;
			}
			std::cout << "server_num: " << server_num << " total time: " << end_time << std::endl;


		}
		else {
			std::cout << "file open error" << std::endl;
		}
	}
	getchar();
	return 0;
}

void server(int _id) {
	//柜员线程，保证取号互斥且访问waitlist也互斥

	bool get_work = false;
	Consumer cos;
	int this_id = 0;

	while (1) {
		{
			std::unique_lock<std::mutex> lk(list_m);
			list_cv.wait(lk, [] {return (!waitlist.empty()) | done; });
		}
		std::cout << "awake" << std::endl;

		call_signal.down();
		list_signal.down();
		//enter region

		if (!waitlist.empty()) {
			//std::cout << "server " << _id << "enters region" << std::endl;
			get_work = true;
			cos = waitlist.front();
			waitlist.pop();
			this_id = number_id.at(cos.id);
			msg_vec[this_id - 1].begin_t = get_millisec(sys_clk::now(), begin_time);
			msg_vec[this_id - 1].server_id = _id;

		}


		//out region
		list_signal.up();
		//std::cout << "server " << _id << "out region" << std::endl;
		call_signal.up();

		if (get_work) {

			std::this_thread::sleep_for(std::chrono::milliseconds(cos.serve_time));
			//使线程休眠对应时间
			{
				done_signal.down();
				done_num++;				//保证done_num 线程安全
				done_signal.up();
			}
			msg_vec[this_id - 1].leave_t = get_millisec(sys_clk::now(), begin_time);

		}
		if (done_num == consumer_num) {
			done = true;
			list_cv.notify_all();
			//std::cout << "server "<<_id <<  "return " << std::endl;
			return;
		}
		get_work = false;
	}
}

void consumer(Consumer c) {
	//顾客线程，保证拿到号互斥且访问waitlist也互斥
	{
		std::unique_lock<std::mutex> lk(begin_m);
		begin_cv.wait(lk, [] {return ready; });
	}//等待直到所有顾客均被构造完毕

	std::this_thread::sleep_for(std::chrono::milliseconds(c.enter_time));
	take_signal.down();


	msg_vec[c.id - 1].enter_t = get_millisec(sys_clk::now(), begin_time);
	number_id.insert(std::pair<int, int>(number, c.id));

	Consumer cos(number++, c.serve_time, c.enter_time);

	//将顾客放入waitlist
	list_signal.down();
	waitlist.push(cos);
	list_cv.notify_all();
	list_signal.up();

	take_signal.up();
}

