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

#pragma once

//#define DBC_CPP_DISABLE

#ifndef DBC_CPP_DISABLE

#include <deque>
#include <string>
#include <any>
#include <map>


struct require_entry {
	bool m_result;
	const char * m_class_name;
	const char * m_function_name;
};
struct ensure_entry {
	bool m_result;
	const char * m_class_name;
	const char * m_function_name;
};
struct invariant_entry {
	bool m_result;
	const char * m_class_name;
	const char * m_function_name;
};

struct old_entry {
	const char * m_old_var_name;
	std::any m_old_var_value;
};

struct method_call {
	std::deque<require_entry>
		m_require_stack; // bottom = derived, top = base
	std::deque<ensure_entry> m_ensure_stack;
	std::deque<invariant_entry> m_invariant_stack;
	std::deque<old_entry> m_old_stack;

	bool m_require_loop = false;
	bool m_ensure_loop = false;
	bool m_invariant_loop = false;

	void eval_requires();
	void eval_ensures();
	void eval_invariants();
};




std::any OLD(const char * m_var_name);
void OLD(const char * m_var_name, std::any m_var_val);
void CLEAR_OLDS();





bool RECURSING();
void RECURSE_INVARIANTS(invariant_entry p_inv,
						std::function<void(void)> base_class_calls);
void RECURSE_REQUIRES(require_entry p_re,
					  std::function<void(void)> base_class_calls);
void RECURSE_ENSURES(ensure_entry p_en,
					 std::function<void(void)> base_class_calls);
void DO(std::function<void(void)> func);





#define BASE_CALLS(p_x) std::function<void(void)> __base_calls = [&]() { p_x }

#define INVARIANT(p_predicate, p_class_name) private: 	\
const char * __class_name = p_class_name;				\
bool __invariant(const char * p_func_name) { 		\
    return p_predicate;                               \
};

#define CHECK_INVARIANT RECURSE_INVARIANTS({ __invariant(__func__), __class_name, __func__ }, __base_calls);

#define REQUIRE(p_predicate) RECURSE_REQUIRES({p_predicate, __class_name, __func__}, __base_calls);

#define ENSURE(p_predicate) 											\
RECURSE_ENSURES({p_predicate, __class_name, __func__}, __base_calls); 	\
if (!RECURSING()) {                                                 	\
    CLEAR_OLDS();                                   					\
}

#else

#define BASE_CALLS(p_x)
#define INVARIANT(p_predicate, p_class_name)
#define CHECK_INVARIANT
#define REQUIRE(p_predicate)
#define ENSURE(p_predicate)
#define OLD(p_var, p_var_2)
#define DO(x) x();

#endif