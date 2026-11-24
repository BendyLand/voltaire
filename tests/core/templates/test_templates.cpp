
// === Merged from tests/core/templates/test_a_hash_map.cpp ===
/*  test_a_hash_map.cpp                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestAHashMap {

TEST_CASE("[AHashMap] List initialization") {
	AHashMap<int, String> map{ { 0, "A" }, { 1, "B" }, { 2, "C" }, { 3, "D" }, { 4, "E" } };

	CHECK(map.size() == 5);
	CHECK(map[0] == "A");
	CHECK(map[1] == "B");
	CHECK(map[2] == "C");
	CHECK(map[3] == "D");
	CHECK(map[4] == "E");
}

TEST_CASE("[AHashMap] List initialization with existing elements") {
	AHashMap<int, String> map{ { 0, "A" }, { 0, "B" }, { 0, "C" }, { 0, "D" }, { 0, "E" } };

	CHECK(map.size() == 1);
	CHECK(map[0] == "E");
}

TEST_CASE("[AHashMap] Insert element") {
	AHashMap<int, int> map;
	AHashMap<int, int>::Iterator e = map.insert(42, 84);

	CHECK(e);
	CHECK(e->key == 42);
	CHECK(e->value == 84);
	CHECK(map[42] == 84);
	CHECK(map.has(42));
	CHECK(map.find(42));
}

TEST_CASE("[AHashMap] Overwrite element") {
	AHashMap<int, int> map;
	map.insert(42, 84);
	map.insert(42, 1234);

	CHECK(map[42] == 1234);
}

TEST_CASE("[AHashMap] Erase via element") {
	AHashMap<int, int> map;
	AHashMap<int, int>::Iterator e = map.insert(42, 84);
	map.remove(e);
	CHECK(!map.has(42));
	CHECK(!map.find(42));
}

TEST_CASE("[AHashMap] Erase via key") {
	AHashMap<int, int> map;
	map.insert(42, 84);
	map.erase(42);
	CHECK(!map.has(42));
	CHECK(!map.find(42));
}

TEST_CASE("[AHashMap] Size") {
	AHashMap<int, int> map;
	map.insert(42, 84);
	map.insert(123, 84);
	map.insert(123, 84);
	map.insert(0, 84);
	map.insert(123485, 84);

	CHECK(map.size() == 4);
}

TEST_CASE("[AHashMap] Iteration") {
	AHashMap<int, int> map;

	map.insert(42, 84);
	map.insert(123, 12385);
	map.insert(0, 12934);
	map.insert(123485, 1238888);
	map.insert(123, 111111);

	Vector<Pair<int, int>> expected;
	expected.push_back(Pair<int, int>(42, 84));
	expected.push_back(Pair<int, int>(123, 111111));
	expected.push_back(Pair<int, int>(0, 12934));
	expected.push_back(Pair<int, int>(123485, 1238888));

	int idx = 0;
	for (const KeyValue<int, int> &E : map) {
		CHECK(expected[idx] == Pair<int, int>(E.key, E.value));
		idx++;
	}

	idx--;
	for (AHashMap<int, int>::Iterator it = map.last(); it; --it) {
		CHECK(expected[idx] == Pair<int, int>(it->key, it->value));
		idx--;
	}
}

TEST_CASE("[AHashMap] Const iteration") {
	AHashMap<int, int> map;
	map.insert(42, 84);
	map.insert(123, 12385);
	map.insert(0, 12934);
	map.insert(123485, 1238888);
	map.insert(123, 111111);

	const AHashMap<int, int> const_map(map);

	Vector<Pair<int, int>> expected;
	expected.push_back(Pair<int, int>(42, 84));
	expected.push_back(Pair<int, int>(123, 111111));
	expected.push_back(Pair<int, int>(0, 12934));
	expected.push_back(Pair<int, int>(123485, 1238888));
	expected.push_back(Pair<int, int>(123, 111111));

	int idx = 0;
	for (const KeyValue<int, int> &E : const_map) {
		CHECK(expected[idx] == Pair<int, int>(E.key, E.value));
		idx++;
	}

	idx--;
	for (AHashMap<int, int>::ConstIterator it = const_map.last(); it; --it) {
		CHECK(expected[idx] == Pair<int, int>(it->key, it->value));
		idx--;
	}
}

TEST_CASE("[AHashMap] Replace key") {
	AHashMap<int, int> map;
	map.insert(42, 84);
	map.insert(0, 12934);
	CHECK(map.replace_key(0, 1));
	CHECK(map.has(1));
	CHECK(map[1] == 12934);
}

TEST_CASE("[AHashMap] Clear") {
	AHashMap<int, int> map;
	map.insert(42, 84);
	map.insert(123, 12385);
	map.insert(0, 12934);

	map.clear();
	CHECK(!map.has(42));
	CHECK(map.size() == 0);
	CHECK(map.is_empty());
}

TEST_CASE("[AHashMap] Get") {
	AHashMap<int, int> map;
	map.insert(42, 84);
	map.insert(123, 12385);
	map.insert(0, 12934);

	CHECK(map.get(123) == 12385);
	map.get(123) = 10;
	CHECK(map.get(123) == 10);

	CHECK(*map.getptr(0) == 12934);
	*map.getptr(0) = 1;
	CHECK(*map.getptr(0) == 1);

	CHECK(map.get(42) == 84);
	CHECK(map.getptr(-10) == nullptr);
}

TEST_CASE("[AHashMap] Insert, iterate and remove many elements") {
	const int elem_max = 1234;
	AHashMap<int, int> map;
	for (int i = 0; i < elem_max; i++) {
		map.insert(i, i);
	}

	//insert order should have been kept
	int idx = 0;
	for (const KeyValue<int, int> &K : map) {
		CHECK(idx == K.key);
		CHECK(idx == K.value);
		CHECK(map.has(idx));
		idx++;
	}

	Vector<int> elems_still_valid;

	for (int i = 0; i < elem_max; i++) {
		if ((i % 5) == 0) {
			map.erase(i);
		} else {
			elems_still_valid.push_back(i);
		}
	}

	CHECK(elems_still_valid.size() == map.size());

	for (int i = 0; i < elems_still_valid.size(); i++) {
		CHECK(map.has(elems_still_valid[i]));
	}
}

TEST_CASE("[AHashMap] Insert, iterate and remove many strings") {
	const int elem_max = 432;
	AHashMap<String, String> map;

	// To not print WARNING: Excessive collision count (NN), is the right hash function being used?
	ERR_PRINT_OFF;
	for (int i = 0; i < elem_max; i++) {
		map.insert(itos(i), itos(i));
	}
	ERR_PRINT_ON;

	//insert order should have been kept
	int idx = 0;
	for (auto &K : map) {
		CHECK(itos(idx) == K.key);
		CHECK(itos(idx) == K.value);
		CHECK(map.has(itos(idx)));
		idx++;
	}

	Vector<String> elems_still_valid;

	for (int i = 0; i < elem_max; i++) {
		if ((i % 5) == 0) {
			map.erase(itos(i));
		} else {
			elems_still_valid.push_back(itos(i));
		}
	}

	CHECK(elems_still_valid.size() == map.size());

	for (int i = 0; i < elems_still_valid.size(); i++) {
		CHECK(map.has(elems_still_valid[i]));
	}

	elems_still_valid.clear();
}

TEST_CASE("[AHashMap] Copy constructor") {
	AHashMap<int, int> map0;
	const uint32_t count = 5;
	for (uint32_t i = 0; i < count; i++) {
		map0.insert(i, i);
	}
	AHashMap<int, int> map1(map0);
	CHECK(map0.size() == map1.size());
	CHECK(map0.get_capacity() == map1.get_capacity());
	CHECK(*map0.getptr(0) == *map1.getptr(0));
}

TEST_CASE("[AHashMap] Operator =") {
	AHashMap<int, int> map0;
	AHashMap<int, int> map1;
	const uint32_t count = 5;
	map1.insert(1234, 1234);
	for (uint32_t i = 0; i < count; i++) {
		map0.insert(i, i);
	}
	map1 = map0;
	CHECK(map0.size() == map1.size());
	CHECK(map0.get_capacity() == map1.get_capacity());
	CHECK(*map0.getptr(0) == *map1.getptr(0));
}

TEST_CASE("[AHashMap] Array methods") {
	AHashMap<int, int> map;
	for (int i = 0; i < 100; i++) {
		map.insert(100 - i, i);
	}
	for (int i = 0; i < 100; i++) {
		CHECK(map.get_by_index(i).value == i);
	}
	int index = map.get_index(1);
	CHECK(map.get_by_index(index).value == 99);
	CHECK(map.erase_by_index(index));
	CHECK(!map.erase_by_index(index));
	CHECK(map.get_index(1) == -1);
}

} // namespace TestAHashMap

// === Merged from tests/core/templates/test_command_queue.cpp ===
/*  test_command_queue.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/os/condition_variable.h"

namespace TestCommandQueue {

class ThreadWork {
	Semaphore thread_sem;
	Semaphore main_sem;
	Mutex mut;
	int threading_errors = 0;
	enum State {
		MAIN_START,
		MAIN_DONE,
		THREAD_START,
		THREAD_DONE,
	} state;

public:
	ThreadWork() {
		mut.lock();
		state = MAIN_START;
	}
	~ThreadWork() {
		CHECK_MESSAGE(threading_errors == 0, "threads did not lock/unlock correctly");
	}
	void thread_wait_for_work() {
		thread_sem.wait();
		mut.lock();
		if (state != MAIN_DONE) {
			threading_errors++;
		}
		state = THREAD_START;
	}
	void thread_done_work() {
		if (state != THREAD_START) {
			threading_errors++;
		}
		state = THREAD_DONE;
		mut.unlock();
		main_sem.post();
	}

	void main_wait_for_done() {
		main_sem.wait();
		mut.lock();
		if (state != THREAD_DONE) {
			threading_errors++;
		}
		state = MAIN_START;
	}
	void main_start_work() {
		if (state != MAIN_START) {
			threading_errors++;
		}
		state = MAIN_DONE;
		mut.unlock();
		thread_sem.post();
	}
};

class SharedThreadState {
public:
	ThreadWork reader_threadwork;
	ThreadWork writer_threadwork;

	CommandQueueMT command_queue;

	enum TestMsgType {
		TEST_MSG_FUNC1_TRANSFORM,
		TEST_MSG_FUNC2_TRANSFORM_FLOAT,
		TEST_MSG_FUNC3_TRANSFORMx6,
		TEST_MSGSYNC_FUNC1_TRANSFORM,
		TEST_MSGSYNC_FUNC2_TRANSFORM_FLOAT,
		TEST_MSGRET_FUNC1_TRANSFORM,
		TEST_MSGRET_FUNC2_TRANSFORM_FLOAT,
		TEST_MSG_MAX
	};

	Vector<TestMsgType> message_types_to_write;
	bool during_writing = false;
	int message_count_to_read = 0;
	bool exit_threads = false;

	Thread reader_thread;
	Thread writer_thread;

	int func1_count = 0;

	void func1(Transform3D t) {
		func1_count++;
	}
	void func2(Transform3D t, float f) {
		func1_count++;
	}
	void func3(Transform3D t1, Transform3D t2, Transform3D t3, Transform3D t4, Transform3D t5, Transform3D t6) {
		func1_count++;
	}
	Transform3D func1r(Transform3D t) {
		func1_count++;
		return t;
	}
	Transform3D func2r(Transform3D t, float f) {
		func1_count++;
		return t;
	}

	void add_msg_to_write(TestMsgType type) {
		message_types_to_write.push_back(type);
	}

	void reader_thread_loop() {
		reader_threadwork.thread_wait_for_work();
		while (!exit_threads) {
			command_queue.flush_all();
			message_count_to_read = 0;

			reader_threadwork.thread_done_work();
			reader_threadwork.thread_wait_for_work();
		}
		command_queue.flush_all();
		reader_threadwork.thread_done_work();
	}
	static void static_reader_thread_loop(void *stsvoid) {
		SharedThreadState *sts = static_cast<SharedThreadState *>(stsvoid);
		sts->reader_thread_loop();
	}

	void writer_thread_loop() {
		during_writing = false;
		writer_threadwork.thread_wait_for_work();
		while (!exit_threads) {
			Transform3D tr;
			Transform3D otr;
			float f = 1;
			during_writing = true;
			for (int i = 0; i < message_types_to_write.size(); i++) {
				TestMsgType msg_type = message_types_to_write[i];
				switch (msg_type) {
					case TEST_MSG_FUNC1_TRANSFORM:
						command_queue.push(this, &SharedThreadState::func1, tr);
						break;
					case TEST_MSG_FUNC2_TRANSFORM_FLOAT:
						command_queue.push(this, &SharedThreadState::func2, tr, f);
						break;
					case TEST_MSG_FUNC3_TRANSFORMx6:
						command_queue.push(this, &SharedThreadState::func3, tr, tr, tr, tr, tr, tr);
						break;
					case TEST_MSGSYNC_FUNC1_TRANSFORM:
						command_queue.push_and_sync(this, &SharedThreadState::func1, tr);
						break;
					case TEST_MSGSYNC_FUNC2_TRANSFORM_FLOAT:
						command_queue.push_and_sync(this, &SharedThreadState::func2, tr, f);
						break;
					case TEST_MSGRET_FUNC1_TRANSFORM:
						command_queue.push_and_ret(this, &SharedThreadState::func1r, &otr, tr);
						break;
					case TEST_MSGRET_FUNC2_TRANSFORM_FLOAT:
						command_queue.push_and_ret(this, &SharedThreadState::func2r, &otr, tr, f);
						break;
					default:
						break;
				}
			}
			message_types_to_write.clear();
			during_writing = false;

			writer_threadwork.thread_done_work();
			writer_threadwork.thread_wait_for_work();
		}
		writer_threadwork.thread_done_work();
	}
	static void static_writer_thread_loop(void *stsvoid) {
		SharedThreadState *sts = static_cast<SharedThreadState *>(stsvoid);
		sts->writer_thread_loop();
	}

	void init_threads() {
		reader_thread.start(&SharedThreadState::static_reader_thread_loop, this);
		writer_thread.start(&SharedThreadState::static_writer_thread_loop, this);
	}
	void destroy_threads() {
		exit_threads = true;
		reader_threadwork.main_start_work();
		writer_threadwork.main_start_work();

		reader_thread.wait_to_finish();
		writer_thread.wait_to_finish();
	}

	struct CopyMoveTestType {
		inline static int copy_count;
		inline static int move_count;
		int value = 0;

		CopyMoveTestType(int p_value = 0) :
				value(p_value) {}

		CopyMoveTestType(const CopyMoveTestType &p_other) :
				value(p_other.value) {
			copy_count++;
		}

		CopyMoveTestType(CopyMoveTestType &&p_other) :
				value(p_other.value) {
			move_count++;
		}

		CopyMoveTestType &operator=(const CopyMoveTestType &p_other) {
			value = p_other.value;
			copy_count++;
			return *this;
		}

		CopyMoveTestType &operator=(CopyMoveTestType &&p_other) {
			value = p_other.value;
			move_count++;
			return *this;
		}
	};

	void copy_move_test_copy(CopyMoveTestType p_test_type) {
	}
	void copy_move_test_ref(const CopyMoveTestType &p_test_type) {
	}
	void copy_move_test_move(CopyMoveTestType &&p_test_type) {
	}
};

static void test_command_queue_basic() {
	const char *COMMAND_QUEUE_SETTING = "memory/limits/command_queue/multithreading_queue_size_kb";
	ProjectSettings::set_setting(COMMAND_QUEUE_SETTING, 1);
	SharedThreadState sts;
	sts.init_threads();

	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC1_TRANSFORM);
	sts.writer_threadwork.main_start_work();
	sts.writer_threadwork.main_wait_for_done();
	CHECK_MESSAGE(sts.func1_count == 0,
			"Control: no messages read before reader has run.");

	sts.message_count_to_read = 1;
	sts.reader_threadwork.main_start_work();
	sts.reader_threadwork.main_wait_for_done();
	CHECK_MESSAGE(sts.func1_count == 1,
			"Reader should have read one message");

	sts.message_count_to_read = -1;
	sts.reader_threadwork.main_start_work();
	sts.reader_threadwork.main_wait_for_done();
	CHECK_MESSAGE(sts.func1_count == 1,
			"Reader should have read no additional messages from flush_all");

	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC1_TRANSFORM);
	sts.writer_threadwork.main_start_work();
	sts.writer_threadwork.main_wait_for_done();

	sts.message_count_to_read = -1;
	sts.reader_threadwork.main_start_work();
	sts.reader_threadwork.main_wait_for_done();
	CHECK_MESSAGE(sts.func1_count == 2,
			"Reader should have read one additional message from flush_all");

	sts.destroy_threads();

	CHECK_MESSAGE(sts.func1_count == 2,
			"Reader should have read no additional messages after join");
	ProjectSettings::set_setting(COMMAND_QUEUE_SETTING,
			ProjectSettings::property_get_revert(COMMAND_QUEUE_SETTING));
}

TEST_CASE("[CommandQueue] Test Queue Basics") {
	test_command_queue_basic();
}


TEST_CASE("[CommandQueue] Test Queue Wrapping to same spot.") {
	const char *COMMAND_QUEUE_SETTING = "memory/limits/command_queue/multithreading_queue_size_kb";
	ProjectSettings::set_setting(COMMAND_QUEUE_SETTING, 1);
	SharedThreadState sts;
	sts.init_threads();

	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC3_TRANSFORMx6);
	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC3_TRANSFORMx6);
	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC1_TRANSFORM);
	sts.writer_threadwork.main_start_work();
	sts.writer_threadwork.main_wait_for_done();

	sts.message_count_to_read = -1;
	sts.reader_threadwork.main_start_work();
	sts.reader_threadwork.main_wait_for_done();
	CHECK_MESSAGE(sts.func1_count == 3,
			"Reader should have read at least three messages");

	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC3_TRANSFORMx6);
	sts.writer_threadwork.main_start_work();
	sts.writer_threadwork.main_wait_for_done();
	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC1_TRANSFORM);
	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC3_TRANSFORMx6);
	sts.writer_threadwork.main_start_work();
	OS::delay_usec(1000);

	sts.message_count_to_read = -1;
	sts.reader_threadwork.main_start_work();
	OS::delay_usec(1000);

	sts.writer_threadwork.main_wait_for_done();
	sts.reader_threadwork.main_wait_for_done();
	CHECK_MESSAGE(sts.func1_count >= 3,
			"Reader should have read at least three messages");

	sts.message_count_to_read = 6 - sts.func1_count;
	sts.reader_threadwork.main_start_work();

	// The following will fail immediately.
	// The reason it hangs indefinitely in engine, is all subsequent calls to
	// CommandQueue.wait_and_flush_one will also fail.
	sts.reader_threadwork.main_wait_for_done();

	// Because looping around uses an extra message, easiest to consume all.
	sts.message_count_to_read = -1;
	sts.reader_threadwork.main_start_work();
	sts.reader_threadwork.main_wait_for_done();
	CHECK_MESSAGE(sts.func1_count == 6,
			"Reader should have read both message sets");

	sts.destroy_threads();

	CHECK_MESSAGE(sts.func1_count == 6,
			"Reader should have read no additional messages after join");
	ProjectSettings::set_setting(COMMAND_QUEUE_SETTING,
			ProjectSettings::property_get_revert(COMMAND_QUEUE_SETTING));
}

TEST_CASE("[CommandQueue] Test Queue Lapping") {
	const char *COMMAND_QUEUE_SETTING = "memory/limits/command_queue/multithreading_queue_size_kb";
	ProjectSettings::set_setting(COMMAND_QUEUE_SETTING, 1);
	SharedThreadState sts;
	sts.init_threads();

	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC1_TRANSFORM);
	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC3_TRANSFORMx6);
	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC3_TRANSFORMx6);
	sts.writer_threadwork.main_start_work();
	sts.writer_threadwork.main_wait_for_done();

	// We need to read an extra message so that it triggers the dealloc logic once.
	// Otherwise, the queue will be considered full.
	sts.message_count_to_read = 3;
	sts.reader_threadwork.main_start_work();
	sts.reader_threadwork.main_wait_for_done();
	CHECK_MESSAGE(sts.func1_count == 3,
			"Reader should have read first set of messages");

	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC3_TRANSFORMx6);
	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC3_TRANSFORMx6);
	sts.writer_threadwork.main_start_work();
	// Don't wait for these, because the queue isn't big enough.
	sts.writer_threadwork.main_wait_for_done();

	sts.add_msg_to_write(SharedThreadState::TEST_MSG_FUNC2_TRANSFORM_FLOAT);
	sts.writer_threadwork.main_start_work();
	OS::delay_usec(1000);

	sts.message_count_to_read = 3;
	sts.reader_threadwork.main_start_work();
	sts.reader_threadwork.main_wait_for_done();

	sts.writer_threadwork.main_wait_for_done();

	sts.message_count_to_read = -1;
	sts.reader_threadwork.main_start_work();
	sts.reader_threadwork.main_wait_for_done();

	CHECK_MESSAGE(sts.func1_count == 6,
			"Reader should have read rest of the messages after lapping writers.");

	sts.destroy_threads();

	CHECK_MESSAGE(sts.func1_count == 6,
			"Reader should have read no additional messages after join");
	ProjectSettings::set_setting(COMMAND_QUEUE_SETTING,
			ProjectSettings::property_get_revert(COMMAND_QUEUE_SETTING));
}

TEST_CASE("[CommandQueue] Test Parameter Passing Semantics") {
	SharedThreadState sts;
	sts.init_threads();

	SUBCASE("Testing with lvalue") {
		SharedThreadState::CopyMoveTestType::copy_count = 0;
		SharedThreadState::CopyMoveTestType::move_count = 0;

		SharedThreadState::CopyMoveTestType lvalue(42);

		SUBCASE("Pass by copy") {
			sts.command_queue.push(&sts, &SharedThreadState::copy_move_test_copy, lvalue);

			sts.message_count_to_read = -1;
			sts.reader_threadwork.main_start_work();
			sts.reader_threadwork.main_wait_for_done();

			CHECK(SharedThreadState::CopyMoveTestType::copy_count == 1);
			CHECK(SharedThreadState::CopyMoveTestType::move_count == 1);
		}

		SUBCASE("Pass by reference") {
			sts.command_queue.push(&sts, &SharedThreadState::copy_move_test_ref, lvalue);

			sts.message_count_to_read = -1;
			sts.reader_threadwork.main_start_work();
			sts.reader_threadwork.main_wait_for_done();

			CHECK(SharedThreadState::CopyMoveTestType::copy_count == 1);
			CHECK(SharedThreadState::CopyMoveTestType::move_count == 0);
		}
	}

	SUBCASE("Testing with rvalue") {
		SharedThreadState::CopyMoveTestType::copy_count = 0;
		SharedThreadState::CopyMoveTestType::move_count = 0;

		SUBCASE("Pass by copy") {
			sts.command_queue.push(&sts, &SharedThreadState::copy_move_test_copy,
					SharedThreadState::CopyMoveTestType(43));

			sts.message_count_to_read = -1;
			sts.reader_threadwork.main_start_work();
			sts.reader_threadwork.main_wait_for_done();

			CHECK(SharedThreadState::CopyMoveTestType::copy_count == 0);
			CHECK(SharedThreadState::CopyMoveTestType::move_count == 2);
		}

		SUBCASE("Pass by reference") {
			sts.command_queue.push(&sts, &SharedThreadState::copy_move_test_ref,
					SharedThreadState::CopyMoveTestType(43));

			sts.message_count_to_read = -1;
			sts.reader_threadwork.main_start_work();
			sts.reader_threadwork.main_wait_for_done();

			CHECK(SharedThreadState::CopyMoveTestType::copy_count == 0);
			CHECK(SharedThreadState::CopyMoveTestType::move_count == 1);
		}

		SUBCASE("Pass by rvalue reference") {
			sts.command_queue.push(&sts, &SharedThreadState::copy_move_test_move,
					SharedThreadState::CopyMoveTestType(43));

			sts.message_count_to_read = -1;
			sts.reader_threadwork.main_start_work();
			sts.reader_threadwork.main_wait_for_done();

			CHECK(SharedThreadState::CopyMoveTestType::copy_count == 0);
			CHECK(SharedThreadState::CopyMoveTestType::move_count == 1);
		}
	}

	sts.destroy_threads();
}

} // namespace TestCommandQueue

// === Merged from tests/core/templates/test_fixed_vector.cpp ===
/*  test_fixed_vector.cpp                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/span.h"

namespace TestFixedVector {

struct MoveOnly {
	bool is_alive = true;

	MoveOnly() = default;
	MoveOnly(const MoveOnly &p) = delete;
	MoveOnly &operator=(const MoveOnly &p) = delete;

	MoveOnly(MoveOnly &&p) {
		is_alive = p.is_alive;
		p.is_alive = false;
	}
	MoveOnly &operator=(MoveOnly &&p) {
		if (&p == this) {
			return *this;
		}
		is_alive = p.is_alive;
		p.is_alive = false;
		return *this;
	}
};

static_assert(!std::is_trivially_copyable_v<MoveOnly>);
static_assert(!std::is_trivially_constructible_v<MoveOnly>);

TEST_CASE("[FixedVector] Basic Checks") {
	FixedVector<uint16_t, 1> vector;
	CHECK_EQ(vector.capacity(), 1);

	CHECK_EQ(vector.size(), 0);
	CHECK(vector.is_empty());
	CHECK(!vector.is_full());

	vector.push_back(5);
	CHECK_EQ(vector.size(), 1);
	CHECK_EQ(vector[0], 5);
	CHECK_EQ(vector.ptr()[0], 5);
	CHECK(!vector.is_empty());
	CHECK(vector.is_full());

	vector.pop_back();
	CHECK_EQ(vector.size(), 0);
	CHECK(vector.is_empty());
	CHECK(!vector.is_full());

	FixedVector<uint16_t, 2> vector1 = { 1, 2 };
	CHECK_EQ(vector1.capacity(), 2);
	CHECK_EQ(vector1.size(), 2);
	CHECK_EQ(vector1[0], 1);
	CHECK_EQ(vector1[1], 2);

	FixedVector<Variant, 3> vector_variant;
	CHECK_EQ(vector_variant.size(), 0);
	CHECK_EQ(vector_variant.capacity(), 3);
	vector_variant.resize_initialized(3);
	vector_variant[0] = "Test";
	vector_variant[1] = 1;
	CHECK_EQ(vector_variant.capacity(), 3);
	CHECK_EQ(vector_variant.size(), 3);
	CHECK_EQ(vector_variant[0], "Test");
	CHECK_EQ(vector_variant[1], Variant(1));
	CHECK_EQ(vector_variant[2].get_type(), Variant::NIL);

	// Test that move-only types are transferred.
	FixedVector<MoveOnly, 1> a;
	a.push_back(MoveOnly());
	CHECK_EQ(a.size(), 1);
	FixedVector<MoveOnly, 1> b(std::move(a));
	CHECK_EQ(a.size(), 0);
	CHECK_EQ(b.size(), 1);
	a = std::move(b);
	CHECK_EQ(a.size(), 1);
	CHECK_EQ(b.size(), 0);
}

TEST_CASE("[FixedVector] Alignment Checks") {
	FixedVector<uint16_t, 4> vector_uint16;
	vector_uint16.resize_uninitialized(4);
	CHECK((size_t)&vector_uint16[0] % alignof(uint16_t) == 0);
	CHECK((size_t)&vector_uint16[1] % alignof(uint16_t) == 0);
	CHECK((size_t)&vector_uint16[2] % alignof(uint16_t) == 0);
	CHECK((size_t)&vector_uint16[3] % alignof(uint16_t) == 0);

	FixedVector<uint32_t, 4> vector_uint32;
	vector_uint32.resize_uninitialized(4);
	CHECK((size_t)&vector_uint32[0] % alignof(uint32_t) == 0);
	CHECK((size_t)&vector_uint32[1] % alignof(uint32_t) == 0);
	CHECK((size_t)&vector_uint32[2] % alignof(uint32_t) == 0);
	CHECK((size_t)&vector_uint32[3] % alignof(uint32_t) == 0);

	FixedVector<uint64_t, 4> vector_uint64;
	vector_uint64.resize_uninitialized(4);
	CHECK((size_t)&vector_uint64[0] % alignof(uint64_t) == 0);
	CHECK((size_t)&vector_uint64[1] % alignof(uint64_t) == 0);
	CHECK((size_t)&vector_uint64[2] % alignof(uint64_t) == 0);
	CHECK((size_t)&vector_uint64[3] % alignof(uint64_t) == 0);
}

} // namespace TestFixedVector

// === Merged from tests/core/templates/test_hash_map.cpp ===
/*  test_hash_map.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/hash_map.h"

namespace TestHashMap {

TEST_CASE("[HashMap] List initialization") {
	HashMap<int, String> map{ { 0, "A" }, { 1, "B" }, { 2, "C" }, { 3, "D" }, { 4, "E" } };

	CHECK(map.size() == 5);
	CHECK(map[0] == "A");
	CHECK(map[1] == "B");
	CHECK(map[2] == "C");
	CHECK(map[3] == "D");
	CHECK(map[4] == "E");
}

TEST_CASE("[HashMap] List initialization with existing elements") {
	HashMap<int, String> map{ { 0, "A" }, { 0, "B" }, { 0, "C" }, { 0, "D" }, { 0, "E" } };

	CHECK(map.size() == 1);
	CHECK(map[0] == "E");
}

TEST_CASE("[HashMap] Insert element") {
	HashMap<int, int> map;
	HashMap<int, int>::Iterator e = map.insert(42, 84);

	CHECK(e);
	CHECK(e->key == 42);
	CHECK(e->value == 84);
	CHECK(map[42] == 84);
	CHECK(map.has(42));
	CHECK(map.find(42));
}

TEST_CASE("[HashMap] Overwrite element") {
	HashMap<int, int> map;
	map.insert(42, 84);
	map.insert(42, 1234);

	CHECK(map[42] == 1234);
}

TEST_CASE("[HashMap] Erase via element") {
	HashMap<int, int> map;
	HashMap<int, int>::Iterator e = map.insert(42, 84);
	map.remove(e);
	CHECK(!map.has(42));
	CHECK(!map.find(42));
}

TEST_CASE("[HashMap] Erase via key") {
	HashMap<int, int> map;
	map.insert(42, 84);
	map.erase(42);
	CHECK(!map.has(42));
	CHECK(!map.find(42));
}

TEST_CASE("[HashMap] Size") {
	HashMap<int, int> map;
	map.insert(42, 84);
	map.insert(123, 84);
	map.insert(123, 84);
	map.insert(0, 84);
	map.insert(123485, 84);

	CHECK(map.size() == 4);
}

TEST_CASE("[HashMap] Iteration") {
	HashMap<int, int> map;
	map.insert(42, 84);
	map.insert(123, 12385);
	map.insert(0, 12934);
	map.insert(123485, 1238888);
	map.insert(123, 111111);

	Vector<Pair<int, int>> expected;
	expected.push_back(Pair<int, int>(42, 84));
	expected.push_back(Pair<int, int>(123, 111111));
	expected.push_back(Pair<int, int>(0, 12934));
	expected.push_back(Pair<int, int>(123485, 1238888));

	int idx = 0;
	for (const KeyValue<int, int> &E : map) {
		CHECK(expected[idx] == Pair<int, int>(E.key, E.value));
		++idx;
	}
}

TEST_CASE("[HashMap] Const iteration") {
	HashMap<int, int> map;
	map.insert(42, 84);
	map.insert(123, 12385);
	map.insert(0, 12934);
	map.insert(123485, 1238888);
	map.insert(123, 111111);

	const HashMap<int, int> const_map(map);

	Vector<Pair<int, int>> expected;
	expected.push_back(Pair<int, int>(42, 84));
	expected.push_back(Pair<int, int>(123, 111111));
	expected.push_back(Pair<int, int>(0, 12934));
	expected.push_back(Pair<int, int>(123485, 1238888));
	expected.push_back(Pair<int, int>(123, 111111));

	int idx = 0;
	for (const KeyValue<int, int> &E : const_map) {
		CHECK(expected[idx] == Pair<int, int>(E.key, E.value));
		++idx;
	}
}

TEST_CASE("[HashMap] Sort") {
	HashMap<int, int> hashmap;
	int shuffled_ints[]{ 6, 1, 9, 8, 3, 0, 4, 5, 7, 2 };

	for (int i : shuffled_ints) {
		hashmap[i] = i;
	}
	hashmap.sort();

	int i = 0;
	for (const KeyValue<int, int> &kv : hashmap) {
		CHECK_EQ(kv.key, i);
		i++;
	}

	struct ReverseSort {
		bool operator()(const KeyValue<int, int> &p_a, const KeyValue<int, int> &p_b) {
			return p_a.key > p_b.key;
		}
	};
	hashmap.sort_custom<ReverseSort>();

	for (const KeyValue<int, int> &kv : hashmap) {
		i--;
		CHECK_EQ(kv.key, i);
	}
}

} // namespace TestHashMap

// === Merged from tests/core/templates/test_hash_set.cpp ===
/*  test_hash_set.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/hash_set.h"

namespace TestHashSet {

TEST_CASE("[HashSet] List initialization") {
	HashSet<int> set{ 0, 1, 2, 3, 4 };

	CHECK(set.size() == 5);
	CHECK(set.has(0));
	CHECK(set.has(1));
	CHECK(set.has(2));
	CHECK(set.has(3));
	CHECK(set.has(4));
}

TEST_CASE("[HashSet] List initialization with existing elements") {
	HashSet<int> set{ 0, 0, 0, 0, 0 };

	CHECK(set.size() == 1);
	CHECK(set.has(0));
}

TEST_CASE("[HashSet] Insert element") {
	HashSet<int> set;
	HashSet<int>::Iterator e = set.insert(42);

	CHECK(e);
	CHECK(*e == 42);
	CHECK(set.has(42));
	CHECK(set.find(42));
	set.reset();
}

TEST_CASE("[HashSet] Insert existing element") {
	HashSet<int> set;
	set.insert(42);
	set.insert(42);

	CHECK(set.has(42));
	CHECK(set.size() == 1);
}

TEST_CASE("[HashSet] Insert, iterate and remove many elements") {
	const int elem_max = 12343;
	HashSet<int> set;
	for (int i = 0; i < elem_max; i++) {
		set.insert(i);
	}

	//insert order should have been kept
	int idx = 0;
	for (const int &K : set) {
		CHECK(idx == K);
		CHECK(set.has(idx));
		idx++;
	}

	Vector<int> elems_still_valid;

	for (int i = 0; i < elem_max; i++) {
		if ((i % 5) == 0) {
			set.erase(i);
		} else {
			elems_still_valid.push_back(i);
		}
	}

	CHECK(elems_still_valid.size() == set.size());

	for (int i = 0; i < elems_still_valid.size(); i++) {
		CHECK(set.has(elems_still_valid[i]));
	}
}

TEST_CASE("[HashSet] Insert, iterate and remove many strings") {
	// This tests a key that uses allocation, to see if any leaks occur

	uint64_t pre_mem = Memory::get_mem_usage();
	const int elem_max = 4018;
	HashSet<String> set;
	for (int i = 0; i < elem_max; i++) {
		set.insert(itos(i));
	}

	//insert order should have been kept
	int idx = 0;
	for (const String &K : set) {
		CHECK(itos(idx) == K);
		CHECK(set.has(itos(idx)));
		idx++;
	}

	Vector<String> elems_still_valid;

	for (int i = 0; i < elem_max; i++) {
		if ((i % 5) == 0) {
			set.erase(itos(i));
		} else {
			elems_still_valid.push_back(itos(i));
		}
	}

	CHECK(elems_still_valid.size() == set.size());

	for (int i = 0; i < elems_still_valid.size(); i++) {
		CHECK(set.has(elems_still_valid[i]));
	}

	elems_still_valid.clear();
	set.reset();

	CHECK(Memory::get_mem_usage() == pre_mem);
}

TEST_CASE("[HashSet] Erase via element") {
	HashSet<int> set;
	HashSet<int>::Iterator e = set.insert(42);
	set.remove(e);
	CHECK(!set.has(42));
	CHECK(!set.find(42));
}

TEST_CASE("[HashSet] Erase via key") {
	HashSet<int> set;
	set.insert(42);
	set.insert(49);
	set.erase(42);
	CHECK(!set.has(42));
	CHECK(!set.find(42));
}

TEST_CASE("[HashSet] Insert and erase half elements") {
	HashSet<int> set;
	set.insert(1);
	set.insert(2);
	set.insert(3);
	set.insert(4);
	set.erase(1);
	set.erase(3);

	CHECK(set.size() == 2);
	CHECK(set.has(2));
	CHECK(set.has(4));
}

TEST_CASE("[HashSet] Size") {
	HashSet<int> set;
	set.insert(42);
	set.insert(123);
	set.insert(123);
	set.insert(0);
	set.insert(123485);

	CHECK(set.size() == 4);
}

TEST_CASE("[HashSet] Iteration") {
	HashSet<int> set;
	set.insert(42);
	set.insert(123);
	set.insert(0);
	set.insert(123485);

	Vector<int> expected;
	expected.push_back(42);
	expected.push_back(123);
	expected.push_back(0);
	expected.push_back(123485);

	int idx = 0;
	for (const int &E : set) {
		CHECK(expected[idx] == E);
		++idx;
	}
}

TEST_CASE("[HashSet] Copy") {
	HashSet<int> set;
	set.insert(42);
	set.insert(123);
	set.insert(0);
	set.insert(123485);

	Vector<int> expected;
	expected.push_back(42);
	expected.push_back(123);
	expected.push_back(0);
	expected.push_back(123485);

	HashSet<int> copy_assign;
	copy_assign = set;

	int idx = 0;
	for (const int &E : copy_assign) {
		CHECK(expected[idx] == E);
		++idx;
	}

	HashSet<int> copy_construct(set);

	idx = 0;
	for (const int &E : copy_construct) {
		CHECK(expected[idx] == E);
		++idx;
	}
}

TEST_CASE("[HashSet] Equality") {
	// Empty sets.
	CHECK(HashSet<int>{} == HashSet<int>{});
	CHECK(HashSet<int>{} != HashSet<int>{ 1, 2, 3 });
	CHECK(HashSet<int>{ 1, 2, 3 } != HashSet<int>{});

	// Different length.
	CHECK(HashSet<int>{ 1, 2, 3 } != HashSet<int>{ 1, 2, 3, 4 });
	CHECK(HashSet<int>{ 1, 2, 3, 4 } != HashSet<int>{ 4, 3, 2 });

	// Same length.
	CHECK(HashSet<int>{ 1, 2, 3 } == HashSet<int>{ 1, 2, 3 });
	CHECK(HashSet<int>{ 1, 2, 3 } == HashSet<int>{ 3, 2, 1 });
	CHECK(HashSet<int>{ 1, 2, 3 } != HashSet<int>{ 1, 2, 8 });
}

} // namespace TestHashSet

// === Merged from tests/core/templates/test_list.cpp ===
/*  test_list.cpp                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/list.h"

namespace TestList {

static void populate_integers(List<int> &p_list, List<int>::Element *r_elements[], int num_elements) {
	p_list.clear();
	for (int i = 0; i < num_elements; ++i) {
		List<int>::Element *n = p_list.push_back(i);
		r_elements[i] = n;
	}
}

TEST_CASE("[List] List initialization") {
	List<int> list{ 0, 1, 2, 3, 4 };

	CHECK(list.size() == 5);
	CHECK(list.get(0) == 0);
	CHECK(list.get(1) == 1);
	CHECK(list.get(2) == 2);
	CHECK(list.get(3) == 3);
	CHECK(list.get(4) == 4);
}

TEST_CASE("[List] Push/pop back") {
	List<String> list;

	List<String>::Element *n;
	n = list.push_back("A");
	CHECK(n->get() == "A");
	n = list.push_back("B");
	CHECK(n->get() == "B");
	n = list.push_back("C");
	CHECK(n->get() == "C");

	CHECK(list.size() == 3);
	CHECK(!list.is_empty());

	String v;
	v = list.back()->get();
	list.pop_back();
	CHECK(v == "C");
	v = list.back()->get();
	list.pop_back();
	CHECK(v == "B");
	v = list.back()->get();
	list.pop_back();
	CHECK(v == "A");

	CHECK(list.size() == 0);
	CHECK(list.is_empty());

	CHECK(list.back() == nullptr);
	CHECK(list.front() == nullptr);
}

TEST_CASE("[List] Push/pop front") {
	List<String> list;

	List<String>::Element *n;
	n = list.push_front("A");
	CHECK(n->get() == "A");
	n = list.push_front("B");
	CHECK(n->get() == "B");
	n = list.push_front("C");
	CHECK(n->get() == "C");

	CHECK(list.size() == 3);
	CHECK(!list.is_empty());

	String v;
	v = list.front()->get();
	list.pop_front();
	CHECK(v == "C");
	v = list.front()->get();
	list.pop_front();
	CHECK(v == "B");
	v = list.front()->get();
	list.pop_front();
	CHECK(v == "A");

	CHECK(list.size() == 0);
	CHECK(list.is_empty());

	CHECK(list.back() == nullptr);
	CHECK(list.front() == nullptr);
}

TEST_CASE("[List] Set and get") {
	List<String> list;
	list.push_back("A");

	List<String>::Element *n = list.front();
	CHECK(n->get() == "A");

	n->set("X");
	CHECK(n->get() == "X");
}

TEST_CASE("[List] Insert before") {
	List<String> list;
	List<String>::Element *a = list.push_back("A");
	List<String>::Element *b = list.push_back("B");
	List<String>::Element *c = list.push_back("C");

	list.insert_before(b, "I");

	CHECK(a->next()->get() == "I");
	CHECK(c->prev()->prev()->get() == "I");
	CHECK(list.front()->next()->get() == "I");
	CHECK(list.back()->prev()->prev()->get() == "I");
}

TEST_CASE("[List] Insert after") {
	List<String> list;
	List<String>::Element *a = list.push_back("A");
	List<String>::Element *b = list.push_back("B");
	List<String>::Element *c = list.push_back("C");

	list.insert_after(b, "I");

	CHECK(a->next()->next()->get() == "I");
	CHECK(c->prev()->get() == "I");
	CHECK(list.front()->next()->next()->get() == "I");
	CHECK(list.back()->prev()->get() == "I");
}

TEST_CASE("[List] Insert before null") {
	List<String> list;
	List<String>::Element *a = list.push_back("A");
	List<String>::Element *b = list.push_back("B");
	List<String>::Element *c = list.push_back("C");

	list.insert_before(nullptr, "I");

	CHECK(a->next()->get() == "B");
	CHECK(b->get() == "B");
	CHECK(c->prev()->prev()->get() == "A");
	CHECK(list.front()->next()->get() == "B");
	CHECK(list.back()->prev()->prev()->get() == "B");
	CHECK(list.back()->get() == "I");
}

TEST_CASE("[List] Insert after null") {
	List<String> list;
	List<String>::Element *a = list.push_back("A");
	List<String>::Element *b = list.push_back("B");
	List<String>::Element *c = list.push_back("C");

	list.insert_after(nullptr, "I");

	CHECK(a->next()->get() == "B");
	CHECK(b->get() == "B");
	CHECK(c->prev()->prev()->get() == "A");
	CHECK(list.front()->next()->get() == "B");
	CHECK(list.back()->prev()->prev()->get() == "B");
	CHECK(list.back()->get() == "I");
}

TEST_CASE("[List] Find") {
	List<int> list;
	List<int>::Element *n[10];
	// Indices match values.
	populate_integers(list, n, 10);

	for (int i = 0; i < 10; ++i) {
		CHECK(n[i]->get() == list.find(i)->get());
	}
}

TEST_CASE("[List] Erase (by value)") {
	List<int> list;
	List<int>::Element *n[4];
	// Indices match values.
	populate_integers(list, n, 4);

	CHECK(list.front()->next()->next()->get() == 2);
	bool erased = list.erase(2); // 0, 1, 3.
	CHECK(erased);
	CHECK(list.size() == 3);

	// The pointer n[2] points to the freed memory which is not reset to zero,
	// so the below assertion may pass, but this relies on undefined behavior.
	// CHECK(n[2]->get() == 2);

	CHECK(list.front()->get() == 0);
	CHECK(list.front()->next()->next()->get() == 3);
	CHECK(list.back()->get() == 3);
	CHECK(list.back()->prev()->get() == 1);

	CHECK(n[1]->next()->get() == 3);
	CHECK(n[3]->prev()->get() == 1);

	erased = list.erase(9000); // Doesn't exist.
	CHECK(!erased);
}

TEST_CASE("[List] Erase (by element)") {
	List<int> list;
	List<int>::Element *n[4];
	// Indices match values.
	populate_integers(list, n, 4);

	bool erased = list.erase(n[2]);
	CHECK(erased);
	CHECK(list.size() == 3);
	CHECK(n[1]->next()->get() == 3);
	CHECK(n[3]->prev()->get() == 1);
}

TEST_CASE("[List] Element erase") {
	List<int> list;
	List<int>::Element *n[4];
	// Indices match values.
	populate_integers(list, n, 4);

	n[2]->erase();

	CHECK(list.size() == 3);
	CHECK(n[1]->next()->get() == 3);
	CHECK(n[3]->prev()->get() == 1);
}

TEST_CASE("[List] Clear") {
	List<int> list;
	List<int>::Element *n[100];
	populate_integers(list, n, 100);

	list.clear();

	CHECK(list.size() == 0);
	CHECK(list.is_empty());
}

TEST_CASE("[List] Invert") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.reverse();

	CHECK(list.front()->get() == 3);
	CHECK(list.front()->next()->get() == 2);
	CHECK(list.back()->prev()->get() == 1);
	CHECK(list.back()->get() == 0);
}

TEST_CASE("[List] Move to front") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.move_to_front(n[3]);

	CHECK(list.front()->get() == 3);
	CHECK(list.back()->get() == 2);
}

TEST_CASE("[List] Move to back") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.move_to_back(n[0]);

	CHECK(list.back()->get() == 0);
	CHECK(list.front()->get() == 1);
}

TEST_CASE("[List] Move before") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.move_before(n[3], n[1]);

	CHECK(list.front()->next()->get() == n[3]->get());
}

template <typename T>
static void compare_lists(const List<T> &p_result, const List<T> &p_expected) {
	CHECK_EQ(p_result.size(), p_expected.size());
	const typename List<T>::Element *result_it = p_result.front();
	const typename List<T>::Element *expected_it = p_expected.front();
	for (int i = 0; i < p_result.size(); i++) {
		CHECK(result_it);
		CHECK(expected_it);
		CHECK_EQ(result_it->get(), expected_it->get());
		result_it = result_it->next();
		expected_it = expected_it->next();
	}
	CHECK(!result_it);
	CHECK(!expected_it);

	result_it = p_result.back();
	expected_it = p_expected.back();
	for (int i = 0; i < p_result.size(); i++) {
		CHECK(result_it);
		CHECK(expected_it);
		CHECK_EQ(result_it->get(), expected_it->get());
		result_it = result_it->prev();
		expected_it = expected_it->prev();
	}
	CHECK(!result_it);
	CHECK(!expected_it);
}

TEST_CASE("[List] Sort") {
	List<String> result{ "D", "B", "A", "C" };
	result.sort();
	List<String> expected{ "A", "B", "C", "D" };
	compare_lists(result, expected);

	List<int> empty_result{};
	empty_result.sort();
	List<int> empty_expected{};
	compare_lists(empty_result, empty_expected);

	List<int> one_result{ 1 };
	one_result.sort();
	List<int> one_expected{ 1 };
	compare_lists(one_result, one_expected);

	List<float> reversed_result{ 2.0, 1.5, 1.0 };
	reversed_result.sort();
	List<float> reversed_expected{ 1.0, 1.5, 2.0 };
	compare_lists(reversed_result, reversed_expected);

	List<int> already_sorted_result{ 1, 2, 3, 4, 5 };
	already_sorted_result.sort();
	List<int> already_sorted_expected{ 1, 2, 3, 4, 5 };
	compare_lists(already_sorted_result, already_sorted_expected);

	List<int> with_duplicates_result{ 1, 2, 3, 1, 2, 3 };
	with_duplicates_result.sort();
	List<int> with_duplicates_expected{ 1, 1, 2, 2, 3, 3 };
	compare_lists(with_duplicates_result, with_duplicates_expected);
}

TEST_CASE("[List] Swap adjacent front and back") {
	List<int> list;
	List<int>::Element *n[2];
	populate_integers(list, n, 2);

	list.swap(list.front(), list.back());

	CHECK(list.front()->prev() == nullptr);
	CHECK(list.front() != list.front()->next());

	CHECK(list.front() == n[1]);
	CHECK(list.back() == n[0]);

	CHECK(list.back()->next() == nullptr);
	CHECK(list.back() != list.back()->prev());
}

TEST_CASE("[List] Swap first adjacent pair") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.swap(n[0], n[1]);

	CHECK(list.front()->prev() == nullptr);
	CHECK(list.front() != list.front()->next());

	CHECK(list.front() == n[1]);
	CHECK(list.front()->next() == n[0]);
	CHECK(list.back()->prev() == n[2]);
	CHECK(list.back() == n[3]);

	CHECK(list.back()->next() == nullptr);
	CHECK(list.back() != list.back()->prev());
}

TEST_CASE("[List] Swap middle adjacent pair") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.swap(n[1], n[2]);

	CHECK(list.front()->prev() == nullptr);

	CHECK(list.front() == n[0]);
	CHECK(list.front()->next() == n[2]);
	CHECK(list.back()->prev() == n[1]);
	CHECK(list.back() == n[3]);

	CHECK(list.back()->next() == nullptr);
}

TEST_CASE("[List] Swap last adjacent pair") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.swap(n[2], n[3]);

	CHECK(list.front()->prev() == nullptr);

	CHECK(list.front() == n[0]);
	CHECK(list.front()->next() == n[1]);
	CHECK(list.back()->prev() == n[3]);
	CHECK(list.back() == n[2]);

	CHECK(list.back()->next() == nullptr);
}

TEST_CASE("[List] Swap first cross pair") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.swap(n[0], n[2]);

	CHECK(list.front()->prev() == nullptr);

	CHECK(list.front() == n[2]);
	CHECK(list.front()->next() == n[1]);
	CHECK(list.back()->prev() == n[0]);
	CHECK(list.back() == n[3]);

	CHECK(list.back()->next() == nullptr);
}

TEST_CASE("[List] Swap last cross pair") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.swap(n[1], n[3]);

	CHECK(list.front()->prev() == nullptr);

	CHECK(list.front() == n[0]);
	CHECK(list.front()->next() == n[3]);
	CHECK(list.back()->prev() == n[2]);
	CHECK(list.back() == n[1]);

	CHECK(list.back()->next() == nullptr);
}

TEST_CASE("[List] Swap edges") {
	List<int> list;
	List<int>::Element *n[4];
	populate_integers(list, n, 4);

	list.swap(n[1], n[3]);

	CHECK(list.front()->prev() == nullptr);

	CHECK(list.front() == n[0]);
	CHECK(list.front()->next() == n[3]);
	CHECK(list.back()->prev() == n[2]);
	CHECK(list.back() == n[1]);

	CHECK(list.back()->next() == nullptr);
}

TEST_CASE("[List] Swap middle (values check)") {
	List<String> list;
	List<String>::Element *n_str1 = list.push_back("Still");
	List<String>::Element *n_str2 = list.push_back("waiting");
	List<String>::Element *n_str3 = list.push_back("for");
	List<String>::Element *n_str4 = list.push_back("Godot.");

	CHECK(n_str1->get() == "Still");
	CHECK(n_str4->get() == "Godot.");

	CHECK(list.front()->get() == "Still");
	CHECK(list.front()->next()->get() == "waiting");
	CHECK(list.back()->prev()->get() == "for");
	CHECK(list.back()->get() == "Godot.");

	list.swap(n_str2, n_str3);

	CHECK(list.front()->next()->get() == "for");
	CHECK(list.back()->prev()->get() == "waiting");
}

TEST_CASE("[List] Swap front and back (values check)") {
	List<Variant> list;
	Variant str = "Godot";
	List<Variant>::Element *n_str = list.push_back(str);
	Variant color = Color(0, 0, 1);
	List<Variant>::Element *n_color = list.push_back(color);

	CHECK(list.front()->get() == "Godot");
	CHECK(list.back()->get() == Color(0, 0, 1));

	list.swap(n_str, n_color);

	CHECK(list.front()->get() == Color(0, 0, 1));
	CHECK(list.back()->get() == "Godot");
}

TEST_CASE("[List] Swap adjacent back and front (reverse order of elements)") {
	List<int> list;
	List<int>::Element *n[2];
	populate_integers(list, n, 2);

	list.swap(n[1], n[0]);

	List<int>::Element *it = list.front();
	while (it) {
		List<int>::Element *prev_it = it;
		it = it->next();
		if (it == prev_it) {
			FAIL_CHECK("Infinite loop detected.");
			break;
		}
	}
}

} // namespace TestList

// === Merged from tests/core/templates/test_local_vector.cpp ===
/*  test_local_vector.cpp                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/local_vector.h"

namespace TestLocalVector {

TEST_CASE("[LocalVector] List Initialization.") {
	LocalVector<int> vector{ 0, 1, 2, 3, 4 };

	CHECK(vector.size() == 5);
	CHECK(vector[0] == 0);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 2);
	CHECK(vector[3] == 3);
	CHECK(vector[4] == 4);
}

TEST_CASE("[LocalVector] Push Back.") {
	LocalVector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	CHECK(vector[0] == 0);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 2);
	CHECK(vector[3] == 3);
	CHECK(vector[4] == 4);
}

TEST_CASE("[LocalVector] Find, has.") {
	LocalVector<int> vector;
	vector.push_back(3);
	vector.push_back(1);
	vector.push_back(4);
	vector.push_back(0);
	vector.push_back(2);

	CHECK(vector[0] == 3);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 4);
	CHECK(vector[3] == 0);
	CHECK(vector[4] == 2);

	CHECK(vector.find(0) == 3);
	CHECK(vector.find(1) == 1);
	CHECK(vector.find(2) == 4);
	CHECK(vector.find(3) == 0);
	CHECK(vector.find(4) == 2);

	CHECK(vector.find(-1) == -1);
	CHECK(vector.find(5) == -1);

	CHECK(vector.has(0));
	CHECK(vector.has(1));
	CHECK(vector.has(2));
	CHECK(vector.has(3));
	CHECK(vector.has(4));

	CHECK(!vector.has(-1));
	CHECK(!vector.has(5));
}

TEST_CASE("[LocalVector] Remove.") {
	LocalVector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	vector.remove_at(0);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 3);
	CHECK(vector[3] == 4);

	vector.remove_at(2);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 4);

	vector.remove_at(1);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 4);

	vector.remove_at(0);

	CHECK(vector[0] == 4);
}

TEST_CASE("[LocalVector] Remove Unordered.") {
	LocalVector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	CHECK(vector.size() == 5);

	vector.remove_at_unordered(0);

	CHECK(vector.size() == 4);

	CHECK(vector.find(0) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(2) != -1);
	CHECK(vector.find(3) != -1);
	CHECK(vector.find(4) != -1);

	// Now the vector is no more ordered.
	vector.remove_at_unordered(vector.find(3));

	CHECK(vector.size() == 3);

	CHECK(vector.find(3) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(2) != -1);
	CHECK(vector.find(4) != -1);

	vector.remove_at_unordered(vector.find(2));

	CHECK(vector.size() == 2);

	CHECK(vector.find(2) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(4) != -1);

	vector.remove_at_unordered(vector.find(4));

	CHECK(vector.size() == 1);

	CHECK(vector.find(4) == -1);
	CHECK(vector.find(1) != -1);

	// Remove the last one.
	vector.remove_at_unordered(0);

	CHECK(vector.is_empty());
	CHECK(vector.size() == 0);
}

TEST_CASE("[LocalVector] Erase Unordered.") {
	LocalVector<int> vector;
	vector.push_back(1);
	vector.push_back(3);
	vector.push_back(0);
	vector.push_back(2);
	vector.push_back(4);

	CHECK(vector.find(1) == 0);

	vector.erase_unordered(1);

	CHECK(vector.find(1) == -1);
	CHECK(vector.size() == 4);
	CHECK(vector[0] == 4);
}

TEST_CASE("[LocalVector] Erase.") {
	LocalVector<int> vector;
	vector.push_back(1);
	vector.push_back(3);
	vector.push_back(0);
	vector.push_back(2);
	vector.push_back(4);

	CHECK(vector.find(2) == 3);

	vector.erase(2);

	CHECK(vector.find(2) == -1);
	CHECK(vector.size() == 4);
}

TEST_CASE("[LocalVector] Size / Resize / Reserve.") {
	LocalVector<int> vector;

	CHECK(vector.is_empty());
	CHECK(vector.size() == 0);
	CHECK(vector.get_capacity() == 0);

	vector.resize(10);

	CHECK(vector.size() == 10);
	CHECK(vector.get_capacity() >= 10);

	vector.resize(5);

	CHECK(vector.size() == 5);
	// Capacity is supposed to change only when the size increase.
	CHECK(vector.get_capacity() >= 10);

	vector.remove_at(0);
	vector.remove_at(0);
	vector.remove_at(0);

	CHECK(vector.size() == 2);
	// Capacity is supposed to change only when the size increase.
	CHECK(vector.get_capacity() >= 10);

	vector.reset();

	CHECK(vector.size() == 0);
	CHECK(vector.get_capacity() == 0);

	vector.reserve(3);

	CHECK(vector.is_empty());
	CHECK(vector.size() == 0);
	CHECK(vector.get_capacity() >= 3);

	vector.push_back(0);
	vector.push_back(0);
	vector.push_back(0);

	CHECK(vector.size() == 3);
	CHECK(vector.get_capacity() >= 3);

	vector.push_back(0);

	CHECK(vector.size() == 4);
	CHECK(vector.get_capacity() >= 4);
}

} // namespace TestLocalVector

// === Merged from tests/core/templates/test_lru.cpp ===
/*  test_lru.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/hash_map.h"

namespace TestLRU {

TEST_CASE("[LRU] Store and read") {
	LRUCache<int, int> lru;

	lru.set_capacity(3);
	lru.insert(1, 1);
	lru.insert(50, 2);
	lru.insert(100, 5);

	CHECK(lru.has(1));
	CHECK(lru.has(50));
	CHECK(lru.has(100));
	CHECK(!lru.has(200));

	CHECK(lru.get(1) == 1);
	CHECK(lru.get(50) == 2);
	CHECK(lru.get(100) == 5);

	CHECK(lru.getptr(1) != nullptr);
	CHECK(lru.getptr(1000) == nullptr);

	lru.insert(600, 600); // Erase <50>
	CHECK(lru.has(600));
	CHECK(!lru.has(50));
}

TEST_CASE("[LRU] Resize and clear") {
	LRUCache<int, int> lru;

	lru.set_capacity(3);
	lru.insert(1, 1);
	lru.insert(2, 2);
	lru.insert(3, 3);

	CHECK(lru.get_capacity() == 3);

	lru.set_capacity(5);
	CHECK(lru.get_capacity() == 5);

	CHECK(lru.has(1));
	CHECK(lru.has(2));
	CHECK(lru.has(3));
	CHECK(!lru.has(4));

	lru.set_capacity(2);
	CHECK(lru.get_capacity() == 2);

	CHECK(!lru.has(1));
	CHECK(lru.has(2));
	CHECK(lru.has(3));
	CHECK(!lru.has(4));

	lru.clear();
	CHECK(!lru.has(1));
	CHECK(!lru.has(2));
	CHECK(!lru.has(3));
	CHECK(!lru.has(4));
}

} // namespace TestLRU

// === Merged from tests/core/templates/test_paged_array.cpp ===
/*  test_paged_array.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestPagedArray {

TEST_CASE("[PagedArray] Simple fill and refill") {
	PagedArrayPool<uint32_t> pool;
	PagedArray<uint32_t> array;
	array.set_page_pool(&pool);

	for (uint32_t i = 0; i < 123456; i++) {
		array.push_back(i);
	}
	CHECK_MESSAGE(
			array.size() == 123456,
			"PagedArray should have 123456 elements.");

	bool all_match = true;
	for (uint32_t i = 0; i < 123456; i++) {
		if (array[i] != i) {
			all_match = false;
			break;
		}
	}

	CHECK_MESSAGE(
			all_match,
			"PagedArray elements should match from 0 to 123455.");

	array.clear();

	CHECK_MESSAGE(
			array.size() == 0,
			"PagedArray elements should be 0 after clear.");

	for (uint32_t i = 0; i < 999; i++) {
		array.push_back(i);
	}
	CHECK_MESSAGE(
			array.size() == 999,
			"PagedArray should have 999 elements.");

	all_match = true;
	for (uint32_t i = 0; i < 999; i++) {
		if (array[i] != i) {
			all_match = false;
		}
	}

	CHECK_MESSAGE(
			all_match,
			"PagedArray elements should match from 0 to 998.");

	array.reset(); //reset so pagepool can be reset
	pool.reset();
}

TEST_CASE("[PagedArray] Shared pool fill, including merging") {
	PagedArrayPool<uint32_t> pool;
	PagedArray<uint32_t> array1;
	PagedArray<uint32_t> array2;
	array1.set_page_pool(&pool);
	array2.set_page_pool(&pool);

	for (uint32_t i = 0; i < 123456; i++) {
		array1.push_back(i);
	}
	CHECK_MESSAGE(
			array1.size() == 123456,
			"PagedArray #1 should have 123456 elements.");

	bool all_match = true;
	for (uint32_t i = 0; i < 123456; i++) {
		if (array1[i] != i) {
			all_match = false;
		}
	}

	CHECK_MESSAGE(
			all_match,
			"PagedArray #1 elements should match from 0 to 123455.");

	for (uint32_t i = 0; i < 999; i++) {
		array2.push_back(i);
	}
	CHECK_MESSAGE(
			array2.size() == 999,
			"PagedArray #2 should have 999 elements.");

	all_match = true;
	for (uint32_t i = 0; i < 999; i++) {
		if (array2[i] != i) {
			all_match = false;
		}
	}

	CHECK_MESSAGE(
			all_match,
			"PagedArray #2 elements should match from 0 to 998.");

	array1.merge_unordered(array2);

	CHECK_MESSAGE(
			array1.size() == 123456 + 999,
			"PagedArray #1 should now be 123456 + 999 elements.");

	CHECK_MESSAGE(
			array2.size() == 0,
			"PagedArray #2 should now be 0 elements.");

	array1.reset(); //reset so pagepool can be reset
	array2.reset(); //reset so pagepool can be reset
	pool.reset();
}

TEST_CASE("[PagedArray] Extensive merge_unordered() test") {
	for (int page_size = 1; page_size <= 128; page_size *= 2) {
		PagedArrayPool<uint32_t> pool(page_size);
		PagedArray<uint32_t> array1;
		PagedArray<uint32_t> array2;
		array1.set_page_pool(&pool);
		array2.set_page_pool(&pool);

		const int max_count = 123;
		// Test merging arrays of lengths 0+123, 1+122, 2+121, ..., 123+0
		for (uint32_t j = 0; j < max_count; j++) {
			CHECK(array1.size() == 0);
			CHECK(array2.size() == 0);

			uint32_t sum = 12345;
			for (uint32_t i = 0; i < j; i++) {
				// Hashing the addend makes it extremely unlikely for any values
				// other than the original inputs to produce a matching sum
				uint32_t addend = hash_murmur3_one_32(i) + i;
				array1.push_back(addend);
				sum += addend;
			}
			for (uint32_t i = j; i < max_count; i++) {
				// See above
				uint32_t addend = hash_murmur3_one_32(i) + i;
				array2.push_back(addend);
				sum += addend;
			}

			CHECK(array1.size() == j);
			CHECK(array2.size() == max_count - j);

			array1.merge_unordered(array2);
			CHECK_MESSAGE(array1.size() == max_count, "merge_unordered() added/dropped elements while merging");

			// If any elements were altered during merging, the sum will not match up.
			for (uint32_t i = 0; i < array1.size(); i++) {
				sum -= array1[i];
			}
			CHECK_MESSAGE(sum == 12345, "merge_unordered() altered elements while merging");

			array1.clear();
		}

		array1.reset();
		array2.reset();
		pool.reset();
	}
}

} // namespace TestPagedArray

// === Merged from tests/core/templates/test_rid.cpp ===
/*  test_rid.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/templates/local_vector.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"

#ifdef TSAN_ENABLED
#include <sanitizer/tsan_interface.h>
#endif

namespace TestRID {

TEST_CASE("[RID] Default Constructor") {
	RID rid;

	CHECK(rid.get_id() == 0);
}

TEST_CASE("[RID] Factory method") {
	RID rid = RID::from_uint64(1);

	CHECK(rid.get_id() == 1);
}

TEST_CASE("[RID] Operators") {
	RID rid = RID::from_uint64(1);

	RID rid_zero = RID::from_uint64(0);
	RID rid_one = RID::from_uint64(1);
	RID rid_two = RID::from_uint64(2);

	CHECK_FALSE(rid == rid_zero);
	CHECK(rid == rid_one);
	CHECK_FALSE(rid == rid_two);

	CHECK_FALSE(rid < rid_zero);
	CHECK_FALSE(rid < rid_one);
	CHECK(rid < rid_two);

	CHECK_FALSE(rid <= rid_zero);
	CHECK(rid <= rid_one);
	CHECK(rid <= rid_two);

	CHECK(rid > rid_zero);
	CHECK_FALSE(rid > rid_one);
	CHECK_FALSE(rid > rid_two);

	CHECK(rid >= rid_zero);
	CHECK(rid >= rid_one);
	CHECK_FALSE(rid >= rid_two);

	CHECK(rid != rid_zero);
	CHECK_FALSE(rid != rid_one);
	CHECK(rid != rid_two);
}

TEST_CASE("[RID] 'is_valid' & 'is_null'") {
	RID rid_zero = RID::from_uint64(0);
	RID rid_one = RID::from_uint64(1);

	CHECK_FALSE(rid_zero.is_valid());
	CHECK(rid_zero.is_null());

	CHECK(rid_one.is_valid());
	CHECK_FALSE(rid_one.is_null());
}

TEST_CASE("[RID] 'get_local_index'") {
	CHECK(RID::from_uint64(1).get_local_index() == 1);
	CHECK(RID::from_uint64(4'294'967'295).get_local_index() == 4'294'967'295);
	CHECK(RID::from_uint64(4'294'967'297).get_local_index() == 1);
}

#ifdef THREADS_ENABLED
// This case would let sanitizers realize data races.
// Additionally, on purely weakly ordered architectures, it would detect synchronization issues
// if RID_Alloc failed to impose proper memory ordering and the test's threads are distributed
// among multiple L1 caches.
TEST_CASE("[RID_Owner] Thread safety") {
	struct DataHolder {
		char data[Thread::CACHE_LINE_BYTES];
	};

	struct RID_OwnerTester {
		uint32_t thread_count = 0;
		RID_Owner<DataHolder, true> rid_owner;
		TightLocalVector<Thread> threads;
		SafeNumeric<uint32_t> next_thread_idx;
		// Using std::atomic directly since SafeNumeric doesn't support relaxed ordering.
		TightLocalVector<std::atomic<uint64_t>> rids;
		std::atomic<uint32_t> sync[2] = {};
		std::atomic<uint32_t> correct = 0;

		// A barrier that doesn't introduce memory ordering constraints, only compiler ones.
		// The idea is not to cause any sync traffic that would make the code we want to test
		// seem correct as a side effect.
		void lockstep(uint32_t p_step) {
			uint32_t buf_idx = p_step % 2;
			uint32_t target = (p_step / 2 + 1) * threads.size();
			sync[buf_idx].fetch_add(1, std::memory_order_relaxed);
			do {
				Thread::yield();
			} while (sync[buf_idx].load(std::memory_order_relaxed) != target);
		}

		explicit RID_OwnerTester(bool p_chunk_for_all, bool p_chunks_preallocated) :
				thread_count(OS::get_processor_count()),
				rid_owner(sizeof(DataHolder) * (p_chunk_for_all ? thread_count : 1)) {
			threads.resize(thread_count);
			rids.resize(threads.size());
			if (p_chunks_preallocated) {
				LocalVector<RID> prealloc_rids;
				for (uint32_t i = 0; i < (p_chunk_for_all ? 1 : threads.size()); i++) {
					prealloc_rids.push_back(rid_owner.make_rid());
				}
				for (uint32_t i = 0; i < prealloc_rids.size(); i++) {
					rid_owner.free(prealloc_rids[i]);
				}
			}
		}

		~RID_OwnerTester() {
			for (uint32_t i = 0; i < threads.size(); i++) {
				rid_owner.free(RID::from_uint64(rids[i].load(std::memory_order_relaxed)));
			}
		}

		void test() {
			for (uint32_t i = 0; i < threads.size(); i++) {
				threads[i].start(
						[](void *p_data) {
							RID_OwnerTester *rot = (RID_OwnerTester *)p_data;

							auto _compute_thread_unique_byte = [](uint32_t p_idx) -> char {
								return ((p_idx & 0xff) ^ (0b11111110 << (p_idx % 8)));
							};

							// 1. Each thread gets a zero-based index.
							uint32_t self_th_idx = rot->next_thread_idx.postincrement();

							rot->lockstep(0);

							// 2. Each thread makes a RID holding unique data.
							DataHolder initial_data;
							memset(&initial_data, _compute_thread_unique_byte(self_th_idx), Thread::CACHE_LINE_BYTES);
							RID my_rid = rot->rid_owner.make_rid(initial_data);
							rot->rids[self_th_idx].store(my_rid.get_id(), std::memory_order_relaxed);

							rot->lockstep(1);

							// 3. Each thread verifies all the others.
							uint32_t local_correct = 0;
							for (uint32_t th_idx = 0; th_idx < rot->threads.size(); th_idx++) {
								if (th_idx == self_th_idx) {
									continue;
								}
								char expected_unique_byte = _compute_thread_unique_byte(th_idx);
								RID rid = RID::from_uint64(rot->rids[th_idx].load(std::memory_order_relaxed));
								DataHolder *data = rot->rid_owner.get_or_null(rid);
#ifdef TSAN_ENABLED
								__tsan_acquire(data); // We know not a race in practice.
#endif
								bool ok = true;
								for (uint32_t j = 0; j < Thread::CACHE_LINE_BYTES; j++) {
									if (data->data[j] != expected_unique_byte) {
										ok = false;
										break;
									}
								}
								if (ok) {
									local_correct++;
								}
#ifdef TSAN_ENABLED
								__tsan_release(data);
#endif
							}

							rot->lockstep(2);

							rot->correct.fetch_add(local_correct, std::memory_order_acq_rel);
						},
						this);
			}

			for (uint32_t i = 0; i < threads.size(); i++) {
				threads[i].wait_to_finish();
			}

			CHECK_EQ(correct.load(), threads.size() * (threads.size() - 1));
		}
	};

	SUBCASE("All items in one chunk, pre-allocated") {
		RID_OwnerTester tester(true, true);
		tester.test();
	}
	SUBCASE("All items in one chunk, NOT pre-allocated") {
		RID_OwnerTester tester(true, false);
		tester.test();
	}
	SUBCASE("One item per chunk, pre-allocated") {
		RID_OwnerTester tester(false, true);
		tester.test();
	}
	SUBCASE("One item per chunk, NOT pre-allocated") {
		RID_OwnerTester tester(false, false);
		tester.test();
	}
}
#endif // THREADS_ENABLED

} // namespace TestRID

// === Merged from tests/core/templates/test_self_list.cpp ===
/*  test_self_list.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/types.h"

namespace TestSelfList {

TEST_CASE("[SelfList] Sort") {
	const int SIZE = 5;
	int numbers[SIZE]{ 3, 2, 5, 1, 4 };
	SelfList<int> elements[SIZE]{
		SelfList<int>(&numbers[0]),
		SelfList<int>(&numbers[1]),
		SelfList<int>(&numbers[2]),
		SelfList<int>(&numbers[3]),
		SelfList<int>(&numbers[4]),
	};

	SelfList<int>::List list;
	for (int i = 0; i < SIZE; i++) {
		list.add_last(&elements[i]);
	}

	SelfList<int> *it = list.first();
	for (int i = 0; i < SIZE; i++) {
		CHECK_EQ(numbers[i], *it->self());
		it = it->next();
	}

	list.sort();
	it = list.first();
	for (int i = 1; i <= SIZE; i++) {
		CHECK_EQ(i, *it->self());
		it = it->next();
	}

	for (SelfList<int> &element : elements) {
		element.remove_from_list();
	}
}

} // namespace TestSelfList

// === Merged from tests/core/templates/test_span.cpp ===
/*  test_span.cpp                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/span.h"

namespace TestSpan {

TEST_CASE("[Span] Constexpr Validators") {
	constexpr Span<uint16_t> span_empty;
	static_assert(span_empty.ptr() == nullptr);
	static_assert(span_empty.size() == 0);
	static_assert(span_empty.is_empty());

	constexpr static uint16_t value = 5;
	Span<uint16_t> span_value(&value, 1);
	CHECK(span_value.ptr() == &value);
	CHECK(span_value.size() == 1);
	CHECK(!span_value.is_empty());

	static constexpr int ints[] = { 0, 1, 2, 3, 4, 5 };
	constexpr Span<int> span_array = ints;
	static_assert(span_array.size() == 6);
	static_assert(!span_array.is_empty());
	static_assert(span_array[0] == 0);
	static_assert(span_array[span_array.size() - 1] == 5);

	constexpr Span<char32_t> span_string = U"122345";
	static_assert(span_string.size() == 6);
	static_assert(!span_string.is_empty());
	static_assert(span_string[0] == U'1');
	static_assert(span_string[span_string.size() - 1] == U'5');

	CHECK_EQ(span_string, span_string); // Same identity / ptr.
	CHECK_EQ(span_string, Span(U"1223456", 6)); // Different ptr.
	CHECK_EQ(span_string, Span("122345").reinterpret<uint8_t>()); // Different type.

	int idx = 0;
	for (const char32_t &chr : span_string) {
		CHECK_EQ(chr, span_string[idx++]);
	}
}

} // namespace TestSpan

// === Merged from tests/core/templates/test_vector.cpp ===
/*  test_vector.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/vector.h"

namespace TestVector {

TEST_CASE("[Vector] List initialization") {
	Vector<int> vector{ 0, 1, 2, 3, 4 };

	CHECK(vector.size() == 5);
	CHECK(vector[0] == 0);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 2);
	CHECK(vector[3] == 3);
	CHECK(vector[4] == 4);
}

TEST_CASE("[Vector] Push back and append") {
	Vector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	// Alias for `push_back`.
	vector.append(4);

	CHECK(vector[0] == 0);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 2);
	CHECK(vector[3] == 3);
	CHECK(vector[4] == 4);
}

TEST_CASE("[Vector] Append array") {
	Vector<int> vector;
	vector.push_back(1);
	vector.push_back(2);

	Vector<int> vector_other;
	vector_other.push_back(128);
	vector_other.push_back(129);
	vector.append_array(vector_other);

	CHECK(vector.size() == 4);
	CHECK(vector[0] == 1);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 128);
	CHECK(vector[3] == 129);
}

TEST_CASE("[Vector] Insert") {
	Vector<int> vector;
	vector.insert(0, 2);
	vector.insert(0, 8);
	vector.insert(2, 5);
	vector.insert(1, 5);
	vector.insert(0, -2);

	CHECK(vector.size() == 5);
	CHECK(vector[0] == -2);
	CHECK(vector[1] == 8);
	CHECK(vector[2] == 5);
	CHECK(vector[3] == 2);
	CHECK(vector[4] == 5);
}

TEST_CASE("[Vector] Ordered insert") {
	Vector<int> vector;
	vector.ordered_insert(2);
	vector.ordered_insert(8);
	vector.ordered_insert(5);
	vector.ordered_insert(5);
	vector.ordered_insert(-2);

	CHECK(vector.size() == 5);
	CHECK(vector[0] == -2);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 5);
	CHECK(vector[3] == 5);
	CHECK(vector[4] == 8);
}

TEST_CASE("[Vector] Insert + Ordered insert") {
	Vector<int> vector;
	vector.ordered_insert(2);
	vector.ordered_insert(8);
	vector.insert(0, 5);
	vector.ordered_insert(5);
	vector.insert(1, -2);

	CHECK(vector.size() == 5);
	CHECK(vector[0] == 5);
	CHECK(vector[1] == -2);
	CHECK(vector[2] == 2);
	CHECK(vector[3] == 5);
	CHECK(vector[4] == 8);
}

TEST_CASE("[Vector] Fill large array and modify it") {
	Vector<int> vector;
	vector.resize(1'000'000);
	vector.fill(0x60d07);

	vector.write[200] = 0;
	CHECK(vector.size() == 1'000'000);
	CHECK(vector[0] == 0x60d07);
	CHECK(vector[200] == 0);
	CHECK(vector[499'999] == 0x60d07);
	CHECK(vector[999'999] == 0x60d07);
	vector.remove_at(200);
	CHECK(vector[200] == 0x60d07);

	vector.clear();
	CHECK(vector.size() == 0);
}

TEST_CASE("[Vector] Copy creation") {
	Vector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	Vector<int> vector_other = Vector<int>(vector);
	vector_other.remove_at(0);
	CHECK(vector_other[0] == 1);
	CHECK(vector_other[1] == 2);
	CHECK(vector_other[2] == 3);
	CHECK(vector_other[3] == 4);

	// Make sure the original vector isn't modified.
	CHECK(vector[0] == 0);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 2);
	CHECK(vector[3] == 3);
	CHECK(vector[4] == 4);
}

TEST_CASE("[Vector] Duplicate") {
	Vector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	Vector<int> vector_other = vector.duplicate();
	vector_other.remove_at(0);
	CHECK(vector_other[0] == 1);
	CHECK(vector_other[1] == 2);
	CHECK(vector_other[2] == 3);
	CHECK(vector_other[3] == 4);

	// Make sure the original vector isn't modified.
	CHECK(vector[0] == 0);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 2);
	CHECK(vector[3] == 3);
	CHECK(vector[4] == 4);
}

TEST_CASE("[Vector] Get, set") {
	Vector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	CHECK(vector.get(0) == 0);
	CHECK(vector.get(1) == 1);
	vector.set(2, 256);
	CHECK(vector.get(2) == 256);
	CHECK(vector.get(3) == 3);

	ERR_PRINT_OFF;
	// Invalid (but should not crash): setting out of bounds.
	vector.set(6, 500);
	ERR_PRINT_ON;

	CHECK(vector.get(4) == 4);
}

TEST_CASE("[Vector] To byte array (variant call)") {
	// PackedInt32Array.
	{
		PackedInt32Array vector[] = { { 0, -1, 2008 }, {} };
		PackedByteArray out[] = { { /* 0 */ 0x00, 0x00, 0x00, 0x00, /* -1 */ 0xFF, 0xFF, 0xFF, 0xFF, /* 2008 */ 0xD8, 0x07, 0x00, 0x00 }, {} };

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}

	// PackedInt64Array.
	{
		PackedInt64Array vector[] = { { 0, -1, 2008 }, {} };
		PackedByteArray out[] = { { /* 0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* -1 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* 2008 */ 0xD8, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, {} };

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}

	// PackedFloat32Array.
	{
		PackedFloat32Array vector[] = { { 0.0, -1.0, 200e24 }, {} };
		PackedByteArray out[] = { { /* 0.0 */ 0x00, 0x00, 0x00, 0x00, /* -1.0 */ 0x00, 0x00, 0x80, 0xBF, /* 200e24 */ 0xA6, 0x6F, 0x25, 0x6B }, {} };

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}
	// PackedFloat64Array.
	{
		PackedFloat64Array vector[] = { { 0.0, -1.0, 200e24 }, {} };
		PackedByteArray out[] = { { /* 0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* -1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF, /* 200e24 */ 0x35, 0x03, 0x32, 0xB7, 0xF4, 0xAD, 0x64, 0x45 }, {} };

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}

	// PackedStringArray.
	{
		PackedStringArray vector[] = { { "test", "string" }, {}, { "", "test" } };
		PackedByteArray out[] = { { /* test */ 0x74, 0x65, 0x73, 0x74, /* null */ 0x00, /* string */ 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, /* null */ 0x00 }, {}, { /* null */ 0x00, /* test */ 0x74, 0x65, 0x73, 0x74, /* null */ 0x00 } };

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}

	// PackedVector2Array.
	{
		PackedVector2Array vector[] = { { Vector2(), Vector2(1, -1) }, {} };
#ifdef REAL_T_IS_DOUBLE
		PackedByteArray out[] = { { /* X=0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Y=0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* X=1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, /* Y=-1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF }, {} };
#else
		PackedByteArray out[] = { { /* X=0.0 */ 0x00, 0x00, 0x00, 0x00, /* Y=0.0 */ 0x00, 0x00, 0x00, 0x00, /* X=1.0 */ 0x00, 0x00, 0x80, 0x3F, /* Y=-1.0 */ 0x00, 0x00, 0x80, 0xBF }, {} };
#endif

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}

	// PackedVector3Array.
	{
		PackedVector3Array vector[] = { { Vector3(), Vector3(1, 1, -1) }, {} };
#ifdef REAL_T_IS_DOUBLE
		PackedByteArray out[] = { { /* X=0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Y=0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Z=0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* X=1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, /* Y=1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, /* Z=-1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF }, {} };
#else
		PackedByteArray out[] = { { /* X=0.0 */ 0x00, 0x00, 0x00, 0x00, /* Y=0.0 */ 0x00, 0x00, 0x00, 0x00, /* Z=0.0 */ 0x00, 0x00, 0x00, 0x00, /* X=1.0 */ 0x00, 0x00, 0x80, 0x3F, /* Y=1.0 */ 0x00, 0x00, 0x80, 0x3F, /* Z=-1.0 */ 0x00, 0x00, 0x80, 0xBF }, {} };
#endif

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}

	// PackedColorArray.
	{
		PackedColorArray vector[] = { { Color(), Color(1, 1, 1) }, {} };
		PackedByteArray out[] = { { /* R=0.0 */ 0x00, 0x00, 0x00, 0x00, /* G=0.0 */ 0x00, 0x00, 0x00, 0x00, /* B=0.0 */ 0x00, 0x00, 0x00, 0x00, /* A=1.0 */ 0x00, 0x00, 0x80, 0x3F, /* R=1.0 */ 0x00, 0x00, 0x80, 0x3F, /* G=1.0 */ 0x00, 0x00, 0x80, 0x3F, /* B=1.0 */ 0x00, 0x00, 0x80, 0x3F, /* A=1.0 */ 0x00, 0x00, 0x80, 0x3F }, {} };

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}

	// PackedVector4Array.
	{
		PackedVector4Array vector[] = { { Vector4(), Vector4(1, -1, 1, -1) }, {} };
#ifdef REAL_T_IS_DOUBLE
		PackedByteArray out[] = { { /* X=0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Y=0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Z 0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* W=0.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* X=1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, /* Y=-1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF, /* Z=1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, /* W=-1.0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF }, {} };
#else
		PackedByteArray out[] = { { /* X=0.0 */ 0x00, 0x00, 0x00, 0x00, /* Y=0.0 */ 0x00, 0x00, 0x00, 0x00, /* Z=0.0 */ 0x00, 0x00, 0x00, 0x00, /* W 0.0 */ 0x00, 0x00, 0x00, 0x00, /* X 1.0 */ 0x00, 0x00, 0x80, 0x3F, /* Y=-1.0 */ 0x00, 0x00, 0x80, 0xBF, /* Z=1.0 */ 0x00, 0x00, 0x80, 0x3F, /* W=-1.0 */ 0x00, 0x00, 0x80, 0xBF }, {} };
#endif

		for (size_t i = 0; i < std_size(vector); i++) {
			Callable::CallError err;
			Variant v_ret;
			Variant v_vector = vector[i];
			v_vector.callp("to_byte_array", nullptr, 0, v_ret, err);
			CHECK(v_ret.get_type() == Variant::PACKED_BYTE_ARRAY);
			CHECK(v_ret.operator PackedByteArray() == out[i]);
		}
	}
}

