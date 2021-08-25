# mlflow.cpp
**mlflow.cpp** is a C++ library which communicates with [mlflow](https://mlflow.org/) via its [REST API](https://www.mlflow.org/docs/latest/rest-api.html).
This library supports Windows10 and linux.

Note: not all REST APIs are implemented.

## Requiremtents
- C++17 or later
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [nlohmann/json](https://github.com/nlohmann/json)
- [result](https://github.com/bitwizeshift/result)

## Implemented APIs
- Create Experiment
- Get Experiment
- Get Experiment By Name
- Create Run
- Log Metric
- Log Batch
- Set Tag
- Log Param
- Update Run

Also, `mlflow::start_run()` and `mlflow::end_run()` are implemented as helper functions.

## Example
```cpp
#include <iostream>
#include <mlflow.hpp>
#include <chrono>
#include <thread>

cpp::result<void, std::string> send_data(mlflow::Client& cli) {
	using namespace std::chrono;
	mlflow::Param param{"PARAM1", "PARAM_VALUE1"};
	auto ret = cli.log_param(param);
	if (!ret) {
		return cpp::failure(ret.error());
	}
	ret = cli.set_tag({"TAG1", "TAG_VALUE1"});
	if (!ret) {
		return cpp::failure(ret.error());
	}
	ret = cli.log_batch({{"METRIC_KEY", "-1.0", 0}}, {{"PARAM2", "PARAM_VALUE2"}},
						{{"TAG2", "TAG_VALUE2"}});
	if (!ret) {
		return cpp::failure(ret.error());
	}
	std::this_thread::sleep_for(100ms);
	ret = cli.log_metric({"METRIC_KEY", "0.0", 1});
	std::this_thread::sleep_for(100ms);
	ret = cli.log_metric({"METRIC_KEY", "1.0", 2});
	if (!ret) {
		return cpp::failure(ret.error());
	}
	return {};
}

cpp::result<void, std::string> example_high_level(void) {
	mlflow::Client cli("http://localhost:5000");
	auto ret_ = cli.start_run("example_high_level");
	if (!ret_) {
		return cpp::failure(ret_.error());
	}
	auto ret1 = send_data(cli);
	if (!ret1) {
		return cpp::failure(ret1.error());
	}	
	// called in the destructor
	//ret_ = cli.end_run(); 
	return {};
}

int main(void) {
	ret = example_high_level();
	if (!ret) {
		std::cerr << ret.error() << std::endl;
		return 1;
	}

	return 0;
}
```

