#pragma once
#include <httplib.h>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#pragma warning(suppress : 5105)
#include <Lmcons.h>
#include <Windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#else
#include <pwd.h>
#endif

#include <chrono>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace mlflow {
namespace utils {
#ifdef _WIN32
inline std::string to_multibyte(UINT enc_dst, const std::wstring& src) {
	int length_multibyte = WideCharToMultiByte(enc_dst, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
	if (length_multibyte <= 0) {
		return "";
	}
	std::string dst(length_multibyte, 0);
	WideCharToMultiByte(enc_dst, 0, src.data(), -1, &dst[0], length_multibyte, NULL, NULL);
	return dst.erase(static_cast<size_t>(length_multibyte) - 1, 1);	 //ヌル文字削除
}
#endif

std::string get_user_name() {
#ifdef _WIN32
	wchar_t user_name[UNLEN + 1];
	DWORD user_name_size = UNLEN + 1;
	if (GetUserNameW(user_name, &user_name_size)) {
		return to_multibyte(CP_UTF8, user_name);
	}
	throw std::runtime_error("get_username: GetUserNameW failed");
#else
	uid_t uid = geteuid();
	struct passwd* pw = getpwuid(uid);
	if (pw) {
		return std::string(pw->pw_name);
	}
	throw std::runtime_error("get_username: getpwuid failed");
#endif
};

std::string get_program_path() {
#ifdef _WIN32
	DWORD nsize = _MAX_PATH + 1;
	int cnt = 0;
	while (true) {
		std::vector<wchar_t> path(nsize);
		auto rc = GetModuleFileNameW(nullptr, &path[0], nsize);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			nsize *= 2;
			cnt++;
			if (cnt > 15) {
				throw std::runtime_error("get_program_path: iterataton limit reached");
			}
			continue;
		} else if (rc == 0) {
			throw std::runtime_error("get_program_path: GetModuleFileNameW failed");
		} else {
			return to_multibyte(CP_UTF8, path.data());
		}
	}
	throw std::runtime_error("get_program_path: GetModuleFileName failed");

#else
	char exePath[PATH_MAX];
	ssize_t len = ::readlink("/proc/self/exe", exePath, sizeof(exePath));
	if (len == -1 || len == sizeof(exePath)) {
		throw std::runtime_error("readlink failed");
	}
	exePath[len] = '\0';
	return exePath;
#endif
}

}  // namespace utils
namespace detail {
const static std::vector<std::string> RunStatus = {"RUNNING", "SCHEDULED", "FINISHED", "FAILED",
												   "KILLED"};
const static std::vector<std::string> ViewType = {"ACTIVE_ONLY", "DELETED_ONLY", "ALL"};

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
	KeyValue(const std::string& key, const std::string& value) : key(key), value(value) {}
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
	Metric(const std::string& key, const std::string& value, std::int64_t timestamp,
		   std::int64_t step)
		: key(key), value(value), timestamp(timestamp), step(step){};
	Metric(const std::string& key, const std::string& value, std::int64_t step)
		: key(key),
		  value(value),
		  timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::system_clock::now().time_since_epoch())
						.count()),
		  step(step){};

   public:
	std::string key;
	std::string value;
	std::int64_t timestamp;
	std::int64_t step;
};