TEST_CASE("[Vector] To byte array") {
	Vector<int> vector;
	vector.push_back(0);
	vector.push_back(-1);
	vector.push_back(2008);
	vector.push_back(999999999);

	Vector<uint8_t> byte_array = vector.to_byte_array();
	CHECK(byte_array.size() == 16);
	// vector[0]
	CHECK(byte_array[0] == 0);
	CHECK(byte_array[1] == 0);
	CHECK(byte_array[2] == 0);
	CHECK(byte_array[3] == 0);

	// vector[1]
	CHECK(byte_array[4] == 255);
	CHECK(byte_array[5] == 255);
	CHECK(byte_array[6] == 255);
	CHECK(byte_array[7] == 255);

	// vector[2]
	CHECK(byte_array[8] == 216);
	CHECK(byte_array[9] == 7);
	CHECK(byte_array[10] == 0);
	CHECK(byte_array[11] == 0);

	// vector[3]
	CHECK(byte_array[12] == 255);
	CHECK(byte_array[13] == 201);
	CHECK(byte_array[14] == 154);
	CHECK(byte_array[15] == 59);
}

TEST_CASE("[Vector] Slice") {
	Vector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	Vector<int> slice0 = vector.slice(0, 0);
	CHECK(slice0.size() == 0);

	Vector<int> slice1 = vector.slice(1, 3);
	CHECK(slice1.size() == 2);
	CHECK(slice1[0] == 1);
	CHECK(slice1[1] == 2);

	Vector<int> slice2 = vector.slice(1, -1);
	CHECK(slice2.size() == 3);
	CHECK(slice2[0] == 1);
	CHECK(slice2[1] == 2);
	CHECK(slice2[2] == 3);

	Vector<int> slice3 = vector.slice(3);
	CHECK(slice3.size() == 2);
	CHECK(slice3[0] == 3);
	CHECK(slice3[1] == 4);

	Vector<int> slice4 = vector.slice(2, -2);
	CHECK(slice4.size() == 1);
	CHECK(slice4[0] == 2);

	Vector<int> slice5 = vector.slice(-2);
	CHECK(slice5.size() == 2);
	CHECK(slice5[0] == 3);
	CHECK(slice5[1] == 4);

	Vector<int> slice6 = vector.slice(2, 42);
	CHECK(slice6.size() == 3);
	CHECK(slice6[0] == 2);
	CHECK(slice6[1] == 3);
	CHECK(slice6[2] == 4);

	ERR_PRINT_OFF;
	Vector<int> slice7 = vector.slice(5, 1);
	CHECK(slice7.size() == 0); // Expected to fail.
	ERR_PRINT_ON;
}

