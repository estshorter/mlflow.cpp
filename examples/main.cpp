#include <iostream>
#include <mlflow.hpp>
#include <chrono>
#include <thread>

cpp::result<void, std::string> example_low_level(void) {
	using namespace std::chrono;
	mlflow::Client cli("http://localhost:5000");
	auto exp = cli.get_experiment_by_name("Default");
	if (!exp) {
		return cpp::failure(exp.error());
	}
	std::string exp_id = exp.value().experiment_id;
	auto run = cli.create_run(exp_id);
	if (!run) {
		return cpp::failure(run.error());
	}
	std::string run_id = run.value().info.run_id;
	cli.set_runid(run_id);

	mlflow::Param param{"PARAM1", "PARAM_VALUE1"};
	auto ret = cli.log_param(param);
	if (!ret) {
		return cpp::failure(ret.error());
	}
	ret = cli.set_tag({"TAG1", "TAG_VALUE1"});
	if (!ret) {
		return cpp::failure(ret.error());
	}
	ret = cli.log_batch({}, {{"PARAM2", "PARAM_VALUE2"}}, {{"TAG2", "TAG_VALUE2"}});
	if (!ret) {
		return cpp::failure(ret.error());
	}
	ret = cli.log_metric({"METRIC_KEY", "0.0", 0});
	std::this_thread::sleep_for(1s);
	ret = cli.log_metric({"METRIC_KEY", "1.0", 1});
	if (!ret) {
		return cpp::failure(ret.error());
	}
	auto ret_ = cli.update_run(mlflow::RunStatus::FINISHED);
	if (!ret_) {
		return cpp::failure(ret_.error());
	}
	return {};
}

int main(void) {
	auto ret = example_low_level();
	if (!ret) {
		std::cerr << ret.error() << std::endl;
		return 1;
	}
	return 0;
}