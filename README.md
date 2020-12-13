## Design by contract - C++


This is an implementation of Eiffel's [Design By Contract (tm)](https://www.eiffel.org/doc/solutions/Design_by_Contract_and_Assertions) features; Invariants, Require, Ensure, Do and Old in the C++ language that supports inheritance.

## How it works

dbc_cpp works by keeping a stack of predicates (requires, ensures and invariants) for each method call. When it detects a method call has completed,
it computes the total require, ensure and invariant predicates (including inherited methods) and throws an exception if they fail.

All the code must be within the DO() lambda clause to execute properly, you can specify capturing the entire state by using the form [&]() { code }

## Capabilities
All of the below work with inheritance, but, the library has not been battle tested so use with caution.

- Invariants
- Require 
- Do
- Ensure
- Old


## Example
```
class base {
INVARIANT(true, "base")
public:
 	virtual ~base() = default;
  
	virtual void func() {
		BASE_CALLS(); // always required at the start
    
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
INVARIANT(true, "example")

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
```