TEST_CASE("[Vector] Find, has") {
	Vector<int> vector;
	vector.push_back(3);
	vector.push_back(1);
	vector.push_back(4);
	vector.push_back(0);
	vector.push_back(2);

	CHECK(vector[0] == 3);
	CHECK(vector[1] == 1);
	CHECK(vector[2] == 4);
	CHECK(vector[3] == 0);
	CHECK(vector[4] == 2);

	CHECK(vector.find(0) == 3);
	CHECK(vector.find(1) == 1);
	CHECK(vector.find(2) == 4);
	CHECK(vector.find(3) == 0);
	CHECK(vector.find(4) == 2);

	CHECK(vector.find(-1) == -1);
	CHECK(vector.find(5) == -1);

	CHECK(vector.has(0));
	CHECK(vector.has(1));
	CHECK(vector.has(2));
	CHECK(vector.has(3));
	CHECK(vector.has(4));

	CHECK(!vector.has(-1));
	CHECK(!vector.has(5));
}

TEST_CASE("[Vector] Remove at") {
	Vector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	vector.remove_at(0);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 3);
	CHECK(vector[3] == 4);

	vector.remove_at(2);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 4);

	vector.remove_at(1);

	CHECK(vector[0] == 1);
	CHECK(vector[1] == 4);

	vector.remove_at(0);

	CHECK(vector[0] == 4);
}