void to_json(nlohmann::json& j, const Metric& m) {
	j = nlohmann::json{{"key", m.key},
					   {"value", m.value},
					   {"timestamp", std::to_string(m.timestamp)},
					   {"step", std::to_string(m.step)}};
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
		{"status", detail::RunStatus[static_cast<int>(rd.status)]},
		{"start_time", std::to_string(rd.start_time)},
		{"end_time", std::to_string(rd.end_time)},
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
	Client(const std::string& scheme_host_port) : cli(scheme_host_port), run_id_(""){};
	Client(const std::string& host, int port) : cli(host, port), run_id_(""){};

	~Client() {
		if (running && !run_id_.empty()) {
			end_run();
		}
	}

	void set_proxy(const std::string& host, int port) { cli.set_proxy(host.c_str(), port); };

	std::string create_experiment(const std::string& name,
								  const std::string& artifact_location = "") {
		nlohmann::json send_data;

		send_data["name"] = name;
		if (artifact_location != "") {
			send_data["artifact_location"] = artifact_location;
		}

		auto res =
			cli.Post("/api/2.0/mlflow/experiments/create", send_data.dump(), "application/json");
		handle_http_method_result(res);

		return handle_http_body(res, "experiment_id", "create_experiment");
	}

	Experiment get_experiment(const std::string& experiment_id) {
		auto res = cli.Get(
			("/api/2.0/mlflow/experiments/get?experiment_id=" + detail::url_encode(experiment_id))
				.c_str());
		handle_http_method_result(res);

		return handle_http_body(res, "experiment", "get_experiment_by_name");
	}

	Experiment get_experiment_by_name(const std::string& name) {
		auto res = cli.Get(
			("/api/2.0/mlflow/experiments/get-by-name?experiment_name=" + detail::url_encode(name))
				.c_str());
		handle_http_method_result(res);

		return handle_http_body(res, "experiment", "get_experiment_by_name");
	}

	Run create_run(const std::string& experiment_id, const int64_t start_time,
				   const std::vector<RunTag>& tags = {}) {
		nlohmann::json send_data;
		send_data["experiment_id"] = experiment_id;
		send_data["start_time"] = start_time;
		send_data["tags"] = tags;
		auto res = cli.Post("/api/2.0/mlflow/runs/create", send_data.dump(), "application/json");
		handle_http_method_result(res);

		auto ret = handle_http_body(res, "run", "create_run");
		running = true;
		return ret;
	};

	Run create_run(const std::string& experiment_id, const std::vector<RunTag>& tags = {}) {
		using namespace std::chrono;
		auto unixtime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		return create_run(experiment_id, unixtime, tags);
	}

	RunInfo update_run(const std::string& run_id, RunStatus status, std::int64_t endtime) {
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
		handle_http_method_result(res);

		auto ret_ = handle_http_body(res, "run_info", "update_run");
		switch (status) {
			case RunStatus::FINISHED:
			case RunStatus::FAILED:
			case RunStatus::KILLED:
				running = false;
				break;
			case RunStatus::RUNNING:
				running = true;
				break;
			default:
				break;
		}
		return ret_;
	};

	RunInfo update_run(const std::string& run_id, RunStatus status) {
		using namespace std::chrono;
		auto unixtime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		return update_run(run_id, status, unixtime);
	}

	RunInfo update_run(RunStatus status) {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}
		return update_run(this->run_id_, status);
	}

	void log_metric(const std::string& run_id, const Metric& metric) {
		nlohmann::json send_data(metric);
		send_data["run_id"] = run_id;

		auto res =
			cli.Post("/api/2.0/mlflow/runs/log-metric", send_data.dump(), "application/json");
		return handle_http_method_result(res);
	};

	void log_metric(const Metric& metric) {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}
		return log_metric(run_id_, metric);
	};

	void log_batch(const std::string& run_id, const std::vector<Metric>& metrics = {},
				   const std::vector<Param>& params = {}, const std::vector<RunTag>& tags = {}) {
		nlohmann::json send_data;
		send_data["run_id"] = run_id;
		send_data["metrics"] = metrics;
		send_data["params"] = params;
		send_data["tags"] = tags;
		auto res = cli.Post("/api/2.0/mlflow/runs/log-batch", send_data.dump(), "application/json");
		return handle_http_method_result(res);
	};

	void log_batch(const std::vector<Metric>& metrics = {}, const std::vector<Param>& params = {},
				   const std::vector<RunTag>& tags = {}) {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}
		return log_batch(run_id_, metrics, params, tags);
	}

	void log_param(const std::string& run_id, const Param& param) {
		nlohmann::json send_data(param);
		send_data["run_id"] = run_id;

		auto res =
			cli.Post("/api/2.0/mlflow/runs/log-parameter", send_data.dump(), "application/json");
		return handle_http_method_result(res);
	};

	void log_param(const Param& param) {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}
		return log_param(run_id_, param);
	}

	void set_tag(const std::string& run_id, const RunTag& tag) {
		nlohmann::json send_data(tag);
		send_data["run_id"] = run_id;

		auto res = cli.Post("/api/2.0/mlflow/runs/set-tag", send_data.dump(), "application/json");
		return handle_http_method_result(res);
	};

	void set_tag(const RunTag& tag) {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}
		return set_tag(run_id_, tag);
	}

	void set_runid(const std::string& run_id) { run_id_ = run_id; }

	void set_run_name(const std::string& run_id, const std::string& run_name) {
		return set_tag(run_id, {"mlflow.runName", run_name});
	}

	void set_run_name(const std::string& run_name) {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}

		return set_run_name(run_id_, run_name);
	}

	void set_source_name(const std::string& run_id, const std::string& path) {
		return set_tag(run_id, {"mlflow.source.name", path});
	}

	void set_source_name() {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}

		auto path = utils::get_program_path();
		std::replace(path.begin(), path.end(), '\\', '/');
		return set_source_name(run_id_, path);
	}

	void set_user_name(const std::string& run_id, const std::string& username) {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}
		return set_tag(run_id, {"mlflow.user", username});
	}

	void set_user_name() {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}

		auto ret = utils::get_user_name();
		return set_user_name(run_id_, ret);
	}

	void end_run() {
		if (run_id_.empty()) {
			throw std::runtime_error("run_id_ is empty");
		}

		auto ret = update_run(run_id_, mlflow::RunStatus::FINISHED);
	}

	void start_run(const std::string& run_name = "", const std::string& run_id = "",
				   const std::string& experiment_id = "0") {
		if (!run_id.empty()) {
			auto ret = update_run(run_id, RunStatus::RUNNING, 0);
		}

		auto exp = get_experiment(experiment_id);

		std::string exp_id = exp.experiment_id;
		auto run = create_run(exp_id);

		set_runid(run.info.run_id);
		if (!run_name.empty()) {
			set_run_name(run_name);
		}
		set_tag({ "mlflow.source.type", "LOCAL" });
	}

   private:
	void handle_http_method_result(const httplib::Result& res) {
		if (!res) {
			throw std::runtime_error("conection error: " + httplib::to_string(res.error()));
		}
		if (res->status == 200) return;
		std::ostringstream oss;
		oss << "invalid status: " << res->status << httplib::detail::status_message(res->status);
		oss << ", body: " << std::endl << res->body;
		throw std::runtime_error(oss.str());
	};

	nlohmann::json handle_http_body(const httplib::Result& res, const std::string& key,
									const std::string& funcname) {
#ifdef _DEBUG
		std::cout << funcname << ":" << std::endl;
		std::cout << res->body << std::endl;
#endif
		nlohmann::json ret_json = nlohmann::json::parse(res->body);
		if (!ret_json.contains(key)) {
			throw std::runtime_error(funcname + ": invalid response body, expected keyword: \"" +
									 key + "\"" + "but " + res->body);
		}
		return ret_json[key];
	}

	httplib::Client cli;
	std::string run_id_;
	bool running = false;
};
}  // namespace mlflow