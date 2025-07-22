#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct DataObject {
  string id;
  int ts_read;
  int ts_write;

  DataObject(string obj_id) : id(obj_id), ts_read(-1), ts_write(-1) {}
  DataObject() {}
};

struct Operation {
  string type; // "r" | "w" | "c"
  string transaction;
  string object;
  int moment;

  Operation(string t, string trans, string obj, int m)
      : type(t), transaction(trans), object(obj), moment(m) {}
};

class TimestampScheduler {
private:
  map<string, int> transaction_timestamps;
  map<string, DataObject> data_objects;
  vector<string> objects;
  vector<string> transactions;
  vector<int> timestamps;

public:
  void parseInput(const string &filename) {
    ifstream file(filename);
    string line;
    int section = 0;

    while (getline(file, line)) {
      if (line.empty() || line[0] == '#')
        continue;

      if (line.find(';') != string::npos) {
        line = line.substr(0, line.find(';'));
        stringstream ss(line);
        string item;

        if (section == 0) { // Objects
          while (getline(ss, item, ',')) {
            item.erase(remove_if(item.begin(), item.end(), ::isspace),
                       item.end());
            objects.push_back(item);
            data_objects[item] = DataObject(item);
          }
          section++;
        } else if (section == 1) {
          while (getline(ss, item, ',')) {
            item.erase(remove_if(item.begin(), item.end(), ::isspace),
                       item.end());
            std::transform(item.begin(), item.end(), item.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            transactions.push_back(item);
          }
          section++;
        } else if (section == 2) {
          while (getline(ss, item, ',')) {
            item.erase(remove_if(item.begin(), item.end(), ::isspace),
                       item.end());
            timestamps.push_back(stoi(item));
          }

          for (int i = 0; i < transactions.size() && i < timestamps.size();
               i++) {
            transaction_timestamps[transactions[i]] = timestamps[i];
          }
          section++;
        }
      }
    }
    file.close();

    cout << "\n=== INFORMAÇÕES CARREGADAS ===" << endl;
    cout << "Objetos: ";
    for (int i = 0; i < objects.size(); i++) {
      cout << objects[i];
      if (i < objects.size() - 1)
        cout << ", ";
    }
    cout << endl;

    cout << "Transações: ";
    for (int i = 0; i < transactions.size(); i++) {
      cout << transactions[i];
      if (i < transactions.size() - 1)
        cout << ", ";
    }
    cout << endl;

    cout << "Timestamps: ";
    for (int i = 0; i < timestamps.size(); i++) {
      cout << timestamps[i];
      if (i < timestamps.size() - 1)
        cout << ", ";
    }
    cout << endl;

    cout << "\nMapeamento Transação -> Timestamp:" << endl;
    for (const auto &pair : transaction_timestamps) {
      cout << "  " << pair.first << " -> " << pair.second << endl;
    }
    cout << endl;
  }

  vector<Operation> parseSchedule(const string &schedule_line) {
    vector<Operation> operations;

    string ops_str = schedule_line.substr(schedule_line.find('-') + 1);

    regex op_regex(R"(([rwRW])\s*(\d+)\s*\(\s*([A-Za-z]+)\s*\)|[cC]\s*(\d+))");
    sregex_iterator iter(ops_str.begin(), ops_str.end(), op_regex);
    sregex_iterator end;

    int moment = 0;
    for (; iter != end; iter++) {
      smatch match = *iter;

      if (match[4].matched) { // Commit
        string transaction = "t" + match[4].str();
        operations.push_back(Operation("c", transaction, "", moment));
      } else {
        string type = match[1].str();
        std::transform(type.begin(), type.end(), type.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        string transaction = "t" + match[2].str();
        string object = match[3].str();
        operations.push_back(Operation(type, transaction, object, moment));
      }
      moment++;
    }

    return operations;
  }

  void printTimestampStructure(const string &schedule_id) {
    cout << "\n--- Estado da Estrutura de Timestamps para " << schedule_id
         << " ---" << endl;
    cout << left << setw(10) << "Objeto" << setw(12) << "TS-Read" << setw(12)
         << "TS-Write" << endl;
    cout << string(34, '-') << endl;

    for (const string &obj : objects) {
      const DataObject &data_obj = data_objects[obj];
      cout << left << setw(10) << obj << setw(12)
           << (data_obj.ts_read == -1 ? "NULL" : to_string(data_obj.ts_read))
           << setw(12)
           << (data_obj.ts_write == -1 ? "NULL" : to_string(data_obj.ts_write))
           << endl;
    }
    cout << endl;
  }

  string processSchedule(const string &schedule_line) {
    for (auto &pair : data_objects) {
      pair.second.ts_read = -1;
      pair.second.ts_write = -1;
    }

    string schedule_id = schedule_line.substr(0, schedule_line.find('-'));
    vector<Operation> operations = parseSchedule(schedule_line);

    cout << "=== PROCESSANDO " << schedule_id << " ===" << endl;
    cout << "Escalonamento: " << schedule_line << endl;

    // obj logs
    static set<string> opened_files_in_process;
    map<string, ofstream> object_files;

    const string log_dir = "obj_logs/";
    std::filesystem::create_directory(log_dir);

    for (const string &obj : objects) {
      ios_base::openmode mode;
      if (opened_files_in_process.count(obj)) {
        mode = ios::app;
      } else {
        mode = ios::trunc;
        opened_files_in_process.insert(obj);
      }

      string filename = log_dir + obj + ".txt";
      object_files[obj].open(filename, mode);
    }

    for (const Operation &op : operations) {
      if (op.type == "c") {
        cout << "Momento " << op.moment << ": " << op.type
             << op.transaction.substr(1) << " (commit)" << endl;
        continue;
      }

      int transaction_ts = transaction_timestamps[op.transaction];
      DataObject &data_obj = data_objects[op.object];

      cout << "Momento " << op.moment << ": " << op.type
           << op.transaction.substr(1) << "(" << op.object
           << ") [TS=" << transaction_ts << "]";

      if (object_files.count(op.object)) {
        object_files[op.object] << schedule_id << ", "
                                << (op.type == "r" ? "Read" : "Write") << ", "
                                << op.moment << endl;
      }

      if (op.type == "r") {
        cout << " -> Verificando: TS(" << op.transaction
             << ")=" << transaction_ts << " vs TS_write(" << op.object
             << ")=" << data_obj.ts_write;

        if (data_obj.ts_write > transaction_ts) {
          cout << " -> CONFLITO! Rollback necessário" << endl;
          for (auto &pair : object_files) {
            pair.second.close();
          }
          return schedule_id + "-ROLLBACK-" + to_string(op.moment);
        }

        data_obj.ts_read = max(data_obj.ts_read, transaction_ts);

        cout << " -> OK, TS_read(" << op.object << ") = " << data_obj.ts_read
             << endl;

      } else if (op.type == "w") {
        cout << " -> Verificando: TS(" << op.transaction
             << ")=" << transaction_ts << " vs TS_read(" << op.object
             << ")=" << data_obj.ts_read << " e TS_write(" << op.object
             << ")=" << data_obj.ts_write;

        if (data_obj.ts_read > transaction_ts ||
            data_obj.ts_write > transaction_ts) {
          cout << " -> CONFLITO! Rollback necessário" << endl;
          for (auto &pair : object_files) {
            pair.second.close();
          }
          return schedule_id + "-ROLLBACK-" + to_string(op.moment);
        }

        data_obj.ts_write = transaction_ts;

        cout << " -> OK, TS_write(" << op.object << ") = " << data_obj.ts_write
             << endl;
      }
    }
    for (auto &pair : object_files) {
      pair.second.close();
    }
    printTimestampStructure(schedule_id);

    return schedule_id + "-OK";
  }

  void processFile(const string &input_filename,
                   const string &output_filename) {
    parseInput(input_filename);

    ifstream input_file(input_filename);
    ofstream output_file(output_filename);
    string line;
    bool in_schedules = false;

    while (getline(input_file, line)) {
      if (line.empty())
        continue;

      if (line.find("E_") == 0) {
        in_schedules = true;
      }

      if (in_schedules && line.find("E_") == 0) {
        string result = processSchedule(line);
        output_file << result << endl;
        cout << "RESULTADO: " << result << endl;
        cout << string(50, '=') << endl;
      }
    }

    input_file.close();
    output_file.close();

    cout << "\n=== PROCESSAMENTO CONCLUÍDO ===" << endl;
    cout << "Arquivo de saída gerado: " << output_filename << endl;
    cout << "Arquivos de log dos objetos:" << endl;
    for (const string &obj : objects) {
      cout << "  - " << obj << ".txt" << endl;
    }
  }
};

int main() {
  TimestampScheduler scheduler;

  try {
    scheduler.processFile("in.txt", "out.txt");
    cout << "\nVerifique os arquivos gerados para os resultados completos."
         << endl;
  } catch (const exception &e) {
    cerr << "Erro: " << e.what() << endl;
    return 1;
  }

  return 0;
}