TEST_CASE("[Vector] Remove at and find") {
	Vector<int> vector;
	vector.push_back(0);
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(4);

	CHECK(vector.size() == 5);

	vector.remove_at(0);

	CHECK(vector.size() == 4);

	CHECK(vector.find(0) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(2) != -1);
	CHECK(vector.find(3) != -1);
	CHECK(vector.find(4) != -1);

	vector.remove_at(vector.find(3));

	CHECK(vector.size() == 3);

	CHECK(vector.find(3) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(2) != -1);
	CHECK(vector.find(4) != -1);

	vector.remove_at(vector.find(2));

	CHECK(vector.size() == 2);

	CHECK(vector.find(2) == -1);
	CHECK(vector.find(1) != -1);
	CHECK(vector.find(4) != -1);

	vector.remove_at(vector.find(4));

	CHECK(vector.size() == 1);

	CHECK(vector.find(4) == -1);
	CHECK(vector.find(1) != -1);

	vector.remove_at(0);

	CHECK(vector.is_empty());
	CHECK(vector.size() == 0);
}

TEST_CASE("[Vector] Erase") {
	Vector<int> vector;
	vector.push_back(1);
	vector.push_back(3);
	vector.push_back(0);
	vector.push_back(2);
	vector.push_back(4);

	CHECK(vector.find(2) == 3);

	vector.erase(2);

	CHECK(vector.find(2) == -1);
	CHECK(vector.size() == 4);
}

