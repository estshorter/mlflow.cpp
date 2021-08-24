#include <chrono>
#include <iostream>
#include <mlflow.hpp>

int main(void) {
	using namespace std::chrono;

	mlflow::Client cli("http://localhost:5000");
	auto exp = cli.get_experiment_by_name("Default");
	if (!exp) {
		std::cerr << exp.error() << std::endl;
		return 1;
	}
	std::string exp_id = exp.value().experiment_id;
	auto run = cli.create_run(exp_id);
	if (!run) {
		std::cerr << run.error() << std::endl;
		return 1;
	}
	std::string run_id = run.value().info.run_id;
	mlflow::Param param{"KEY", "KEY_VALUE"};
	auto ret = cli.log_param(run_id, param);
	if (!ret) {
		std::cerr << ret.error() << std::endl;
		return 1;
	}
	ret = cli.set_tag(run_id, {"TAG", "TAG_VALUE"});
	ret = cli.log_batch(run_id, {}, {{"KEY1", "VALUE"}}, {{"KEY2", "VALU2"}});

	auto ret_ = cli.update_run(run_id, mlflow::RunStatus::FINISHED);
	if (!ret_) {
		std::cerr << ret_.error() << std::endl;
		return 1;
	}
	return 0;
}