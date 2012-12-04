#include <utility> // for std::forward
#include <iostream>


static int total = 0;
static void add(int n)
{
	std::cout << "Adding " << n << std::endl;
	total += n;
}

static void display(const char message[])
{
	std::cout << message << ": " << total << std::endl;
}

template <class Callable, typename... ArgTypes>
void* Call(Callable native_func, ArgTypes&&... args) noexcept
{
	std::clog << "in Call" << std::endl;
	return native_func(std::forward<ArgTypes>(args)...);
}

static void* test_lambda(int delta)
{
	std::clog << "in test_lambda" << std::endl;
	return Call([=](int delta)
	{
		std::clog << "in lambda" << std::endl;
		add(delta);
		display("total");
		return nullptr;
	}, delta);
}

int main(int argc, char* argv[])
{
	std::clog << "start" << std::endl;
	test_lambda(5);
	std::clog << "after first call" << std::endl;
	test_lambda(20);
	std::clog << "after second call" << std::endl;
	test_lambda(-256);
	std::clog << "after third call" << std::endl;
        return total != -231;
}