TEST_CASE("[Vector] Size, resize, reserve") {
	Vector<int> vector;
	CHECK(vector.is_empty());
	CHECK(vector.size() == 0);

	vector.resize(10);

	CHECK(vector.size() == 10);

	vector.resize(5);

	CHECK(vector.size() == 5);

	vector.remove_at(0);
	vector.remove_at(0);
	vector.remove_at(0);

	CHECK(vector.size() == 2);

	vector.clear();

	CHECK(vector.size() == 0);
	CHECK(vector.is_empty());

	vector.push_back(0);
	vector.push_back(0);
	vector.push_back(0);

	CHECK(vector.size() == 3);

	vector.push_back(0);

	CHECK(vector.size() == 4);
}

TEST_CASE("[Vector] Sort") {
	Vector<int> vector;
	vector.push_back(2);
	vector.push_back(8);
	vector.push_back(-4);
	vector.push_back(5);
	vector.sort();

	CHECK(vector.size() == 4);
	CHECK(vector[0] == -4);
	CHECK(vector[1] == 2);
	CHECK(vector[2] == 5);
	CHECK(vector[3] == 8);
}

TEST_CASE("[Vector] Sort custom") {
	Vector<String> vector;
	vector.push_back("world");
	vector.push_back("World");
	vector.push_back("Hello");
	vector.push_back("10Hello");
	vector.push_back("12Hello");
	vector.push_back("01Hello");
	vector.push_back("1Hello");
	vector.push_back(".Hello");
	vector.sort_custom<NaturalNoCaseComparator>();

	CHECK(vector.size() == 8);
	CHECK(vector[0] == ".Hello");
	CHECK(vector[1] == "01Hello");
	CHECK(vector[2] == "1Hello");
	CHECK(vector[3] == "10Hello");
	CHECK(vector[4] == "12Hello");
	CHECK(vector[5] == "Hello");
	CHECK(vector[6] == "world");
	CHECK(vector[7] == "World");
}

