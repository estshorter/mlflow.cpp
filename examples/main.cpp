#include <mlflow.hpp>
#include <thread>

void send_data(mlflow::Client& cli) {
	using namespace std::chrono;
	mlflow::Param param{"PARAM1", "PARAM_VALUE1"};
	cli.log_param(param);
	cli.set_tag({"TAG1", "TAG_VALUE1"});
	cli.log_batch({{"METRIC_KEY", "-1.0", 0}}, {{"PARAM2", "PARAM_VALUE2"}},
				  {{"TAG2", "TAG_VALUE2"}});
	std::this_thread::sleep_for(100ms);
	cli.log_metric({"METRIC_KEY", "0.0", 1});
	std::this_thread::sleep_for(100ms);
	cli.log_metric({"METRIC_KEY", "1.0", 2});
}

void example_low_level(void) {
	mlflow::Client cli("http://localhost:5000");
	auto exp = cli.get_experiment_by_name("Default");
	std::string exp_id = exp.experiment_id;
	auto run = cli.create_run(exp_id);
	std::string run_id = run.info.run_id;
	cli.set_runid(run_id);
	cli.set_run_name("example_low_level");
	cli.set_user_name();
	cli.set_source_name();

	send_data(cli);

	cli.update_run(mlflow::RunStatus::FINISHED);
}

void example_high_level(void) {
	mlflow::Client cli("http://localhost:5000");
	cli.start_run("example_high_level");
	send_data(cli);
	// called in the destructor
	// ret_ = cli.end_run();
}

int main(void) {
	example_low_level();
	example_high_level();
	return 0;
}