//
// MIT LICENSE
//
// Copyright (c) 2020 R.G.Bamford
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

/*

example usage:

class base {
public:
 	virtual ~base() = default;

	virtual void func() {
		BASE_CALLS();

		CHECK_INVARIANT

		REQUIRE(true);

		OLD("m_count", m_count);

		DO([this]() {
			m_count++;
		});

		ENSURE(
			m_count == (std::any_cast<int>(OLD("m_count")) + 1)
		);

		CHECK_INVARIANT
	}
}

class example : public base {
public:
 	virtual ~example() = default;

	virtual void func() {
		BASE_CALLS(
			base::func();
		);

		CHECK_INVARIANT

		REQUIRE(true);

		OLD("m_count", m_count);

		DO([this]() {
			m_count += 2;
		});

		ENSURE(
			m_count == (std::any_cast<int>(OLD("m_count")) + 2)
		);

		CHECK_INVARIANT
	}
}
 */

#include "dbc_cpp.h"

#ifndef DBC_CPP_DISABLE

static std::deque<method_call> g_method_call_stack;
static method_call g_current_method_call;

void method_call::eval_requires() {
	bool predicate_sum;
	bool first_require = true;

	require_entry failed_class;
	bool failed_class_found = false;

	for (require_entry & entry : m_require_stack) {
		if (first_require) {
			predicate_sum = entry.m_result;
			first_require = false;
		}

		if (!failed_class_found) {
			if (entry.m_result == false) {
				failed_class_found = true;
				failed_class = entry;
			}
		}

		predicate_sum |= entry.m_result;
	}

	m_require_stack.clear();

	if (!predicate_sum) {
		std::string class_name = failed_class.m_class_name;
		std::string func_name = failed_class.m_function_name;
		throw std::runtime_error(
			"requires failed @ " + class_name + "." + func_name
				+ "()");
	}
}

void method_call::eval_ensures() {
	bool predicate_sum;
	bool first_require = true;

	ensure_entry failed_class;
	bool failed_class_found = false;

	for (ensure_entry & entry : m_ensure_stack) {
		if (first_require) {
			predicate_sum = entry.m_result;
			first_require = false;
		}

		if (!failed_class_found) {
			if (entry.m_result == false) {
				failed_class_found = true;
				failed_class = entry;
			}
		}

		predicate_sum &= entry.m_result;
	}

	m_ensure_stack.clear();

	if (!predicate_sum) {
		std::string class_name = failed_class.m_class_name;
		std::string func_name = failed_class.m_function_name;
		throw std::runtime_error(
			"ensures failed @ " + class_name + "." + func_name + "()");
	}
}

void method_call::eval_invariants() {
	bool predicate_sum;
	bool first_require = true;

	invariant_entry failed_class;
	bool failed_class_found = false;

	for (invariant_entry & entry : m_invariant_stack) {
		if (first_require) {
			predicate_sum = entry.m_result;
			first_require = false;
		}

		if (!failed_class_found) {
			if (entry.m_result == false) {
				failed_class_found = true;
				failed_class = entry;
			}
		}

		predicate_sum &= entry.m_result;
	}

	m_invariant_stack.clear();

	if (!predicate_sum) {
		std::string class_name = failed_class.m_class_name;
		std::string func_name = failed_class.m_function_name;
		throw std::runtime_error(
			"invariant failed @ " + class_name + "." + func_name
				+ "()");
	}
}

bool RECURSING() {
	return g_current_method_call.m_require_loop
		|| g_current_method_call.m_ensure_loop
		|| g_current_method_call.m_invariant_loop;
}

std::any OLD(const char * m_var_name) {
	std::any found_val;
	bool did_find_val = false;
	for (old_entry & old_entry : g_current_method_call.m_old_stack) {
		if (old_entry.m_old_var_name == m_var_name) {
			found_val = old_entry.m_old_var_value;
			did_find_val = true;
		}
	}
	if (!did_find_val) {
		throw std::runtime_error(
			"old with name " + std::string(m_var_name) + " not found ");
	}
	return found_val;
}

void OLD(const char * m_var_name, std::any m_var_val) {
	bool did_find_val = false;

	// update
	for (old_entry & old_entry : g_current_method_call.m_old_stack) {
		if (old_entry.m_old_var_name == m_var_name) {
			old_entry.m_old_var_value = m_var_val;
			did_find_val = true;
		}
	}

	// or add new old.
	if (!did_find_val) {
		g_current_method_call.m_old_stack
			.push_back({m_var_name, m_var_val});
	}
}

