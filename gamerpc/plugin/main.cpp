

#include <memory>
#include <sstream>
#include <filesystem>
#include <inja/inja.hpp>

#include "config.h"
#include "generator_helpers.h"
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/compiler/plugin.pb.h>

// Cpp Generator for Protobug IDL
class CppServiceGenerator : public google::protobuf::compiler::CodeGenerator {
 public:
  CppServiceGenerator() {}
  virtual ~CppServiceGenerator() {}

  uint64_t GetSupportedFeatures() const override {
    return FEATURE_PROTO3_OPTIONAL;
  }

  virtual bool Generate(const google::protobuf::FileDescriptor* file,
                        const std::string& parameter,
                        google::protobuf::compiler::GeneratorContext* context,
                        std::string* error) const override {
    if (file->options().cc_generic_services()) {
      *error =
          "cpp gamerpc proto compiler plugin does not work with generic "
          "services. To generate cpp gamerpc APIs, please set \""
          "cc_generic_service = false\".";
      return false;
    }

    google::protobuf::FileDescriptorProto fileDescriptorProto;
    file->CopyTo(&fileDescriptorProto);
    file->CopySourceCodeInfoTo(&fileDescriptorProto);

    nlohmann::json json = MessageToJson(fileDescriptorProto);

    google::protobuf::compiler::Version versionProto;
    context->GetCompilerVersion(&versionProto);
    json["version"] = MessageToJson(versionProto);

    bool use_system_headers = true;
    bool generate_mock_code = false;
    bool include_import_headers = false;
    std::string templateType = "gamerpc";
    std::vector<std::string> templatePath;

    if (!parameter.empty()) {
      std::vector<std::string> parameters_list =
          grpc_generator::tokenize(parameter, ",");
      for (auto parameter_string = parameters_list.begin();
           parameter_string != parameters_list.end(); parameter_string++) {
        std::vector<std::string> param =
            grpc_generator::tokenize(*parameter_string, "=");
        if (param[0] == "services_namespace") {
          json["services_namespace"] = param[1];
        } else if (param[0] == "use_system_headers") {
          if (param[1] == "true") {
            json["use_system_headers"] = true;
            use_system_headers = true;
          } else if (param[1] == "false") {
            json["use_system_headers"] = false;
            use_system_headers = false;
          } else {
            *error = std::string("Invalid parameter: ") + *parameter_string;
            return false;
          }
        } else if (param[0] == "gamerpc_search_path") {
          json["gamerpc_search_path"] = param[1];
        } else if (param[0] == "generate_mock_code") {
          if (param[1] == "true") {
            json["generate_mock_code"] = true;
            generate_mock_code = true;
          } else if (param[1] != "false") {
            *error = std::string("Invalid parameter: ") + *parameter_string;
            return false;
          }
        } else if (param[0] == "gmock_search_path") {
          json["gmock_search_path"] = param[1];
        } else if (param[0] == "additional_header_includes") {
          json["additional_header_includes"] = grpc_generator::tokenize(param[1], ":");
        } else if (param[0] == "message_header_extension") {
          json["message_header_extension"] = param[1];
        } else if (param[0] == "template_type" && !param[1].empty()) {
          templateType = param[1];
        } else if (param[0] == "template_path" && !param[1].empty()) {
          templatePath = grpc_generator::tokenize(param[1], "" + std::filesystem::path::preferred_separator);
        } else if (param[0] == "include_import_headers") {
          if (param[1] == "true") {
            json["include_import_headers"] = true;
            include_import_headers = true;
          } else if (param[1] != "false") {
            *error = std::string("Invalid parameter: ") + *parameter_string;
            return false;
          }
        } else {
          *error = std::string("Unknown parameter: ") + *parameter_string;
          return false;
        }
      }
    }

    std::string file_name = grpc_generator::StripProto(file->name());
    json["file_name"] = file_name;

    inja::Environment env;

    if (!GenerateFromTemplate(env, json, context, error, templatePath, templateType + ".pb.h", file_name + "." + templateType + ".pb.h")) {
        return false;
    }

    if (!GenerateFromTemplate(env, json, context, error, templatePath, templateType + ".pb.cpp", file_name + "." + templateType + ".pb.cpp")) {
        return false;
    }

    if (!generate_mock_code) {
      return true;
    }

    return GenerateFromTemplate(env, json, context, error, templatePath, templateType + "_mock.pb.h", file_name + "_mock." + templateType + ".pb.h");
  }

 private:
  // Insert the given code into the given file at the given insertion point.
  void Insert(google::protobuf::compiler::GeneratorContext* context,
              const std::string& filename, const std::string& insertion_point,
              const std::string& code) const {
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
        context->OpenForInsert(filename, insertion_point));
    google::protobuf::io::CodedOutputStream coded_out(output.get());
    coded_out.WriteRaw(code.data(), (int) code.size());
  }

  bool GenerateFromTemplate(inja::Environment& env, const nlohmann::json json,
                        google::protobuf::compiler::GeneratorContext* context,
                        std::string* error, const std::vector<std::string>& templatePath,
                        std::string templateName, std::string outputName) const {
    std::string mock_template = findInPath(templatePath, templateName);
    if (mock_template.empty()) {
        *error = std::string("Cannot find template in path: ") + templateName;
        return false;
    }
    std::string code = env.render_file(mock_template, json);
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
        context->Open(outputName));
    google::protobuf::io::CodedOutputStream coded_out(output.get());
    coded_out.WriteRaw(code.data(), (int) code.size());
    return true;
  }

  nlohmann::json MessageToJson(const google::protobuf::Message& message) const {
    std::string jsonString;
    google::protobuf::util::MessageToJsonString(message, &jsonString);
    return jsonString;
  }

  std::string findInPath(const std::vector<std::string>& searchPath, const std::string fileName) const {
    for (auto &pathString : searchPath) {
        std::filesystem::path path = pathString;
        path /= fileName;
        if (std::filesystem::is_regular_file(std::filesystem::status(path))) {
            return path.u8string();
        }
    }
    std::filesystem::path path = fileName;
    if (std::filesystem::is_regular_file(std::filesystem::status(path))) {
        return path.u8string();
    }
    return std::string();
  }
};


int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    CppServiceGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