TEST_CASE("[Vector] Search") {
	Vector<int> vector;
	vector.push_back(1);
	vector.push_back(2);
	vector.push_back(3);
	vector.push_back(5);
	vector.push_back(8);
	CHECK(vector.bsearch(2, true) == 1);
	CHECK(vector.bsearch(2, false) == 2);
	CHECK(vector.bsearch(5, true) == 3);
	CHECK(vector.bsearch(5, false) == 4);
}

TEST_CASE("[Vector] Operators") {
	Vector<int> vector;
	vector.push_back(2);
	vector.push_back(8);
	vector.push_back(-4);
	vector.push_back(5);

	Vector<int> vector_other;
	vector_other.push_back(2);
	vector_other.push_back(8);
	vector_other.push_back(-4);
	vector_other.push_back(5);

	CHECK(vector == vector_other);

	vector_other.push_back(10);
	CHECK(vector != vector_other);
}

struct CyclicVectorHolder {
	Vector<CyclicVectorHolder> *vector = nullptr;
	bool is_destructing = false;

	~CyclicVectorHolder() {
		if (is_destructing) {
			// The vector must exist and not expose its backing array at this point.
			CHECK_NE(vector, nullptr);
			CHECK_EQ(vector->ptr(), nullptr);
		}
	}
};

