#pragma once
#include <httplib.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>
#include <result.hpp>
#include <string>
#include <utility>
#include <vector>

namespace mlflow {
// https://stackoverflow.com/questions/154536/encode-decode-urls-in-c
std::string url_encode(const std::string& value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
		std::string::value_type c = (*i);

		// Keep alphanumeric and other accepted characters intact
		if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' ||
			c == '~') {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << std::uppercase;
		escaped << '%' << std::setw(2) << static_cast<int>((static_cast<unsigned char>(c)));
		escaped << std::nouppercase;
	}

	return escaped.str();
}

namespace detail {
const static std::vector<std::string> RunStatus = {"RUNNING", "SCHEDULED", "FINISHED", "FAILED",
												   "KILLED"};
const static std::vector<std::string> ViewType = {"ACTIVE_ONLY", "DELETED_ONLY", "ALL"};
};	// namespace detail

enum class RunStatus : int { RUNNING, SCHEDULED, FINISHED, FAILED, KILLED, UNINITALIZED };
RunStatus string_to_run_status(const std::string& str) {
	if (str == "RUNNING") {
		return RunStatus::RUNNING;
	}
	if (str == "SCHEDULED") {
		return RunStatus::SCHEDULED;
	}
	if (str == "FINISHED") {
		return RunStatus::FINISHED;
	}
	if (str == "FAILED") {
		return RunStatus::FAILED;
	}
	if (str == "KILLED") {
		return RunStatus::KILLED;
	}

	throw std::invalid_argument("invalid argument: " + str);
}
enum class ViewType : int { ACTIVE_ONLY, DELETED_ONLY, ALL, UNINITALIZED };

struct KeyValue {
   public:
	KeyValue() : key(""), value(""){};
	KeyValue(const std::string& key, const std::string& value) {
		this->key = key;
		this->value = value;
	}
	std::string key;
	std::string value;
};

void to_json(nlohmann::json& j, const KeyValue& kv) {
	j = nlohmann::json{{"key", kv.key}, {"value", kv.value}};
}

void from_json(const nlohmann::json& j, KeyValue& kv) {
	j.at("key").get_to(kv.key);
	j.at("value").get_to(kv.value);
}

using ExperimentTag = KeyValue;
using Param = KeyValue;
using RunTag = KeyValue;

struct Experiment {
   public:
	Experiment()
		: experiment_id(""),
		  name(""),
		  artifact_location(""),
		  lifecycle_stage(""),
		  last_update_time(0),
		  creation_time(0),
		  tags(){};
	std::string experiment_id;
	std::string name;
	std::string artifact_location;
	std::string lifecycle_stage;
	std::int64_t last_update_time;
	std::int64_t creation_time;
	std::vector<KeyValue> tags;
};

void from_json(const nlohmann::json& j, Experiment& e) {
	j.at("experiment_id").get_to(e.experiment_id);
	j.at("name").get_to(e.name);
	j.at("artifact_location").get_to(e.artifact_location);
	j.at("lifecycle_stage").get_to(e.lifecycle_stage);
	if (j.contains("last_update_time")) {
		e.last_update_time = std::stoll(j.at("last_update_time").get<std::string>());
	}
	if (j.contains("creation_time")) {
		e.creation_time = std::stoll(j.at("creation_time").get<std::string>());
	}
	if (j.contains("tags")) {
		j.at("tags").get_to<std::vector<KeyValue>>(e.tags);
	}
}

struct Metric {
   public:
	Metric() : key(""), value(""), timestamp(0), step(0){};

   public:
	std::string key;
	std::string value;
	std::int64_t timestamp;
	std::int64_t step;
};

void to_json(nlohmann::json& j, const Metric& m) {
	j = nlohmann::json{
		{"key", m.key}, {"value", m.value}, {"timestamp", m.timestamp}, {"step", m.step}};
}

void from_json(const nlohmann::json& j, Metric& m) {
	j.at("key").get_to(m.key);
	j.at("value").get_to(m.value);
	m.timestamp = std::stoll(j.at("timestamp").get<std::string>());
	m.step = std::stoll(j.at("step").get<std::string>());
}

struct RunData {
   public:
	RunData() : metrics(), params(), tags(){};

	std::vector<Metric> metrics;
	std::vector<Param> params;
	std::vector<RunTag> tags;
};