void CLEAR_OLDS() {
	g_current_method_call.m_old_stack.clear();
}

void RECURSE_INVARIANTS(invariant_entry p_inv,
						std::function<void(void)> base_class_calls) {
	if (g_current_method_call.m_invariant_loop) {

		std::deque<invariant_entry> temp_invariant_loop;
		g_method_call_stack.push_back(g_current_method_call);
		g_current_method_call = method_call();

		temp_invariant_loop.push_back(p_inv);

		g_current_method_call = g_method_call_stack.back();
		g_method_call_stack.pop_back();

		g_current_method_call.m_invariant_stack
			.insert(g_current_method_call.m_invariant_stack.end(),
					temp_invariant_loop.begin(),
					temp_invariant_loop.end());

	}
	else {

		std::deque<invariant_entry> temp_invariant_loop;
		g_method_call_stack.push_back(g_current_method_call);
		g_current_method_call = method_call();

		temp_invariant_loop.push_back(p_inv);

		g_current_method_call = g_method_call_stack.back();
		g_method_call_stack.pop_back();

		g_current_method_call.m_invariant_stack
			.insert(g_current_method_call.m_invariant_stack.end(),
					temp_invariant_loop.begin(),
					temp_invariant_loop.end());


		g_current_method_call.m_invariant_loop = true;
		base_class_calls();
		g_current_method_call.m_invariant_loop = false;

		g_current_method_call.eval_invariants();

	}
}

void RECURSE_REQUIRES(require_entry p_re,
					  std::function<void(void)> base_class_calls) {

	// When a method is called, it always pushes its assertions
	// to the current method call stack.
	if (g_current_method_call.m_require_loop) {

		std::deque<require_entry> temp_require_stack;
		g_method_call_stack.push_back(g_current_method_call);
		g_current_method_call = method_call();

		temp_require_stack.push_back(p_re);

		g_current_method_call = g_method_call_stack.back();
		g_method_call_stack.pop_back();

		g_current_method_call.m_require_stack
			.insert(g_current_method_call.m_require_stack.end(),
					temp_require_stack.begin(),
					temp_require_stack.end());

	}
	else {

		std::deque<require_entry> temp_require_stack;
		g_method_call_stack.push_back(g_current_method_call);
		g_current_method_call = method_call();

		temp_require_stack.push_back(p_re);

		g_current_method_call = g_method_call_stack.back();
		g_method_call_stack.pop_back();

		g_current_method_call.m_require_stack
			.insert(g_current_method_call.m_require_stack.end(),
					temp_require_stack.begin(),
					temp_require_stack.end());


		g_current_method_call.m_require_loop = true;
		base_class_calls();
		g_current_method_call.m_require_loop = false;

		g_current_method_call.eval_requires();
	}
}

void RECURSE_ENSURES(ensure_entry p_en,
					 std::function<void(void)> base_class_calls) {
	if (g_current_method_call.m_ensure_loop) {
		// on an ensure loop, just push the requires to the stack.
		// push and pop the method call to allow p_en to execute functions
		std::deque<ensure_entry> temp_ensure_stack;
		g_method_call_stack.push_back(g_current_method_call);
		g_current_method_call = method_call();

		temp_ensure_stack.push_back(p_en);

		g_current_method_call = g_method_call_stack.back();
		g_method_call_stack.pop_back();

		g_current_method_call.m_ensure_stack
			.insert(g_current_method_call.m_ensure_stack.end(),
					temp_ensure_stack.begin(),
					temp_ensure_stack.end());

	}
	else {

		std::deque<ensure_entry> temp_ensure_stack;
		g_method_call_stack.push_back(g_current_method_call);
		g_current_method_call = method_call();

		temp_ensure_stack.push_back(p_en);

		g_current_method_call = g_method_call_stack.back();
		g_method_call_stack.pop_back();

		g_current_method_call.m_ensure_stack
			.insert(g_current_method_call.m_ensure_stack.end(),
					temp_ensure_stack.begin(),
					temp_ensure_stack.end());

		g_current_method_call.m_ensure_loop = true;
		base_class_calls();
		g_current_method_call.m_ensure_loop = false;

		g_current_method_call.eval_ensures();
	}
}

void DO(std::function<void(void)> func) {
	if (!RECURSING()) {
		g_method_call_stack.push_back(g_current_method_call);
		g_current_method_call = method_call();

		func();

		g_current_method_call = g_method_call_stack.back();
		g_method_call_stack.pop_back();
	}
}

#endif