// Disable this test with ASAN, it will otherwise (rightfully) complain about use-after-poison.
#ifndef ASAN_ENABLED
TEST_CASE("[Vector] Cyclic Reference") {
	// Create a stack-space vector.
	Vector<CyclicVectorHolder> vector;
	// Add a new (empty) element.
	vector.resize(1);
	// Expose the vector to its element through CyclicVectorHolder.
	// This is questionable behavior, but should still behave graciously.
	vector.ptrw()[0] = CyclicVectorHolder{ &vector };
	vector.ptrw()[0].is_destructing = true;
	// The vector goes out of scope and destructs, calling CyclicVectorHolder's destructor.
}
#endif

} // namespace TestVector

// === Merged from tests/core/templates/test_vset.cpp ===
/*  test_vset.cpp                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"



#include "core/templates/vector.h"

namespace TestVSet {

template <typename T>
class TestClass : public VSet<T> {
public:
	int _find(const T &p_val, bool &r_exact) const {
		return VSet<T>::_find(p_val, r_exact);
	}
};

TEST_CASE("[VSet] _find and _find_exact correctness.") {
	TestClass<int> set;

	// insert some values
	set.insert(10);
	set.insert(20);
	set.insert(30);
	set.insert(40);
	set.insert(50);

	// data should be sorted
	CHECK(set.size() == 5);
	CHECK(set[0] == 10);
	CHECK(set[1] == 20);
	CHECK(set[2] == 30);
	CHECK(set[3] == 40);
	CHECK(set[4] == 50);

	// _find_exact return exact position for existing elements
	CHECK(set.find(10) == 0);
	CHECK(set.find(30) == 2);
	CHECK(set.find(50) == 4);

	// _find_exact return -1 for non-existing elements
	CHECK(set.find(15) == -1);
	CHECK(set.find(0) == -1);
	CHECK(set.find(60) == -1);

	// test _find
	bool exact;

	// existing elements
	CHECK(set._find(10, exact) == 0);
	CHECK(exact == true);

	CHECK(set._find(30, exact) == 2);
	CHECK(exact == true);

	// non-existing elements
	CHECK(set._find(25, exact) == 2);
	CHECK(exact == false);

	CHECK(set._find(35, exact) == 3);
	CHECK(exact == false);

	CHECK(set._find(5, exact) == 0);
	CHECK(exact == false);

	CHECK(set._find(60, exact) == 5);
	CHECK(exact == false);
}

} // namespace TestVSet

TEST_FORCE_LINK(test_templates)