void to_json(nlohmann::json& j, const RunData& rd) {
	j = nlohmann::json{
		{"metrics", rd.metrics},
		{"params", rd.params},
		{"tags", rd.tags},
	};
}

void from_json(const nlohmann::json& j, RunData& rd) {
	if (j.contains("metrics")) {
		j.at("metrics").get_to(rd.metrics);
	}
	if (j.contains("params")) {
		j.at("params").get_to(rd.params);
	}
	if (j.contains("tags")) {
		j.at("tags").get_to(rd.tags);
	}
}

struct RunInfo {
   public:
	RunInfo()
		: run_id(""),
		  // run_uuid(""),
		  experiment_id(""),
		  user_id(""),
		  status(RunStatus::UNINITALIZED),
		  start_time(0),
		  end_time(0),
		  artifact_uri(""),
		  lifecycle_stage(""){};

	std::string run_id;
	// std::string run_uuid;
	std::string experiment_id;
	std::string user_id;
	RunStatus status;
	std::int64_t start_time;
	std::int64_t end_time;
	std::string artifact_uri;
	std::string lifecycle_stage;
};

void to_json(nlohmann::json& j, const RunInfo& rd) {
	j = nlohmann::json{
		{"run_id", rd.run_id},
		{"experiment_id", rd.experiment_id},
		{"user_id", rd.user_id},
		{"status", rd.status},
		{"start_time", rd.start_time},
		{"end_time", rd.end_time},
		{"artifact_uri", rd.artifact_uri},
		{"lifecycle_stage", rd.lifecycle_stage},
	};
}

void from_json(const nlohmann::json& j, RunInfo& rd) {
	j.at("run_id").get_to(rd.run_id);
	j.at("experiment_id").get_to(rd.experiment_id);
	j.at("user_id").get_to(rd.user_id);
	rd.status = string_to_run_status(j.at("status"));
	rd.start_time = std::stoll(j.at("start_time").get<std::string>());
	if (j.contains("end_time")) {
		rd.end_time = std::stoll(j.at("end_time").get<std::string>());
	}
	j.at("artifact_uri").get_to(rd.artifact_uri);
	j.at("lifecycle_stage").get_to(rd.lifecycle_stage);
}

struct Run {
   public:
	Run() : info(), data(){};
	RunInfo info;
	RunData data;
};

void to_json(nlohmann::json& j, const Run& rd) {
	j = nlohmann::json{
		{"info", rd.info},
		{"data", rd.data},
	};
}

void from_json(const nlohmann::json& j, Run& rd) {
	j.at("info").get_to(rd.info);
	j.at("data").get_to(rd.data);
}

class Client {
   public:
	Client(const std::string& scheme_host_port) : cli(scheme_host_port){};
	Client(const std::string& host, int port) : cli(host, port){};
	void set_proxy(const std::string& host, int port) { cli.set_proxy(host.c_str(), port); };

	cpp::result<std::string, std::string> create_experiment(
		const std::string& name, const std::string& artifact_location = "") {
		nlohmann::json send_data;

		send_data["name"] = name;
		if (artifact_location != "") {
			send_data["artifact_location"] = artifact_location;
		}

		auto res =
			cli.Post("/api/2.0/mlflow/experiments/create", send_data.dump(), "application/json");
		auto ret = handle_http_result(res);
		if (!ret) {
			return cpp::failure(ret.error());
		}
		return handle_http_body(res, "experiment_id", "create_experiment");
	}

	cpp::result<Experiment, std::string> get_experiment_by_name(const std::string& name) {
		auto res =
			cli.Get(("/api/2.0/mlflow/experiments/get-by-name?experiment_name=" + url_encode(name))
						.c_str());
		auto ret = handle_http_result(res);
		if (!ret) {
			return cpp::failure(ret.error());
		}

		return handle_http_body(res, "experiment", "get_experiment_by_name");
	}

	cpp::result<Run, std::string> create_run(const std::string& experiment_id,
											 const int64_t start_time
											 /* const std::vector<RunTag>& tags*/) {
		nlohmann::json send_data;
		send_data["experiment_id"] = experiment_id;
		send_data["start_time"] = start_time;
		auto res = cli.Post("/api/2.0/mlflow/runs/create", send_data.dump(), "application/json");
		auto ret = handle_http_result(res);
		if (!ret) {
			return cpp::failure(ret.error());
		}

		return handle_http_body(res, "run", "create_run");
	};

	cpp::result<Run, std::string> create_run(const std::string& experiment_id) {
		using namespace std::chrono;
		auto unixtime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		return create_run(experiment_id, unixtime);
	}

	cpp::result<RunInfo, std::string> update_run(const std::string& run_id, RunStatus status,
												 std::int64_t endtime) {
		nlohmann::json send_data;

		send_data["run_id"] = run_id;
		send_data["status"] = detail::RunStatus[static_cast<int>(status)];
		switch (status) {
			case RunStatus::FINISHED:
			case RunStatus::FAILED:
			case RunStatus::KILLED:
				send_data["end_time"] = endtime;
				break;
			default:
				break;
		}
		auto res = cli.Post("/api/2.0/mlflow/runs/update", send_data.dump(), "application/json");
		auto ret = handle_http_result(res);
		if (!ret) {
			return cpp::failure(ret.error());
		}

		return handle_http_body(res, "run_info", "update_run");
	};

	cpp::result<RunInfo, std::string> update_run(const std::string& run_id, RunStatus status) {
		using namespace std::chrono;
		auto unixtime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		return update_run(run_id, status, unixtime);
	}

	cpp::result<void, std::string> log_metric(const std::string& run_id, const Metric& metric) {
		nlohmann::json send_data;
		send_data["run_id"] = run_id;
		send_data["key"] = metric.key;
		send_data["value"] = metric.value;
		send_data["timestamp"] = metric.timestamp;
		send_data["step"] = metric.step;

		auto res =
			cli.Post("/api/2.0/mlflow/runs/log-metric", send_data.dump(), "application/json");
		auto ret = handle_http_result(res);
		if (!ret) {
			return cpp::failure(ret.error());
		}
		return {};
	};

	cpp::result<void, std::string> log_batch(const std::string& run_id,
											 const std::vector<Metric>& metrics = {},
											 const std::vector<Param>& params = {},
											 const std::vector<RunTag>& tags = {}) {
		nlohmann::json send_data;
		send_data["run_id"] = run_id;
		send_data["metrics"] = metrics;
		send_data["params"] = params;
		send_data["tags"] = tags;
		auto res = cli.Post("/api/2.0/mlflow/runs/log-batch", send_data.dump(), "application/json");
		auto ret = handle_http_result(res);
		if (!ret) {
			return cpp::failure(ret.error());
		}
		return {};
	};

	cpp::result<void, std::string> log_param(const std::string& run_id, const Param& param) {
		nlohmann::json send_data;
		send_data["run_id"] = run_id;
		send_data["key"] = param.key;
		send_data["value"] = param.value;

		auto res =
			cli.Post("/api/2.0/mlflow/runs/log-parameter", send_data.dump(), "application/json");
		auto ret = handle_http_result(res);
		if (!ret) {
			return cpp::failure(ret.error());
		}
		return {};
	};

	cpp::result<void, std::string> set_tag(const std::string& run_id, const RunTag& tag) {
		nlohmann::json send_data;
		send_data["run_id"] = run_id;
		send_data["key"] = tag.key;
		send_data["value"] = tag.value;

		auto res = cli.Post("/api/2.0/mlflow/runs/set-tag", send_data.dump(), "application/json");
		auto ret = handle_http_result(res);
		if (!ret) {
			return cpp::failure(ret.error());
		}
		return {};
	};

   private:
	cpp::result<void, std::string> handle_http_result(const httplib::Result& res) {
		if (!res) {
			std::ostringstream oss;
			oss << "invalid status: " << res->status;
			return cpp::failure(oss.str());
		}
		if (res->status != 200) {
			std::ostringstream oss;
			oss << "invalid status: " << res->status;
			oss << ", body: " << std::endl << res->body;
			return cpp::failure(oss.str());
		}
		return {};
	};

	cpp::result<nlohmann::json, std::string> handle_http_body(const httplib::Result& res,
															  const std::string& key,
															  const std::string& funcname) {
#ifdef _DEBUG
		std::cout << funcname << ":" << std::endl;
		std::cout << res->body << std::endl;
#endif
		nlohmann::json ret_json = nlohmann::json::parse(res->body);
		if (!ret_json.contains(key)) {
			return cpp::failure("invalid response body, expected keyword: \"" + key + "\"" +
								"but " + res->body);
		}
		return ret_json[key];
	}

	httplib::Client cli;
};
}  // namespace mlflow